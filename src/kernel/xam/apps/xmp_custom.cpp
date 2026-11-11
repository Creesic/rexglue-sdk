/**
 * Custom soundtracks for XMP title playlists: see xmp_custom.h.
 */

#include <rex/kernel/xam/apps/xmp_custom.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
}

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>

REXCVAR_DEFINE_INT32(xmp_custom_music_mode, 0, "Audio",
                     "Custom soundtrack: 0 off, 1 the music folder mixed into the title's radio "
                     "stations, 2 the music folder only");
REXCVAR_DEFINE_STRING(xmp_custom_music_dir, "", "Audio",
                      "Custom soundtrack folder (empty: \"Music\" beside the executable). Root "
                      "files play on every station, each subfolder on the next station");
REXCVAR_DEFINE_INT32(xmp_custom_music_stations, 8, "Audio",
                     "Custom soundtrack: how many stations the title has (subfolder i plays on "
                     "station i modulo this)");

namespace rex::kernel::xam::apps::xmp_custom {
namespace fs = std::filesystem;

namespace {

constexpr int64_t kMaxDecodeSeconds = 30 * 60;  // longer files play their first half hour

// ---------------------------------------------------------------------------
// Text

// UTF-8 to UTF-16. Bytes that aren't valid UTF-8 are read as Windows-1252, the
// usual encoding of tags written as "Latin-1".
std::u16string Utf8ToUtf16(std::string_view s) {
  static const char16_t kCp1252[32] = {
      0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
      0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD, 0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
      0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178};
  std::u16string out;
  out.reserve(s.size());
  size_t i = 0;
  while (i < s.size()) {
    const uint8_t c = uint8_t(s[i]);
    uint32_t cp = 0;
    size_t len = 0;
    if (c < 0x80) {
      cp = c, len = 1;
    } else if ((c & 0xE0) == 0xC0) {
      cp = c & 0x1F, len = 2;
    } else if ((c & 0xF0) == 0xE0) {
      cp = c & 0x0F, len = 3;
    } else if ((c & 0xF8) == 0xF0) {
      cp = c & 0x07, len = 4;
    }
    bool ok = len > 0 && i + len <= s.size();
    for (size_t k = 1; ok && k < len; ++k) {
      const uint8_t cc = uint8_t(s[i + k]);
      ok = (cc & 0xC0) == 0x80;
      cp = (cp << 6) | (cc & 0x3F);
    }
    if (ok && len > 1 && (cp < (len == 2 ? 0x80u : len == 3 ? 0x800u : 0x10000u) || cp > 0x10FFFF ||
                          (cp >= 0xD800 && cp < 0xE000))) {
      ok = false;  // overlong or not a scalar value
    }
    if (!ok) {
      out.push_back(c >= 0x80 && c < 0xA0 ? kCp1252[c - 0x80] : char16_t(c));
      ++i;
      continue;
    }
    if (cp >= 0x10000) {
      cp -= 0x10000;
      out.push_back(char16_t(0xD800 + (cp >> 10)));
      out.push_back(char16_t(0xDC00 + (cp & 0x3FF)));
    } else {
      out.push_back(char16_t(cp));
    }
    i += len;
  }
  return out;
}

std::string Utf16ToUtf8(std::u16string_view s) {
  std::string out;
  for (size_t i = 0; i < s.size(); ++i) {
    uint32_t cp = s[i];
    if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] < 0xE000) {
      cp = 0x10000 + ((cp - 0xD800) << 10) + (s[++i] - 0xDC00);
    }
    if (cp < 0x80) {
      out.push_back(char(cp));
    } else if (cp < 0x800) {
      out.push_back(char(0xC0 | (cp >> 6)));
      out.push_back(char(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
      out.push_back(char(0xE0 | (cp >> 12)));
      out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back(char(0x80 | (cp & 0x3F)));
    } else {
      out.push_back(char(0xF0 | (cp >> 18)));
      out.push_back(char(0x80 | ((cp >> 12) & 0x3F)));
      out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back(char(0x80 | (cp & 0x3F)));
    }
  }
  return out;
}

// Trims spaces and control characters (tags often carry padding or NULs).
std::u16string Clean(std::u16string s) {
  for (auto& c : s) {
    if (c < 0x20 || c == 0x7F) {
      c = u' ';
    }
  }
  const auto first = s.find_first_not_of(u' ');
  if (first == std::u16string::npos) {
    return {};
  }
  return s.substr(first, s.find_last_not_of(u' ') - first + 1);
}

std::string PathUtf8(const fs::path& p) {
#ifdef _WIN32
  return Utf16ToUtf8(std::u16string_view(reinterpret_cast<const char16_t*>(p.native().c_str()),
                                         p.native().size()));
#else
  return p.native();
#endif
}

std::u16string PathUtf16(const fs::path& p) { return Utf8ToUtf16(PathUtf8(p)); }

std::string Lower(std::string s) {
  for (auto& c : s) {
    c = char(std::tolower(uint8_t(c)));
  }
  return s;
}

// ---------------------------------------------------------------------------
// Sidecar

std::string Unescape(std::string_view s) {
  std::string out;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      const char n = s[++i];
      out.push_back(n == 't' ? '\t' : n == 'n' ? '\n' : n == 'r' ? '\r' : n);
    } else {
      out.push_back(s[i]);
    }
  }
  return out;
}

