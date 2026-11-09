/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <span>
#include <thread>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
}

#include <rex/kernel/xam/apps/xmp_app.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
extern "C" AVInputFormat ff_asf_demuxer;

namespace rex {
namespace kernel {
namespace xam {
using namespace rex::system;
using namespace rex::system::xam;
namespace apps {
using namespace rex::system;

namespace {

// Bumped every time a new track is requested. A background decode checks
// this before applying its result, so a slow decode from a track the
// player already skipped past doesn't clobber a newer one that finished
// first (or play out of order after the fact).
std::atomic<uint64_t> g_music_request_generation{0};

struct MemoryReader {
  const uint8_t* data = nullptr;
  size_t size = 0;
  size_t position = 0;
};

int ReadPacket(void* opaque, uint8_t* buffer, int buffer_size) {
  auto* reader = static_cast<MemoryReader*>(opaque);

  if (reader->position >= reader->size) {
    return AVERROR_EOF;
  }

  const size_t remaining = reader->size - reader->position;
  const size_t count = std::min<size_t>(remaining, buffer_size);
  std::memcpy(buffer, reader->data + reader->position, count);
  reader->position += count;
  return static_cast<int>(count);
}

int64_t Seek(void* opaque, int64_t offset, int whence) {
  auto* reader = static_cast<MemoryReader*>(opaque);

  if (whence == AVSEEK_SIZE) {
    return static_cast<int64_t>(reader->size);
  }

  whence &= ~AVSEEK_FORCE;

  int64_t target = 0;
  switch (whence) {
    case SEEK_SET:
      target = offset;
      break;
    case SEEK_CUR:
      target = static_cast<int64_t>(reader->position) + offset;
      break;
    case SEEK_END:
      target = static_cast<int64_t>(reader->size) + offset;
      break;
    default:
      return -1;
  }

  if (target < 0 || static_cast<uint64_t>(target) > reader->size) {
    return -1;
  }

  reader->position = static_cast<size_t>(target);
  return target;
}

float ConvertSample(const uint8_t* ptr, AVSampleFormat format) {
  switch (format) {
    case AV_SAMPLE_FMT_U8:
      return (static_cast<float>(*ptr) - 128.0f) / 128.0f;

    case AV_SAMPLE_FMT_S16:
      return static_cast<float>(*reinterpret_cast<const int16_t*>(ptr)) / 32768.0f;

    case AV_SAMPLE_FMT_S32:
      return static_cast<float>(*reinterpret_cast<const int32_t*>(ptr)) / 2147483648.0f;

    case AV_SAMPLE_FMT_FLT:
      return *reinterpret_cast<const float*>(ptr);

    case AV_SAMPLE_FMT_DBL:
      return static_cast<float>(*reinterpret_cast<const double*>(ptr));

    default:
      return 0.0f;
  }
}

bool DecodeWma(const std::vector<uint8_t>& file_data,
               std::vector<float>& output,
               int& sample_rate,
               int& channels) {
  MemoryReader reader{file_data.data(), file_data.size(), 0};

  constexpr int kIoBufferSize = 32 * 1024;
  auto* io_buffer = static_cast<uint8_t*>(av_malloc(kIoBufferSize));
  if (!io_buffer) {
    return false;
  }

  AVIOContext* avio = avio_alloc_context(
      io_buffer, kIoBufferSize, 0, &reader, ReadPacket, nullptr, Seek);
  if (!avio) {
    av_free(io_buffer);
    return false;
  }

  AVFormatContext* format = avformat_alloc_context();
  if (!format) {
    avio_context_free(&avio);
    return false;
  }

  format->pb = avio;
  format->flags |= AVFMT_FLAG_CUSTOM_IO;

  bool success = false;
  REXKRNL_WARN("XMP: DecodeWma: input size={} bytes", file_data.size());
  AVCodecContext* codec_context = nullptr;
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;

  do {
    {
      AVInputFormat* asf_format = &::ff_asf_demuxer;
      if (!asf_format) {
        REXKRNL_ERROR("XMP: ASF demuxer not found");
        break;
      }

      REXKRNL_WARN("XMP: forcing demuxer '{}'", asf_format->name);

      const int result =
          avformat_open_input(&format, nullptr, asf_format, nullptr);

      REXKRNL_WARN("XMP: DecodeWma: avformat_open_input -> {}", result);

      if (result < 0) {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {};
        av_strerror(result, error_buffer, sizeof(error_buffer));
        REXKRNL_ERROR(
            "XMP: avformat_open_input failed: {} ({})",
            result, error_buffer);
        break;
      }
    }

    {
      const int result = avformat_find_stream_info(format, nullptr);
      REXKRNL_WARN("XMP: DecodeWma: avformat_find_stream_info -> {} streams={} duration={}",
                   result, format->nb_streams, format->duration);
      if (result < 0) {
        REXKRNL_ERROR("XMP: avformat_find_stream_info failed: {}", result);
        break;
      }
    }

    const int stream_index =
        av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index < 0) {
      REXKRNL_ERROR("XMP: no audio stream found");
      break;
    }

    AVStream* stream = format->streams[stream_index];
    REXKRNL_WARN(
        "XMP: DecodeWma: stream={} codec={} sample_rate={} channels={} duration={}",
        stream_index,
        static_cast<int>(stream->codecpar->codec_id),
        stream->codecpar->sample_rate,
        stream->codecpar->channels,
        stream->duration);
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
      REXKRNL_ERROR("XMP: no decoder for codec {}", static_cast<int>(stream->codecpar->codec_id));
      break;
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
      break;
    }

