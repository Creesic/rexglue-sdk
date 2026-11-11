#pragma once
/**
 * Custom soundtracks for XMP title playlists: the player's own music files
 * (MP3, WMA, FLAC, Ogg Vorbis, WAV) played by a title's radio alongside, or
 * instead of, the songs it hands XMP.
 *
 * The music folder (cvar xmp_custom_music_dir, default "Music" beside the
 * executable):
 *   - files in its root play on every station (title playlist);
 *   - each subfolder, sorted by name, is the next station's own list
 *     (subfolder i plays on station i % xmp_custom_music_stations).
 * xmp_custom_music_mode: 0 off, 1 mixed with the title's songs, 2 custom only.
 *
 * Song text comes from the files' tags (ID3, ASF/WMA, Vorbis comments, WAV
 * INFO) or the file name, and can be overridden per song in the sidecar file
 * soundtrack.tsv in the music folder (written by the PGR4 launcher):
 *   # comment lines
 *   <relative path>\t<title>\t<artist>\t<album>
 * An empty field keeps the file's own text. Paths use '/' and are relative to
 * the music folder; fields escape '\\', '\t', '\n' as \\, \t, \n. UTF-8.
 * The sidecar is re-read when it changes, so edits show on the next lookup.
 */

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace rex::kernel::xam::apps::xmp_custom {

// XMP track numbers of custom songs: kTrackNumberBase + the song's id (stable
// for the process: one id per file path).
constexpr uint32_t kTrackNumberBase = 0x00C50000;
constexpr uint32_t kTrackNumberLimit = 0x00C60000;
inline bool IsCustomTrackNumber(uint32_t n) {
  return n >= kTrackNumberBase && n < kTrackNumberLimit;
}

constexpr const char* kSidecarName = "soundtrack.tsv";

enum class Mode : int32_t { kOff = 0, kMixed = 1, kCustomOnly = 2 };

struct Track {
  std::filesystem::path path;  // absolute
  std::string relative;        // UTF-8, '/' separated: the sidecar key
  int folder = -1;             // -1: the root (every station), else subfolder index
  std::string folder_name;     // UTF-8 subfolder name ("" for the root)
  // The file's own text (empty when the tag is missing).
  std::u16string tag_title, tag_artist, tag_album;
  // The sidecar's text (empty: not overridden).
  std::u16string edit_title, edit_artist, edit_album;
  // What is shown: the edit, else the tag, else (title only) the file name.
  std::u16string title, artist, album;
  uint32_t duration_ms = 0;
  uint32_t id = 0;  // track number - kTrackNumberBase
};

Mode GetMode();
std::filesystem::path MusicDir();
int StationCount();
bool IsSupportedFile(const std::filesystem::path& path);

// Reads every supported file under `dir` (the root and one level of
// subfolders, deeper files belong to their subfolder) with its tags, then
// applies the sidecar. Blocking; `read_tags` false skips the tags (file
// names only).
std::vector<Track> ScanFolder(const std::filesystem::path& dir, bool read_tags = true);

// The library of MusicDir(), scanned in the background on first use and again
// when the folder changes. Waits up to `wait_ms` for a scan in progress.
std::vector<Track> Library(uint32_t wait_ms = 0);
// Starts the background scan now (when the mode is on).
void Prefetch();
// The custom song with this track number, with the sidecar's latest text.
bool LookupTrack(uint32_t track_number, Track& out);

// The songs station `station` gets (root files plus its subfolders').
std::vector<Track> TracksForStation(const std::vector<Track>& library, int station);

// Decodes a whole file to interleaved stereo float PCM.
bool DecodeFile(const std::filesystem::path& path, std::vector<float>& pcm, int& sample_rate,
                int& channels);

// Sidecar I/O (shared with tools: escape rules above).
struct Edit {
  std::string relative;
  std::u16string title, artist, album;
};
std::vector<Edit> ReadSidecar(const std::filesystem::path& dir);

}  // namespace rex::kernel::xam::apps::xmp_custom