std::vector<std::string> SplitTabs(std::string_view line) {
  std::vector<std::string> out;
  size_t start = 0;
  for (;;) {
    const size_t tab = line.find('\t', start);
    out.push_back(Unescape(line.substr(start, tab == std::string_view::npos ? line.npos : tab - start)));
    if (tab == std::string_view::npos) {
      return out;
    }
    start = tab + 1;
  }
}

FILE* OpenFile(const fs::path& path, const char* mode) {
#ifdef _WIN32
  std::wstring wmode(mode, mode + std::strlen(mode));
  FILE* f = nullptr;
  return _wfopen_s(&f, path.c_str(), wmode.c_str()) == 0 ? f : nullptr;
#else
  return std::fopen(path.c_str(), mode);
#endif
}

bool ReadWholeFile(const fs::path& path, std::vector<uint8_t>& out) {
  FILE* f = OpenFile(path, "rb");
  if (!f) {
    return false;
  }
  std::fseek(f, 0, SEEK_END);
  const long size = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (size < 0) {
    std::fclose(f);
    return false;
  }
  out.resize(size_t(size));
  const size_t got = size ? std::fread(out.data(), 1, out.size(), f) : 0;
  std::fclose(f);
  out.resize(got);
  return true;
}

// ---------------------------------------------------------------------------
// FFmpeg over a file / memory

struct FileIo {
  FILE* file = nullptr;
  int64_t size = 0;
};

int FileRead(void* opaque, uint8_t* buf, int size) {
  auto* io = static_cast<FileIo*>(opaque);
  const size_t got = std::fread(buf, 1, size_t(size), io->file);
  return got ? int(got) : AVERROR_EOF;
}

int64_t FileSeek(void* opaque, int64_t offset, int whence) {
  auto* io = static_cast<FileIo*>(opaque);
  if (whence == AVSEEK_SIZE) {
    return io->size;
  }
  whence &= ~AVSEEK_FORCE;
#ifdef _WIN32
  if (_fseeki64(io->file, offset, whence) != 0) {
    return -1;
  }
  return _ftelli64(io->file);
#else
  if (fseeko(io->file, offset, whence) != 0) {
    return -1;
  }
  return ftello(io->file);
#endif
}

// An opened container on a file, closed with it.
struct Container {
  FileIo io;
  AVIOContext* avio = nullptr;
  AVFormatContext* format = nullptr;

  ~Container() {
    if (format) {
      avformat_close_input(&format);
    }
    if (avio) {
      av_freep(&avio->buffer);
      avio_context_free(&avio);
    }
    if (io.file) {
      std::fclose(io.file);
    }
  }