    if (avcodec_parameters_to_context(codec_context, stream->codecpar) < 0) {
      REXKRNL_ERROR("XMP: avcodec_parameters_to_context failed");
      break;
    }

    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
      REXKRNL_ERROR("XMP: avcodec_open2 failed");
      break;
    }

    sample_rate = codec_context->sample_rate;
    channels = codec_context->channels;
    REXKRNL_WARN("XMP: DecodeWma: decoder opened: {} Hz, {} channels",
                 sample_rate, channels);

    if (sample_rate <= 0 || channels <= 0) {
      REXKRNL_ERROR("XMP: invalid decoded audio format {} Hz, {} channels",
                    sample_rate, channels);
      break;
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    if (!packet || !frame) {
      break;
    }

    const auto append_frame = [&]() -> bool {
      const auto format = static_cast<AVSampleFormat>(frame->format);
      const auto base_format = av_get_packed_sample_fmt(format);
      const bool planar = av_sample_fmt_is_planar(format) != 0;
      const int bytes_per_sample = av_get_bytes_per_sample(base_format);

      if (bytes_per_sample <= 0) {
        REXKRNL_ERROR("XMP: unsupported decoded sample format {}",
                      av_get_sample_fmt_name(format));
        return false;
      }

      const size_t old_size = output.size();

      if (channels == 1) {
        output.resize(old_size + static_cast<size_t>(frame->nb_samples) * 2);

        for (int i = 0; i < frame->nb_samples; ++i) {
          const uint8_t* src = planar
              ? frame->extended_data[0] + i * bytes_per_sample
              : frame->data[0] + i * bytes_per_sample;
          const float sample = ConvertSample(src, base_format);

          output[old_size + static_cast<size_t>(i) * 2] = sample;
          output[old_size + static_cast<size_t>(i) * 2 + 1] = sample;
        }
      } else {
        output.resize(old_size + static_cast<size_t>(frame->nb_samples) * 2);

        for (int i = 0; i < frame->nb_samples; ++i) {
          const uint8_t* left;
          const uint8_t* right;

          if (planar) {
            left = frame->extended_data[0] + i * bytes_per_sample;
            right = frame->extended_data[1] + i * bytes_per_sample;
          } else {
            left = frame->data[0] +
                   (static_cast<size_t>(i) * channels) * bytes_per_sample;
            right = left + bytes_per_sample;
          }

          output[old_size + static_cast<size_t>(i) * 2] =
              ConvertSample(left, base_format);
          output[old_size + static_cast<size_t>(i) * 2 + 1] =
              ConvertSample(right, base_format);
        }
      }

      return true;
    };

    const auto drain_decoder = [&]() -> bool {
      while (true) {
        const int result = avcodec_receive_frame(codec_context, frame);

        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
          return true;
        }

        if (result < 0) {
          return false;
        }

        if (!append_frame()) {
          return false;
        }
      }
    };

    while (av_read_frame(format, packet) >= 0) {
      if (packet->stream_index == stream_index) {
        const int send_result = avcodec_send_packet(codec_context, packet);
        av_packet_unref(packet);

        if (send_result < 0 || !drain_decoder()) {
          REXKRNL_ERROR("XMP: WMA decode failed");
          break;
        }
      } else {
        av_packet_unref(packet);
      }
    }

    if (avcodec_send_packet(codec_context, nullptr) < 0 || !drain_decoder()) {
      REXKRNL_ERROR("XMP: WMA decoder flush failed");
      break;
    }

    success = !output.empty();
    REXKRNL_WARN("XMP: DecodeWma: decoded {} float samples, success={}",
                 output.size(), success);
  } while (false);

  av_frame_free(&frame);
  av_packet_free(&packet);
  avcodec_free_context(&codec_context);
  avformat_close_input(&format);

  return success;
}

}  // namespace
XmpApp::XmpApp(KernelState* kernel_state)
    : App(kernel_state, 0xFA),
      state_(State::kIdle),
      playback_client_(PlaybackClient::kTitle),
      playback_mode_(PlaybackMode::kInOrder),
      repeat_mode_(RepeatMode::kPlaylist),
      unknown_flags_(0),
      volume_(1.0f),
      active_playlist_(nullptr),
      active_song_index_(0),
      next_playlist_handle_(1),
      next_song_handle_(1) {
  // The app lives as long as the kernel; the watcher with it.
  std::thread([this] { WatchForTrackEnd(); }).detach();
}

X_HRESULT XmpApp::XMPGetStatus(uint32_t state_ptr) {
  if (!XThread::GetCurrentThread()->main_thread()) {
    // Some stupid games will hammer this on a thread - induce a delay
    // here to keep from starving real threads.
    rex::thread::Sleep(std::chrono::milliseconds(1));
  }

  REXKRNL_TRACE("XMPGetStatus({:08X})", state_ptr);
  memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(state_ptr),
                                   static_cast<uint32_t>(state_));
  return X_E_SUCCESS;
}

