/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2014 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/filesystem/devices/stfs_container_entry.h>
#include <rex/filesystem/devices/stfs_container_file.h>

#include <algorithm>
#include <cmath>

#include <rex/math.h>

namespace rex::filesystem {

StfsContainerFile::StfsContainerFile(uint32_t file_access, StfsContainerEntry* entry)
    : File(file_access, entry), entry_(entry) {}

StfsContainerFile::~StfsContainerFile() = default;

void StfsContainerFile::Destroy() {
  delete this;
}

X_STATUS StfsContainerFile::ReadSync(std::span<uint8_t> buffer, size_t byte_offset,
                                     size_t* out_bytes_read) {
  if (byte_offset >= entry_->size()) {
    return X_STATUS_END_OF_FILE;
  }

  const auto& blocks = entry_->block_list();

  // Sequential reads are common during loads. Reuse the previous block as a
  // starting point to avoid rescanning from block 0 each call.
  size_t start_index = 0;
  size_t src_offset = 0;
  if (has_last_block_ && last_block_index_ < blocks.size()) {
    const auto& last_block = blocks[last_block_index_];
    const size_t last_block_end = last_block_src_offset_ + last_block.length;
    if (byte_offset >= last_block_src_offset_) {
      start_index = last_block_index_;
      src_offset = last_block_src_offset_;
      if (byte_offset >= last_block_end) {
        start_index = last_block_index_ + 1;
        src_offset = last_block_end;
      }
    }
  }

  uint8_t* p = buffer.data();
  size_t remaining_length = std::min(buffer.size(), entry_->size() - byte_offset);

  *out_bytes_read = 0;
  for (size_t i = start_index; i < blocks.size(); i++) {
    const auto& record = blocks[i];
    if (src_offset + record.length <= byte_offset) {
      // Doesn't begin in this region. Skip it.
      src_offset += record.length;
      continue;
    }
    has_last_block_ = true;
    last_block_index_ = i;
    last_block_src_offset_ = src_offset;

    size_t read_offset = (byte_offset > src_offset) ? byte_offset - src_offset : 0;
    size_t read_length = std::min(record.length - read_offset, remaining_length);

    auto& file = entry_->files()->at(record.file);
    rex::filesystem::Seek(file, record.offset + read_offset, SEEK_SET);
    auto num_read = fread(p, 1, read_length, file);

    *out_bytes_read += num_read;
    p += num_read;
    src_offset += record.length;
    remaining_length -= read_length;
    if (remaining_length == 0) {
      break;
    }
  }

  return X_STATUS_SUCCESS;
}

}  // namespace rex::filesystem