  bool Open(const fs::path& path) {
    // FFmpeg's notes on players' files (cover art it can't decode, estimated
    // durations) are noise in the game's log; errors still show.
    static const bool quiet = [] {
      av_log_set_level(AV_LOG_ERROR);
      return true;
    }();
    (void)quiet;
    io.file = OpenFile(path, "rb");
    if (!io.file) {
      return false;
    }
    std::fseek(io.file, 0, SEEK_END);
#ifdef _WIN32
    io.size = _ftelli64(io.file);
#else
    io.size = ftello(io.file);
#endif
    std::fseek(io.file, 0, SEEK_SET);
    constexpr int kBufferSize = 64 * 1024;
    auto* buffer = static_cast<uint8_t*>(av_malloc(kBufferSize));
    avio = buffer ? avio_alloc_context(buffer, kBufferSize, 0, &io, FileRead, nullptr, FileSeek)
                  : nullptr;
    if (!avio) {
      av_free(buffer);
      return false;
    }
    format = avformat_alloc_context();
    if (!format) {
      return false;
    }
    format->pb = avio;
    format->flags |= AVFMT_FLAG_CUSTOM_IO;
    // The name only steers probing (by extension) when the content is unclear.
    const std::string name = PathUtf8(path.filename());
    if (avformat_open_input(&format, name.c_str(), nullptr, nullptr) < 0) {
      format = nullptr;  // freed by avformat_open_input
      return false;
    }
    return true;
  }
};

std::u16string Tag(const AVFormatContext* format, int stream, std::initializer_list<const char*> keys) {
  for (const char* key : keys) {
    const AVDictionary* dicts[2] = {format->metadata,
                                    stream >= 0 ? format->streams[stream]->metadata : nullptr};
    for (const AVDictionary* dict : dicts) {
      if (const AVDictionaryEntry* e = av_dict_get(dict, key, nullptr, 0)) {
        auto text = Clean(Utf8ToUtf16(e->value));
        if (!text.empty()) {
          return text;
        }
      }
    }
  }
  return {};
}