X_HRESULT XmpApp::XMPCreateTitlePlaylist(uint32_t songs_ptr, uint32_t song_count,
                                         uint32_t playlist_name_ptr,
                                         const std::u16string& playlist_name, uint32_t flags,
                                         uint32_t out_song_handles, uint32_t out_playlist_handle) {
  REXKRNL_DEBUG(
      "XMPCreateTitlePlaylist({:08X}, {:08X}, {:08X}({}), {:08X}, {:08X}, "
      "{:08X})",
      songs_ptr, song_count, playlist_name_ptr, rex::string::to_utf8(playlist_name), flags,
      out_song_handles, out_playlist_handle);
  auto playlist = std::make_unique<Playlist>();
  playlist->handle = ++next_playlist_handle_;
  playlist->name = playlist_name;
  playlist->flags = flags;
  if (songs_ptr) {
    for (uint32_t i = 0; i < song_count; ++i) {
      auto song = std::make_unique<Song>();
      song->handle = ++next_song_handle_;
      uint8_t* song_base = memory_->TranslateVirtual(songs_ptr + (i * 36));
      song->file_path = memory::load_and_swap<std::u16string>(
          memory_->TranslateVirtual(memory::load_and_swap<uint32_t>(song_base + 0)));
      song->name = memory::load_and_swap<std::u16string>(
          memory_->TranslateVirtual(memory::load_and_swap<uint32_t>(song_base + 4)));
      song->artist = memory::load_and_swap<std::u16string>(
          memory_->TranslateVirtual(memory::load_and_swap<uint32_t>(song_base + 8)));
      song->album = memory::load_and_swap<std::u16string>(
          memory_->TranslateVirtual(memory::load_and_swap<uint32_t>(song_base + 12)));
      song->album_artist = memory::load_and_swap<std::u16string>(
          memory_->TranslateVirtual(memory::load_and_swap<uint32_t>(song_base + 16)));
      song->genre = memory::load_and_swap<std::u16string>(
          memory_->TranslateVirtual(memory::load_and_swap<uint32_t>(song_base + 20)));
      song->track_number = memory::load_and_swap<uint32_t>(song_base + 24);
      song->duration_ms = memory::load_and_swap<uint32_t>(song_base + 28);
      song->format = static_cast<Song::Format>(memory::load_and_swap<uint32_t>(song_base + 32));
      if (out_song_handles) {
        memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(out_song_handles + (i * 4)),
                                         song->handle);
      }
      playlist->songs.emplace_back(std::move(song));
    }
  }
  if (out_playlist_handle) {
    memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(out_playlist_handle),
                                     playlist->handle);
  }

  auto global_lock = global_critical_region_.Acquire();
  playlists_.insert({playlist->handle, playlist.get()});
  playlist.release();
  return X_E_SUCCESS;
}

X_HRESULT XmpApp::XMPDeleteTitlePlaylist(uint32_t playlist_handle) {
  REXKRNL_DEBUG("XMPDeleteTitlePlaylist({:08X})", playlist_handle);
  auto global_lock = global_critical_region_.Acquire();
  auto it = playlists_.find(playlist_handle);
  if (it == playlists_.end()) {
    REXKRNL_ERROR("Playlist {:08X} not found", playlist_handle);
    return X_E_NOTFOUND;
  }
  auto playlist = it->second;
  if (playlist == active_playlist_) {
    XMPStop(0);
  }
  playlists_.erase(it);
  delete playlist;
  return X_E_SUCCESS;
}