void ReadTags(Track& t) {
  Container c;
  if (!c.Open(t.path)) {
    REXKRNL_WARN("XMP custom: can't open '{}'", PathUtf8(t.path));
    return;
  }
  // The tags, and usually the duration, are known once the header is read. Only a file
  // without a duration (MP3 with no Xing/VBRI header) has its first frames probed, briefly.
  if (c.format->duration <= 0) {
    c.format->probesize = 128 * 1024;
    c.format->max_analyze_duration = AV_TIME_BASE / 2;
    avformat_find_stream_info(c.format, nullptr);
  }
  const int stream = av_find_best_stream(c.format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  t.tag_title = Tag(c.format, stream, {"title", "WM/Title", "TIT2"});
  t.tag_artist = Tag(c.format, stream, {"artist", "author", "album_artist", "WM/AlbumArtist", "TPE1", "performer"});
  t.tag_album = Tag(c.format, stream, {"album", "WM/AlbumTitle", "TALB"});
  if (c.format->duration > 0) {
    t.duration_ms = uint32_t(c.format->duration / (AV_TIME_BASE / 1000));
  } else if (stream >= 0) {
    // MP3 without a Xing/VBRI header: estimate from the size and bit rate.
    const AVStream* st = c.format->streams[stream];
    if (st->duration > 0 && st->time_base.den > 0) {
      t.duration_ms = uint32_t(st->duration * 1000 * st->time_base.num / st->time_base.den);
    } else if (st->codecpar->bit_rate > 0 && c.io.size > 0) {
      t.duration_ms = uint32_t(c.io.size * 8000 / st->codecpar->bit_rate);
    }
  }
}

// The text shown: the edit, else the tag, else the file name ("Artist - Title"
// file names give both).
void Resolve(Track& t) {
  std::u16string name_artist, name_title = PathUtf16(t.path.stem());
  const auto dash = name_title.find(u" - ");
  if (dash != std::u16string::npos && dash > 0 && dash + 3 < name_title.size()) {
    name_artist = Clean(name_title.substr(0, dash));
    name_title = Clean(name_title.substr(dash + 3));
  }
  name_title = Clean(name_title);
  auto pick = [](const std::u16string& edit, const std::u16string& tag, const std::u16string& fallback) {
    return !edit.empty() ? edit : !tag.empty() ? tag : fallback;
  };
  t.title = pick(t.edit_title, t.tag_title, name_title);
  t.artist = pick(t.edit_artist, t.tag_artist, t.tag_title.empty() ? name_artist : std::u16string());
  t.album = pick(t.edit_album, t.tag_album, {});
}

void ApplyEdits(std::vector<Track>& tracks, const std::vector<Edit>& edits) {
  std::map<std::string, const Edit*> by_path;
  for (const auto& e : edits) {
    by_path[Lower(e.relative)] = &e;
  }
  for (auto& t : tracks) {
    const auto it = by_path.find(Lower(t.relative));
    t.edit_title = it != by_path.end() ? it->second->title : std::u16string();
    t.edit_artist = it != by_path.end() ? it->second->artist : std::u16string();
    t.edit_album = it != by_path.end() ? it->second->album : std::u16string();
    Resolve(t);
  }
}

// ---------------------------------------------------------------------------
// Library

std::mutex g_mutex;
std::condition_variable g_scanned;
std::vector<Track> g_library;
std::map<std::string, uint32_t> g_ids;  // lowercased absolute path -> id
bool g_scanning = false, g_have_library = false;
std::string g_signature;  // the folder listing the library was scanned from
fs::path g_scanned_dir;
fs::file_time_type g_sidecar_time{};
std::chrono::steady_clock::time_point g_checked_at{};

// Supported files under `dir` (root, then subfolders by name).
struct Listing {
  struct File {
    fs::path path;
    int folder;
    std::string folder_name;
  };
  std::vector<File> files;
  std::string signature;
};

Listing ListFolder(const fs::path& dir) {
  Listing out;
  std::error_code ec;
  if (!fs::is_directory(dir, ec)) {
    return out;
  }
  std::vector<fs::path> root_files, subfolders;
  for (const auto& e : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
    if (e.is_directory(ec)) {
      subfolders.push_back(e.path());
    } else if (IsSupportedFile(e.path())) {
      root_files.push_back(e.path());
    }
  }
  auto by_name = [](const fs::path& a, const fs::path& b) {
    return Lower(PathUtf8(a.filename())) < Lower(PathUtf8(b.filename()));
  };
  std::sort(root_files.begin(), root_files.end(), by_name);
  std::sort(subfolders.begin(), subfolders.end(), by_name);
  auto add = [&](const fs::path& p, int folder, const std::string& folder_name) {
    out.files.push_back({p, folder, folder_name});
    const auto size = fs::file_size(p, ec);
    const auto time = fs::last_write_time(p, ec).time_since_epoch().count();
    out.signature += PathUtf8(p) + "|" + std::to_string(size) + "|" + std::to_string(time) + "\n";
  };
  for (const auto& p : root_files) {
    add(p, -1, "");
  }
  int index = 0;
  for (const auto& sub : subfolders) {
    std::vector<fs::path> files;
    for (const auto& e : fs::recursive_directory_iterator(sub, fs::directory_options::skip_permission_denied, ec)) {
      if (!e.is_directory(ec) && IsSupportedFile(e.path())) {
        files.push_back(e.path());
      }
    }
    if (files.empty()) {
      continue;
    }
    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
      return Lower(PathUtf8(a)) < Lower(PathUtf8(b));
    });
    const std::string name = PathUtf8(sub.filename());
    for (const auto& p : files) {
      add(p, index, name);
    }
    ++index;
  }
  return out;
}

std::vector<Track> ScanListing(const fs::path& dir, const Listing& listing, bool read_tags) {
  std::vector<Track> tracks;
  for (const auto& f : listing.files) {
    Track t;
    t.path = f.path;
    std::error_code ec;
    std::string rel = PathUtf8(fs::relative(f.path, dir, ec));
    std::replace(rel.begin(), rel.end(), '\\', '/');
    t.relative = rel;
    t.folder = f.folder;
    t.folder_name = f.folder_name;
    if (read_tags) {
      ReadTags(t);
    }
    tracks.push_back(std::move(t));
  }
  ApplyEdits(tracks, ReadSidecar(dir));
  return tracks;
}

void AssignIds(std::vector<Track>& tracks) {
  for (auto& t : tracks) {
    const std::string key = Lower(PathUtf8(t.path));
    auto it = g_ids.find(key);
    if (it == g_ids.end()) {
      it = g_ids.emplace(key, uint32_t(g_ids.size())).first;
    }
    t.id = it->second;
  }
}

void StartScanLocked(const fs::path& dir) {
  if (g_scanning) {
    return;
  }
  g_scanning = true;
  std::thread([dir] {
    const auto start = std::chrono::steady_clock::now();
    Listing listing = ListFolder(dir);
    auto tracks = ScanListing(dir, listing, true);
    std::error_code ec;
    const auto sidecar_time = fs::last_write_time(dir / kSidecarName, ec);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start).count();
    REXKRNL_INFO("XMP custom: {} songs in '{}' ({} ms)", tracks.size(), PathUtf8(dir), ms);
    for (const auto& t : tracks) {
      REXKRNL_DEBUG("XMP custom:   [{}] '{}' title='{}' artist='{}' album='{}' {} ms", t.folder,
                    t.relative, Utf16ToUtf8(t.title), Utf16ToUtf8(t.artist), Utf16ToUtf8(t.album),
                    t.duration_ms);
    }
    std::lock_guard lock(g_mutex);
    AssignIds(tracks);
    g_library = std::move(tracks);
    g_signature = std::move(listing.signature);
    g_scanned_dir = dir;
    g_sidecar_time = ec ? fs::file_time_type{} : sidecar_time;
    g_have_library = true;
    g_scanning = false;
    g_scanned.notify_all();
  }).detach();
}

// Rescans when the folder changed (checked at most every few seconds), and
// re-reads the sidecar when it changed.
void RefreshLocked() {
  const fs::path dir = MusicDir();
  const auto now = std::chrono::steady_clock::now();
  if (!g_have_library || g_scanned_dir != dir) {
    if (!g_scanning) {
      StartScanLocked(dir);
    }
    return;
  }
  if (now - g_checked_at < std::chrono::seconds(2)) {
    return;
  }
  g_checked_at = now;
  std::error_code ec;
  const auto sidecar_time = fs::last_write_time(dir / kSidecarName, ec);
  const auto t = ec ? fs::file_time_type{} : sidecar_time;
  if (t != g_sidecar_time) {
    g_sidecar_time = t;
    ApplyEdits(g_library, ReadSidecar(dir));
    REXKRNL_INFO("XMP custom: re-read {}", kSidecarName);
  }
  if (!g_scanning && ListFolder(dir).signature != g_signature) {
    REXKRNL_INFO("XMP custom: music folder changed, rescanning");
    StartScanLocked(dir);
  }
}

}  // namespace

Mode GetMode() {
  const int32_t m = REXCVAR_GET(xmp_custom_music_mode);
  return m == 1 ? Mode::kMixed : m == 2 ? Mode::kCustomOnly : Mode::kOff;
}

fs::path MusicDir() {
  const std::string dir = REXCVAR_GET(xmp_custom_music_dir);
  if (dir.empty()) {
    return rex::filesystem::GetExecutableFolder() / "Music";
  }
#ifdef _WIN32
  const std::u16string w = Utf8ToUtf16(dir);
  fs::path p(std::wstring(w.begin(), w.end()));
#else
  fs::path p(dir);
#endif
  return p.is_absolute() ? p : rex::filesystem::GetExecutableFolder() / p;
}