X_HRESULT XmpApp::XMPPlayTitlePlaylist(uint32_t playlist_handle, uint32_t song_handle) {
  REXKRNL_DEBUG("XMPPlayTitlePlaylist({:08X}, {:08X})", playlist_handle, song_handle);
  Playlist* playlist = nullptr;
  {
    auto global_lock = global_critical_region_.Acquire();
    auto it = playlists_.find(playlist_handle);
    if (it == playlists_.end()) {
      REXKRNL_ERROR("Playlist {:08X} not found", playlist_handle);
      return X_E_NOTFOUND;
    }
    playlist = it->second;
  }

  REXKRNL_WARN("XMPPlayTitlePlaylist: client={:08X}, songs={}", 
               static_cast<uint32_t>(playback_client_), playlist->songs.size());
  for (const auto& song : playlist->songs) {
    REXKRNL_WARN("  XMP song: {} format={} duration={}", 
                 rex::string::to_utf8(song->file_path),
                 static_cast<uint32_t>(song->format),
                 song->duration_ms);
  }

  if (!playlist->songs.empty()) {
    auto song_path = rex::string::to_utf8(playlist->songs[0]->file_path);
    auto* entry = REX_KERNEL_FS()->ResolvePath(song_path);
    if (!entry) {
      REXKRNL_ERROR("XMP: VFS could not resolve '{}'", song_path);
    } else {
      REXKRNL_WARN("XMP: VFS resolved '{}' -> '{}'",
                   song_path, entry->absolute_path());
    }
  }

  if (playlist->songs.empty()) {
    REXKRNL_WARN("XMPPlayTitlePlaylist: playlist contains no songs");
    return X_E_SUCCESS;
  }

  size_t song_index = 0;
  bool found = false;
  if (song_handle) {
    for (size_t i = 0; i < playlist->songs.size(); ++i) {
      if (playlist->songs[i]->handle == song_handle) {
        song_index = i;
        found = true;
        break;
      }
    }
    if (!found) {
      REXKRNL_WARN("XMPPlayTitlePlaylist: song handle {:08X} not found, using first song",
                   song_handle);
    }
  }
  // No song asked for in shuffle mode: start anywhere (PGR4's stations would
  // otherwise always open on the same track).
  if (!found && playback_mode_ == PlaybackMode::kShuffle && playlist->songs.size() > 1) {
    shuffle_state_ = shuffle_state_ * 1664525u + 1013904223u +
                     static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    song_index = (shuffle_state_ >> 8) % playlist->songs.size();
  }

  REXKRNL_WARN("XMPPlayTitlePlaylist: requested song handle={:08X}, selected index={}",
               song_handle, song_index);
  PlaySong(playlist, song_index);
  return X_E_SUCCESS;
}

void XmpApp::PlaySong(Playlist* playlist, size_t song_index) {
  auto* song = playlist->songs[song_index].get();
  REXKRNL_WARN("XMP: SELECTED index={} handle={:08X} path='{}'", song_index, song->handle,
               rex::string::to_utf8(song->file_path));

  // Active at once, so Next/Previous and GetInfo during the decode see it.
  active_playlist_ = playlist;
  active_song_index_ = static_cast<int>(song_index);
  track_queued_ = false;
  // Keep the shuffle order's place on the song that plays (a skip, or a
  // station started on a random song, starts a new order there).
  if (playback_mode_ == PlaybackMode::kShuffle && playlist->songs.size() > 1) {
    const auto it = std::find(shuffle_order_.begin(), shuffle_order_.end(), active_song_index_);
    if (shuffle_playlist_ != playlist || shuffle_order_.size() != playlist->songs.size() ||
        it == shuffle_order_.end()) {
      Reshuffle(active_song_index_);
    } else {
      shuffle_pos_ = size_t(it - shuffle_order_.begin());
    }
  }

  // Reading the file and decoding it (DecodeWma can take the better part
  // of a second for a full track) used to happen right here, blocking
  // whatever thread called XMPPlayTitlePlaylist. Skipping tracks rapidly
  // would then stall that thread once per skip. Do the slow part on a
  // background thread instead.
  const uint64_t request_generation =
      g_music_request_generation.fetch_add(1, std::memory_order_relaxed) + 1;

  std::string song_file_path = rex::string::to_utf8(song->file_path);
  XmpApp* self = this;

  std::thread([self, song_file_path, request_generation]() {
    auto* entry = REX_KERNEL_FS()->ResolvePath(song_file_path.empty()
                                                   ? std::string_view{}
                                                   : song_file_path);
    if (!entry) {
      REXKRNL_ERROR("XMP: failed to resolve music path");
      return;
    }

    rex::filesystem::File* file = nullptr;
    const auto open_status = entry->Open(
        rex::filesystem::FileAccess::kGenericRead, &file);

    if (XFAILED(open_status) || !file) {
      REXKRNL_ERROR("XMP: failed to open music file '{}': {:08X}",
                    song_file_path, open_status);
      return;
    }

    std::vector<uint8_t> file_data(entry->size());
    size_t bytes_read = 0;

    const auto read_status =
        file->ReadSync(std::span<uint8_t>(file_data.data(), file_data.size()),
                       0, &bytes_read);
    file->Destroy();

    if (XFAILED(read_status) || bytes_read == 0) {
      REXKRNL_ERROR("XMP: failed to read music file '{}': {:08X}",
                    song_file_path, read_status);
      return;
    }

    file_data.resize(bytes_read);

    std::vector<float> pcm;
    int sample_rate = 0;
    int channels = 0;

    if (!DecodeWma(file_data, pcm, sample_rate, channels)) {
      REXKRNL_ERROR("XMP: failed to decode '{}'", song_file_path);
      return;
    }

    // If the player skipped again while this was decoding, a newer
    // request has already taken over - drop this stale result instead
    // of playing a track the player already moved past.
    if (g_music_request_generation.load(std::memory_order_relaxed) !=
        request_generation) {
      REXKRNL_WARN("XMP: discarding stale decode for '{}' (superseded)",
                   song_file_path);
      return;
    }

    auto* audio = self->kernel_state_->emulator()->audio_system();
    if (!audio ||
        !audio->PlayMusicPcm(std::span<const float>(pcm.data(), pcm.size()),
                             sample_rate, channels)) {
      REXKRNL_ERROR("XMP: audio backend rejected decoded music");
      return;
    }

    // No SetMusicVolume here: the backend keeps the last volume and applies
    // it to the stream itself. Re-applying XMP's volume_ on every track would
    // clobber a title that sets music volume through its own mixer path
    // (PGR4 never calls XMPSetVolume, so volume_ stays 1.0).

    REXKRNL_INFO("XMP: playing '{}' ({} Hz, {} channels, {} samples)",
                 song_file_path, sample_rate, channels, pcm.size());

    self->track_queued_ = true;
    // XMPPause pauses the output device and XMPStop leaves it paused, so a
    // playlist started after pause -> stop (PGR4: the start screen or quitting
    // from the pause menu) stayed silent until the next pause/resume. Start
    // the output with the new song, unless the title paused while it decoded.
    if (self->state_ == State::kPaused) {
      audio->PauseMusic();
    } else {
      audio->ResumeMusic();
      self->state_ = State::kPlaying;
    }
    self->OnStateChanged();
    self->kernel_state_->BroadcastNotification(kMsgPlaybackBehaviorChanged, 1);
  }).detach();
}

uint32_t XmpApp::NextRandom() {
  shuffle_state_ = shuffle_state_ * 1664525u + 1013904223u;
  return shuffle_state_ >> 8;
}

void XmpApp::Reshuffle(int first) {
  const int count = static_cast<int>(active_playlist_->songs.size());
  shuffle_playlist_ = active_playlist_;
  shuffle_order_.resize(count);
  for (int i = 0; i < count; ++i) {
    shuffle_order_[i] = i;
  }
  for (int i = count - 1; i > 0; --i) {
    std::swap(shuffle_order_[i], shuffle_order_[NextRandom() % uint32_t(i + 1)]);
  }
  const auto it = std::find(shuffle_order_.begin(), shuffle_order_.end(), first);
  if (it != shuffle_order_.end()) {
    std::iter_swap(shuffle_order_.begin(), it);
  }
  shuffle_pos_ = 0;
}

int XmpApp::StepSongIndex(int step) {
  if (!active_playlist_ || active_playlist_->songs.empty()) {
    return -1;
  }
  const int count = static_cast<int>(active_playlist_->songs.size());
  if (playback_mode_ == PlaybackMode::kShuffle && count > 1) {
    if (shuffle_playlist_ != active_playlist_ || shuffle_order_.size() != size_t(count)) {
      Reshuffle(active_song_index_);
    }
    if (step > 0) {
      if (shuffle_pos_ + 1 < shuffle_order_.size()) {
        return shuffle_order_[shuffle_pos_ + 1];
      }
      // Every song has played: a new order, not starting on the song just heard.
      const int last = active_song_index_;
      Reshuffle(-1);
      if (shuffle_order_[0] == last) {
        std::swap(shuffle_order_[0], shuffle_order_[1 + NextRandom() % uint32_t(count - 1)]);
      }
      return shuffle_order_[0];  // PlaySong moves shuffle_pos_ to it
    }
    return shuffle_order_[shuffle_pos_ ? shuffle_pos_ - 1 : shuffle_order_.size() - 1];
  }
  const int next = active_song_index_ + step;
  if (next >= 0 && next < count) {
    return next;
  }
  return repeat_mode_ == RepeatMode::kPlaylist ? (next + count) % count : -1;
}

void XmpApp::WatchForTrackEnd() {
  // A played-out stream keeps a few bytes the resampler can't complete a
  // frame from (56 seen); under ~20 ms of float stereo counts as finished.
  constexpr int64_t kTrackEndBytes = 8192;
  for (;;) {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (state_ != State::kPlaying || !track_queued_) {
      continue;
    }
    auto* audio = kernel_state_->emulator()->audio_system();
    const int64_t queued = audio ? audio->MusicQueuedBytes() : -1;
    if (queued < 0 || queued >= kTrackEndBytes) {
      continue;
    }
    auto global_lock = global_critical_region_.Acquire();
    if (state_ != State::kPlaying || !track_queued_ || !active_playlist_) {
      continue;
    }
    track_queued_ = false;
    const int next = StepSongIndex(1);
    if (next < 0) {
      REXKRNL_INFO("XMP: playlist finished");
      XMPStop(0);
      continue;
    }
    REXKRNL_INFO("XMP: track finished, next index {}", next);
    PlaySong(active_playlist_, size_t(next));
  }
}

X_HRESULT XmpApp::XMPContinue() {
  REXKRNL_DEBUG("XMPContinue()");
  if (state_ == State::kPaused) {
    if (auto* audio = kernel_state_->emulator()->audio_system()) {
      audio->ResumeMusic();
    }
    state_ = State::kPlaying;
  }
  OnStateChanged();
  return X_E_SUCCESS;
}

X_HRESULT XmpApp::XMPStop(uint32_t unk) {
  assert_zero(unk);
  REXKRNL_DEBUG("XMPStop({:08X})", unk);
  if (auto* audio = kernel_state_->emulator()->audio_system()) {
    audio->StopMusic();
  }

  active_playlist_ = nullptr;
  active_song_index_ = 0;
  track_queued_ = false;
  state_ = State::kIdle;
  OnStateChanged();
  return X_E_SUCCESS;
}