int StationCount() { return std::max(1, int(REXCVAR_GET(xmp_custom_music_stations))); }

bool IsSupportedFile(const fs::path& path) {
  // Hidden files, including macOS's "._" resource forks beside every copied file.
  const std::string name = PathUtf8(path.filename());
  if (name.empty() || name[0] == '.') {
    return false;
  }
  static const char* kExtensions[] = {".mp3", ".wma", ".flac", ".ogg", ".oga", ".wav"};
  const std::string ext = Lower(PathUtf8(path.extension()));
  for (const char* e : kExtensions) {
    if (ext == e) {
      return true;
    }
  }
  return false;
}

std::vector<Track> ScanFolder(const fs::path& dir, bool read_tags) {
  auto tracks = ScanListing(dir, ListFolder(dir), read_tags);
  std::lock_guard lock(g_mutex);
  AssignIds(tracks);
  return tracks;
}

void Prefetch() {
  if (GetMode() == Mode::kOff) {
    return;
  }
  std::lock_guard lock(g_mutex);
  RefreshLocked();
}

std::vector<Track> Library(uint32_t wait_ms) {
  std::unique_lock lock(g_mutex);
  RefreshLocked();
  if (g_scanning && !g_have_library && wait_ms) {
    g_scanned.wait_for(lock, std::chrono::milliseconds(wait_ms), [] { return !g_scanning; });
  }
  return g_library;
}

bool LookupTrack(uint32_t track_number, Track& out) {
  if (!IsCustomTrackNumber(track_number)) {
    return false;
  }
  std::lock_guard lock(g_mutex);
  RefreshLocked();
  for (const auto& t : g_library) {
    if (t.id == track_number - kTrackNumberBase) {
      out = t;
      return true;
    }
  }
  return false;
}

std::vector<Track> TracksForStation(const std::vector<Track>& library, int station) {
  const int stations = StationCount();
  std::vector<Track> out;
  for (const auto& t : library) {
    if (t.folder < 0 || t.folder % stations == station % stations) {
      out.push_back(t);
    }
  }
  return out;
}

std::vector<Edit> ReadSidecar(const fs::path& dir) {
  std::vector<Edit> edits;
  std::vector<uint8_t> data;
  if (!ReadWholeFile(dir / kSidecarName, data)) {
    return edits;
  }
  std::string text(data.begin(), data.end());
  if (text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
    text.erase(0, 3);
  }
  size_t start = 0;
  while (start < text.size()) {
    size_t end = text.find('\n', start);
    if (end == std::string::npos) {
      end = text.size();
    }
    std::string_view line(text.data() + start, end - start);
    start = end + 1;
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    if (line.empty() || line[0] == '#') {
      continue;
    }
    const auto f = SplitTabs(line);
    if (f.empty() || f[0].empty()) {
      continue;
    }
    Edit e;
    e.relative = f[0];
    std::replace(e.relative.begin(), e.relative.end(), '\\', '/');
    e.title = f.size() > 1 ? Clean(Utf8ToUtf16(f[1])) : std::u16string();
    e.artist = f.size() > 2 ? Clean(Utf8ToUtf16(f[2])) : std::u16string();
    e.album = f.size() > 3 ? Clean(Utf8ToUtf16(f[3])) : std::u16string();
    edits.push_back(std::move(e));
  }
  return edits;
}