X_HRESULT XmpApp::XMPPause() {
  REXKRNL_DEBUG("XMPPause()");
  if (state_ == State::kPlaying) {
    if (auto* audio = kernel_state_->emulator()->audio_system()) {
      audio->PauseMusic();
    }
    state_ = State::kPaused;
  }
  OnStateChanged();
  return X_E_SUCCESS;
}

X_HRESULT XmpApp::XMPNext() {
  REXKRNL_DEBUG("XMPNext()");
  auto global_lock = global_critical_region_.Acquire();
  if (!active_playlist_) {
    return X_E_NOTFOUND;
  }
  // A skip always moves on, even past the end of a non-repeating playlist.
  const int count = static_cast<int>(active_playlist_->songs.size());
  int next = StepSongIndex(1);
  if (next < 0) {
    next = (active_song_index_ + 1) % count;
  }
  state_ = State::kPlaying;
  PlaySong(active_playlist_, size_t(next));
  OnStateChanged();
  return X_E_SUCCESS;
}

X_HRESULT XmpApp::XMPPrevious() {
  REXKRNL_DEBUG("XMPPrevious()");
  auto global_lock = global_critical_region_.Acquire();
  if (!active_playlist_) {
    return X_E_NOTFOUND;
  }
  const int count = static_cast<int>(active_playlist_->songs.size());
  int previous = StepSongIndex(-1);
  if (previous < 0) {
    previous = (active_song_index_ + count - 1) % count;
  }
  state_ = State::kPlaying;
  PlaySong(active_playlist_, size_t(previous));
  OnStateChanged();
  return X_E_SUCCESS;
}

void XmpApp::OnStateChanged() {
  kernel_state_->BroadcastNotification(kMsgStateChanged, static_cast<uint32_t>(state_));
}

X_HRESULT XmpApp::DispatchMessageSync(uint32_t message, uint32_t buffer_ptr,
                                      uint32_t buffer_length) {

  if (message == 0x00070002 ||
      message == 0x00070003 ||
      message == 0x00070004 ||
      message == 0x00070005 ||
      message == 0x00070006 ||
      message == 0x00070007 ||
      message == 0x0007000E) {
    REXKRNL_WARN("XMP playback message: {:08X} buffer={:08X} length={}",
                 message, buffer_ptr, buffer_length);
  }

  // NOTE: buffer_length may be zero or valid.
  auto buffer = memory_->TranslateVirtual(buffer_ptr);
  switch (message) {
    case 0x00070002: {
      assert_true(!buffer_length || buffer_length == 12);
      uint32_t xmp_client = memory::load_and_swap<uint32_t>(buffer + 0);
      uint32_t storage_ptr = memory::load_and_swap<uint32_t>(buffer + 4);
      uint32_t song_handle = memory::load_and_swap<uint32_t>(buffer + 8);  // 0?
      uint32_t playlist_handle =
          memory::load_and_swap<uint32_t>(memory_->TranslateVirtual(storage_ptr));
      assert_true(xmp_client == 0x00000002);
      return XMPPlayTitlePlaylist(playlist_handle, song_handle);
    }
    case 0x00070003: {
      assert_true(!buffer_length || buffer_length == 4);
      uint32_t xmp_client = memory::load_and_swap<uint32_t>(buffer + 0);
      assert_true(xmp_client == 0x00000002);
      return XMPContinue();
    }
    case 0x00070004: {
      assert_true(!buffer_length || buffer_length == 8);
      uint32_t xmp_client = memory::load_and_swap<uint32_t>(buffer + 0);
      uint32_t unk = memory::load_and_swap<uint32_t>(buffer + 4);
      assert_true(xmp_client == 0x00000002);
      return XMPStop(unk);
    }
    case 0x00070005: {
      assert_true(!buffer_length || buffer_length == 4);
      uint32_t xmp_client = memory::load_and_swap<uint32_t>(buffer + 0);
      assert_true(xmp_client == 0x00000002);
      return XMPPause();
    }
    case 0x00070006: {
      assert_true(!buffer_length || buffer_length == 4);
      uint32_t xmp_client = memory::load_and_swap<uint32_t>(buffer + 0);
      assert_true(xmp_client == 0x00000002);
      return XMPNext();
    }
    case 0x00070007: {
      assert_true(!buffer_length || buffer_length == 4);
      uint32_t xmp_client = memory::load_and_swap<uint32_t>(buffer + 0);
      assert_true(xmp_client == 0x00000002);
      return XMPPrevious();
    }
    case 0x00070008: {
      assert_true(!buffer_length || buffer_length == 16);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> playback_mode;
        rex::be<uint32_t> repeat_mode;
        rex::be<uint32_t> flags;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 16);

      assert_true(args->xmp_client == 0x00000002 || args->xmp_client == 0x00000000);
      REXKRNL_DEBUG("XMPSetPlaybackBehavior({:08X}, {:08X}, {:08X})", uint32_t(args->playback_mode),
                    uint32_t(args->repeat_mode), uint32_t(args->flags));
      playback_mode_ = static_cast<PlaybackMode>(uint32_t(args->playback_mode));
      repeat_mode_ = static_cast<RepeatMode>(uint32_t(args->repeat_mode));
      unknown_flags_ = args->flags;
      kernel_state_->BroadcastNotification(kMsgPlaybackBehaviorChanged, 0);
      return X_E_SUCCESS;
    }
    case 0x00070009: {
      assert_true(!buffer_length || buffer_length == 8);
      uint32_t xmp_client = memory::load_and_swap<uint32_t>(buffer + 0);
      uint32_t state_ptr = memory::load_and_swap<uint32_t>(buffer + 4);  // out ptr to 4b - expect 0
      assert_true(xmp_client == 0x00000002);
      return XMPGetStatus(state_ptr);
    }
    case 0x0007000B: {
      assert_true(!buffer_length || buffer_length == 8);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> volume_ptr;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 8);

      assert_true(args->xmp_client == 0x00000002);
      REXKRNL_DEBUG("XMPGetVolume: volume={} ptr={:08X}", volume_, uint32_t(args->volume_ptr));
      memory::store_and_swap<float>(memory_->TranslateVirtual(args->volume_ptr), volume_);
      return X_E_SUCCESS;
    }
    case 0x0007000C: {
      assert_true(!buffer_length || buffer_length == 8);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<float> value;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 8);

      assert_true(args->xmp_client == 0x00000002);
      REXKRNL_WARN("!!! XMPSetVolume CALLED: value={} !!!", float(args->value));
      volume_ = std::clamp(float(args->value), 0.0f, 1.0f);

      REXKRNL_WARN("XMPSetVolume: requested={} clamped={}",
                   float(args->value), volume_);

      if (auto* audio = kernel_state_->emulator()->audio_system()) {
        audio->SetMusicVolume(volume_);
      }

      return X_E_SUCCESS;
    }
    case 0x0007000D: {
      assert_true(!buffer_length || buffer_length == 36);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> storage_ptr;
        rex::be<uint32_t> storage_size;
        rex::be<uint32_t> songs_ptr;
        rex::be<uint32_t> song_count;
        rex::be<uint32_t> playlist_name_ptr;
        rex::be<uint32_t> flags;
        rex::be<uint32_t> song_handles_ptr;
        rex::be<uint32_t> playlist_handle_ptr;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 36);

      memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(args->playlist_handle_ptr),
                                       args->storage_ptr);
      assert_true(args->xmp_client == 0x00000002 || args->xmp_client == 0x00000000);
      std::u16string playlist_name;
      if (!args->playlist_name_ptr) {
        playlist_name = u"";
      } else {
        playlist_name = memory::load_and_swap<std::u16string>(
            memory_->TranslateVirtual(args->playlist_name_ptr));
      }
      // dummy_alloc_ptr is the result of a XamAlloc of storage_size.
      assert_true(uint32_t(args->storage_size) == 4 + uint32_t(args->song_count) * 128);
      return XMPCreateTitlePlaylist(args->songs_ptr, args->song_count, args->playlist_name_ptr,
                                    playlist_name, args->flags, args->song_handles_ptr,
                                    args->storage_ptr);
    }
    case 0x0007000E: {
      assert_true(!buffer_length || buffer_length == 12);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> unk_ptr;  // 0
        rex::be<uint32_t> info_ptr;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 12);

      auto info = memory_->TranslateVirtual(args->info_ptr);
      assert_true(args->xmp_client == 0x00000002);
      assert_zero(args->unk_ptr);
      REXKRNL_ERROR("XMPGetInfo?({:08X}, {:08X})", uint32_t(args->unk_ptr),
                    uint32_t(args->info_ptr));
      if (!active_playlist_) {
        return X_E_FAIL;
      }
      auto& song = active_playlist_->songs[active_song_index_];

      REXKRNL_WARN(
          "XMPGetInfo: index={} handle={:08X} name='{}' artist='{}' album='{}' path='{}'",
          active_song_index_,
          song->handle,
          rex::string::to_utf8(song->name),
          rex::string::to_utf8(song->artist),
          rex::string::to_utf8(song->album),
          rex::string::to_utf8(song->file_path));

      // The strings are WCHAR[40] each (80 bytes): PGR4 reads the track
      // number back from +976 to find the song it handed XMP.
      auto store_text = [&](uint32_t offset, const std::u16string& text) {
        constexpr size_t kChars = 40;
        auto* dest = reinterpret_cast<uint16_t*>(info + offset);
        const size_t n = std::min(text.size(), kChars - 1);
        memory::store_and_swap<std::u16string_view>(dest, std::u16string_view(text).substr(0, n));
        memory::store_and_swap<uint16_t>(dest + n, 0);
      };
      memory::store_and_swap<uint32_t>(info + 0, song->handle);
      store_text(576 + 0 * 80, song->name);
      store_text(576 + 1 * 80, song->artist);
      store_text(576 + 2 * 80, song->album);
      store_text(576 + 3 * 80, song->album_artist);
      store_text(576 + 4 * 80, song->genre);
      memory::store_and_swap<uint32_t>(info + 976, song->track_number);
      memory::store_and_swap<uint32_t>(info + 980, song->duration_ms);
      memory::store_and_swap<uint32_t>(info + 984, static_cast<uint32_t>(song->format));
      return X_E_SUCCESS;
    }
    case 0x00070013: {
      assert_true(!buffer_length || buffer_length == 8);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> storage_ptr;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 8);

      uint32_t playlist_handle =
          memory::load_and_swap<uint32_t>(memory_->TranslateVirtual(args->storage_ptr));
      assert_true(args->xmp_client == 0x00000002 || args->xmp_client == 0x00000000);
      return XMPDeleteTitlePlaylist(playlist_handle);
    }
    case 0x0007001A: {
      // XMPSetPlaybackController
      assert_true(!buffer_length || buffer_length == 12);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> controller;
        rex::be<uint32_t> playback_client;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 12);

      assert_true((args->xmp_client == 0x00000002 && args->controller == 0x00000000) ||
                  (args->xmp_client == 0x00000000 && args->controller == 0x00000001));
      REXKRNL_DEBUG("XMPSetPlaybackController({:08X}, {:08X})", uint32_t(args->controller),
                    uint32_t(args->playback_client));

      REXKRNL_DEBUG("XMPSetPlaybackController: playback_client = {:08X}", uint32_t(args->playback_client));
      playback_client_ = PlaybackClient(uint32_t(args->playback_client));
      kernel_state_->BroadcastNotification(kMsgPlaybackControllerChanged, !args->playback_client);
      return X_E_SUCCESS;
    }
    case 0x0007001B: {
      // XMPGetPlaybackController
      assert_true(!buffer_length || buffer_length == 12);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> controller_ptr;
        rex::be<uint32_t> locked_ptr;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 12);

      assert_true(args->xmp_client == 0x00000002);
      REXKRNL_DEBUG("XMPGetPlaybackController({:08X}, {:08X}, {:08X})", uint32_t(args->xmp_client),
                    uint32_t(args->controller_ptr), uint32_t(args->locked_ptr));
      memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(args->controller_ptr), 0);
      memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(args->locked_ptr), 0);

      if (!XThread::GetCurrentThread()->main_thread()) {
        // Atrain spawns a thread 82437FD0 to call this in a tight loop forever.
        rex::thread::Sleep(std::chrono::milliseconds(10));
      }

      return X_E_SUCCESS;
    }
    case 0x00070029: {
      // XMPGetPlaybackBehavior
      assert_true(!buffer_length || buffer_length == 16);
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> playback_mode_ptr;
        rex::be<uint32_t> repeat_mode_ptr;
        rex::be<uint32_t> unk3_ptr;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 16);

      assert_true(args->xmp_client == 0x00000002 || args->xmp_client == 0x00000000);
      REXKRNL_DEBUG("XMPGetPlaybackBehavior({:08X}, {:08X}, {:08X})",
                    uint32_t(args->playback_mode_ptr), uint32_t(args->repeat_mode_ptr),
                    uint32_t(args->unk3_ptr));
      if (args->playback_mode_ptr) {
        memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(args->playback_mode_ptr),
                                         static_cast<uint32_t>(playback_mode_));
      }
      if (args->repeat_mode_ptr) {
        memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(args->repeat_mode_ptr),
                                         static_cast<uint32_t>(repeat_mode_));
      }
      if (args->unk3_ptr) {
        memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(args->unk3_ptr), unknown_flags_);
      }
      return X_E_SUCCESS;
    }
    case 0x0007002E: {
      assert_true(!buffer_length || buffer_length == 12);
      // Query of size for XamAlloc - the result of the alloc is passed to
      // 0x0007000D.
      struct {
        rex::be<uint32_t> xmp_client;
        rex::be<uint32_t> song_count;
        rex::be<uint32_t> size_ptr;
      }* args = memory_->TranslateVirtual<decltype(args)>(buffer_ptr);
      static_assert_size(decltype(*args), 12);

      assert_true(args->xmp_client == 0x00000002 || args->xmp_client == 0x00000000);
      // We don't use the storage, so just fudge the number.
      memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(args->size_ptr),
                                       4 + uint32_t(args->song_count) * 128);
      return X_E_SUCCESS;
    }
    case 0x0007003D: {
      // XMPCaptureOutput - not sure how this works :/
      REXKRNL_DEBUG("XMPCaptureOutput(...)");
      assert_always("XMP output not unimplemented");
      return X_E_FAIL;
    }
  }
  REXKRNL_ERROR(
      "Unimplemented XMP message app={:08X}, msg={:08X}, arg1={:08X}, "
      "arg2={:08X}",
      app_id(), message, buffer_ptr, buffer_length);
  return X_E_FAIL;
}

}  // namespace apps
}  // namespace xam
}  // namespace kernel
}  // namespace rex