bool DecodeFile(const fs::path& path, std::vector<float>& output, int& sample_rate, int& channels) {
  Container c;
  if (!c.Open(path)) {
    REXKRNL_ERROR("XMP custom: can't open '{}'", PathUtf8(path));
    return false;
  }
  if (avformat_find_stream_info(c.format, nullptr) < 0) {
    REXKRNL_ERROR("XMP custom: no stream info in '{}'", PathUtf8(path));
    return false;
  }
  const int stream_index = av_find_best_stream(c.format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (stream_index < 0) {
    REXKRNL_ERROR("XMP custom: no audio in '{}'", PathUtf8(path));
    return false;
  }
  AVStream* stream = c.format->streams[stream_index];
  const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!codec) {
    REXKRNL_ERROR("XMP custom: no decoder for codec {} ('{}')", int(stream->codecpar->codec_id),
                  PathUtf8(path));
    return false;
  }
  AVCodecContext* ctx = avcodec_alloc_context3(codec);
  AVPacket* packet = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  bool success = false;
  do {
    if (!ctx || !packet || !frame || avcodec_parameters_to_context(ctx, stream->codecpar) < 0 ||
        avcodec_open2(ctx, codec, nullptr) < 0) {
      REXKRNL_ERROR("XMP custom: can't open the decoder for '{}'", PathUtf8(path));
      break;
    }
    sample_rate = ctx->sample_rate;
    channels = ctx->channels;
    if (sample_rate <= 0 || channels <= 0) {
      break;
    }
    const size_t max_samples = size_t(kMaxDecodeSeconds) * size_t(sample_rate) * 2;
    auto sample_at = [](const uint8_t* p, AVSampleFormat f) -> float {
      switch (f) {
        case AV_SAMPLE_FMT_U8: return (float(*p) - 128.0f) / 128.0f;
        case AV_SAMPLE_FMT_S16: return float(*reinterpret_cast<const int16_t*>(p)) / 32768.0f;
        case AV_SAMPLE_FMT_S32: return float(*reinterpret_cast<const int32_t*>(p)) / 2147483648.0f;
        case AV_SAMPLE_FMT_FLT: return *reinterpret_cast<const float*>(p);
        case AV_SAMPLE_FMT_DBL: return float(*reinterpret_cast<const double*>(p));
        default: return 0.0f;
      }
    };
    bool failed = false;
    auto append = [&]() {
      const auto fmt = AVSampleFormat(frame->format);
      const auto base = av_get_packed_sample_fmt(fmt);
      const bool planar = av_sample_fmt_is_planar(fmt) != 0;
      const int bps = av_get_bytes_per_sample(base);
      const int ch = frame->channels > 0 ? frame->channels : channels;
      if (bps <= 0 || ch <= 0) {
        failed = true;
        return;
      }
      const size_t old = output.size();
      output.resize(old + size_t(frame->nb_samples) * 2);
      for (int i = 0; i < frame->nb_samples; ++i) {
        const uint8_t* l = planar ? frame->extended_data[0] + i * bps : frame->data[0] + size_t(i) * ch * bps;
        const uint8_t* r = ch == 1 ? l : planar ? frame->extended_data[1] + i * bps : l + bps;
        output[old + size_t(i) * 2] = sample_at(l, base);
        output[old + size_t(i) * 2 + 1] = sample_at(r, base);
      }
    };
    auto drain = [&]() {
      while (!failed) {
        const int r = avcodec_receive_frame(ctx, frame);
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) {
          return;
        }
        if (r < 0) {
          failed = true;
          return;
        }
        append();
      }
    };
    while (!failed && output.size() < max_samples && av_read_frame(c.format, packet) >= 0) {
      if (packet->stream_index == stream_index) {
        // A bad packet is skipped (a damaged frame shouldn't end the song).
        if (avcodec_send_packet(ctx, packet) >= 0) {
          drain();
        }
      }
      av_packet_unref(packet);
    }
    avcodec_send_packet(ctx, nullptr);
    drain();
    if (output.size() >= max_samples) {
      REXKRNL_WARN("XMP custom: '{}' is longer than {} minutes; playing the start", PathUtf8(path),
                   kMaxDecodeSeconds / 60);
    }
    success = !output.empty();
  } while (false);
  av_frame_free(&frame);
  av_packet_free(&packet);
  avcodec_free_context(&ctx);
  return success;
}

}  // namespace rex::kernel::xam::apps::xmp_custom
