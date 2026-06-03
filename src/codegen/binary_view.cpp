/**
 * @file        codegen/binary_view.cpp
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <limits>
#include <fstream>
#include <optional>

#include "../../thirdparty/crypto/TinySHA1.hpp"
#include "../../thirdparty/pe/pe_image.h"
#include <fmt/format.h>
#include <rex/codegen/binary_view.h>
#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/string.h>
#include <rex/system/lzx.h>
#include <rex/system/util/xex2_info.h>
#include <rex/types.h>

#include "codegen_logging.h"
#include <rex/system/binary_types.h>
#include <rex/system/module.h>
#include "../../thirdparty/crypto/rijndael-alg-fst.h"

namespace rex::codegen {

namespace {

constexpr uint8_t kXex2RetailKey[16] = {0x20, 0xB1, 0x85, 0xA5, 0x9D, 0x28, 0xFD, 0xC3,
                                        0x40, 0x58, 0x3F, 0xBB, 0x08, 0x96, 0xBF, 0x91};
constexpr uint8_t kXex2DevkitKey[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct SecurityInfoView {
  const char* aesKey = nullptr;
  uint32_t imageFlags = 0;
  uint32_t loadAddress = 0;
  uint32_t exportTable = 0;
  uint32_t pageDescriptorCount = 0;
  const xex2_page_descriptor* pageDescriptors = nullptr;
};

struct LoadedXexImage {
  uint32_t baseAddress = 0;
  uint32_t imageSize = 0;
  uint32_t entryPoint = 0;
  uint32_t exceptionDirectoryAddr = 0;
  uint32_t exceptionDirectorySize = 0;
  uint32_t exportTableAddr = 0;
  uint32_t importThunkTableStart = 0;
  uint32_t importExportRangeEnd = 0;
  std::vector<std::string> sectionNames;
  std::vector<std::vector<uint8_t>> sectionData;
  std::vector<SectionView> sections;
  std::vector<ImportSymbol> importSymbols;
  BinaryFileInfo info;
};

bool GetOptHeader(const xex2_header* header, xex2_header_keys key, void** out_ptr) {
  for (uint32_t i = 0; i < header->header_count; i++) {
    const xex2_opt_header& opt_header = header->headers[i];
    if (opt_header.key == key) {
      switch (key & 0xFF) {
        case 0x00:
          *reinterpret_cast<uint32_t*>(out_ptr) = static_cast<uint32_t>(opt_header.value);
          break;
        case 0x01:
          *out_ptr = const_cast<void*>(reinterpret_cast<const void*>(&opt_header.value));
          break;
        default:
          *out_ptr = reinterpret_cast<void*>(uintptr_t(header) + opt_header.offset);
          break;
      }
      return true;
    }
  }
  return false;
}

template <typename T>
bool GetOptHeader(const xex2_header* header, xex2_header_keys key, T* out_ptr) {
  return GetOptHeader(header, key, reinterpret_cast<void**>(out_ptr));
}

SecurityInfoView ReadSecurityInfo(const xex2_header* header) {
  SecurityInfoView info{};
  const uint32_t magic = header->magic;
  const void* sec_ptr = reinterpret_cast<const void*>(uintptr_t(header) + header->security_offset);
  if (magic == rex::memory::make_fourcc("XEX1")) {
    const auto* sec = reinterpret_cast<const xex1_security_info*>(sec_ptr);
    info.aesKey = sec->aes_key;
    info.imageFlags = sec->image_flags;
    info.loadAddress = sec->load_address;
    info.exportTable = sec->export_table;
    info.pageDescriptorCount = sec->page_descriptor_count;
    info.pageDescriptors = sec->page_descriptors;
  } else {
    const auto* sec = reinterpret_cast<const xex2_security_info*>(sec_ptr);
    info.aesKey = sec->aes_key;
    info.imageFlags = sec->image_flags;
    info.loadAddress = sec->load_address;
    info.exportTable = sec->export_table;
    info.pageDescriptorCount = sec->page_descriptor_count;
    info.pageDescriptors = sec->page_descriptors;
  }
  return info;
}

void AesDecryptBuffer(const uint8_t* session_key, const uint8_t* input_buffer, size_t input_size,
                      uint8_t* output_buffer, size_t output_size) {
  uint32_t rk[4 * (MAXNR + 1)];
  uint8_t ivec[16] = {0};
  int32_t Nr = rijndaelKeySetupDec(rk, session_key, 128);
  const uint8_t* ct = input_buffer;
  uint8_t* pt = output_buffer;
  for (size_t n = 0; n < input_size && n < output_size; n += 16, ct += 16, pt += 16) {
    rijndaelDecrypt(rk, Nr, ct, pt);
    for (size_t i = 0; i < 16; i++) {
      pt[i] ^= ivec[i];
      ivec[i] = ct[i];
    }
  }
}

uint32_t GetImagePageSize(uint32_t image_flags) {
  return (image_flags & rex::XEX_IMAGE_PAGE_SIZE_4KB) ? 4096u : 64u * 1024u;
}

uint32_t ComputeImageSize(const SecurityInfoView& security_info) {
  uint32_t total_size = 0;
  const uint32_t page_size = GetImagePageSize(security_info.imageFlags);
  for (uint32_t i = 0; i < security_info.pageDescriptorCount; i++) {
    rex::xex2_page_descriptor desc;
    desc.value = rex::byte_swap(security_info.pageDescriptors[i].value);
    total_size += desc.page_count * page_size;
  }
  return total_size;
}

rex::Result<std::vector<uint8_t>> DecompressXexImage(const xex2_header* header,
                                                     std::span<const uint8_t> file_data) {
  const auto security_info = ReadSecurityInfo(header);
  const uint32_t header_size = header->header_size;
  if (file_data.size() < header_size) {
    return rex::Err<std::vector<uint8_t>>(rex::ErrorCategory::Format, "Truncated XEX header");
  }
  const auto* file_info = [&]() -> const xex2_opt_file_format_info* {
    xex2_opt_file_format_info* value = nullptr;
    GetOptHeader(header, XEX_HEADER_FILE_FORMAT_INFO, &value);
    return value;
  }();
  if (!file_info) {
    return rex::Err<std::vector<uint8_t>>(rex::ErrorCategory::Format,
                                          "Missing XEX file format info");
  }

  const uint32_t image_size = ComputeImageSize(security_info);
  std::vector<uint8_t> image_data(image_size);

  const uint8_t* input_base = file_data.data() + header_size;
  const size_t input_size = file_data.size() - header_size;

  uint8_t session_key[16];
  auto try_load = [&](const uint8_t* base_key) -> rex::Result<bool> {
    AesDecryptBuffer(base_key, reinterpret_cast<const uint8_t*>(security_info.aesKey), 16,
                     session_key, 16);

    switch (file_info->compression_type) {
      case rex::XEX_COMPRESSION_NONE: {
        if (input_size > image_data.size()) {
          return rex::Err<bool>(rex::ErrorCategory::Format,
                                "Uncompressed XEX payload exceeds image size");
        }
        switch (file_info->encryption_type) {
          case rex::XEX_ENCRYPTION_NONE:
            std::copy_n(input_base, input_size, image_data.data());
            break;
          case rex::XEX_ENCRYPTION_NORMAL:
            AesDecryptBuffer(session_key, input_base, input_size, image_data.data(),
                             image_data.size());
            break;
          default:
            return rex::Err<bool>(rex::ErrorCategory::Crypto,
                                  "Unsupported XEX encryption type");
        }
        return true;
      }
      case rex::XEX_COMPRESSION_BASIC: {
        std::vector<uint8_t> decoded(input_size);
        const uint8_t* source = input_base;
        if (file_info->encryption_type == rex::XEX_ENCRYPTION_NORMAL) {
          AesDecryptBuffer(session_key, input_base, input_size, decoded.data(), decoded.size());
          source = decoded.data();
        } else if (file_info->encryption_type != rex::XEX_ENCRYPTION_NONE) {
          return rex::Err<bool>(rex::ErrorCategory::Crypto,
                                "Unsupported XEX encryption type");
        }

        auto& comp_info = file_info->compression_info.basic;
        uint32_t block_count = (file_info->info_size - 8) / 8;
        const uint8_t* p = source;
        uint8_t* d = image_data.data();
        const uint8_t* image_end = image_data.data() + image_data.size();
        for (uint32_t n = 0; n < block_count; n++) {
          const uint32_t data_size = comp_info.blocks[n].data_size;
          const uint32_t zero_size = comp_info.blocks[n].zero_size;
          if (d + data_size + zero_size > image_end || p + data_size > source + input_size) {
            return rex::Err<bool>(rex::ErrorCategory::Format,
                                  "Basic-compressed XEX block exceeds image bounds");
          }
          std::memcpy(d, p, data_size);
          p += data_size;
          d += data_size + zero_size;
        }
        return true;
      }
      case rex::XEX_COMPRESSION_NORMAL: {
        std::vector<uint8_t> decoded_input(input_size);
        const uint8_t* source = input_base;
        if (file_info->encryption_type == rex::XEX_ENCRYPTION_NORMAL) {
          AesDecryptBuffer(session_key, input_base, input_size, decoded_input.data(),
                           decoded_input.size());
          source = decoded_input.data();
        } else if (file_info->encryption_type != rex::XEX_ENCRYPTION_NONE) {
          return rex::Err<bool>(rex::ErrorCategory::Crypto,
                                "Unsupported XEX encryption type");
        }

        std::vector<uint8_t> compressed_bytes(input_size);
        const auto* compression_info = &file_info->compression_info;
        const rex::xex2_compressed_block_info* cur_block = &compression_info->normal.first_block;
        const uint8_t* p = source;
        uint8_t* d = compressed_bytes.data();
        sha1::SHA1 s;
        uint8_t digest[0x14];
        while (cur_block->block_size) {
          const size_t block_size = cur_block->block_size;
          if (p + block_size > source + input_size) {
            return rex::Err<bool>(rex::ErrorCategory::Format,
                                  "Normal-compressed XEX block exceeds file bounds");
          }
          const auto* next_block = reinterpret_cast<const rex::xex2_compressed_block_info*>(p);
          s.reset();
          s.processBytes(p, block_size);
          s.finalize(digest);
          if (std::memcmp(digest, cur_block->block_hash, sizeof(digest)) != 0) {
            return rex::Err<bool>(rex::ErrorCategory::Crypto,
                                  "Normal-compressed XEX block hash mismatch");
          }
          const uint8_t* pnext = p + block_size;
          p += 24;
          while (true) {
            if (p + 2 > pnext) {
              return rex::Err<bool>(rex::ErrorCategory::Format,
                                    "Normal-compressed XEX chunk header exceeds block");
            }
            const size_t chunk_size = (size_t(p[0]) << 8) | p[1];
            p += 2;
            if (!chunk_size) {
              break;
            }
            if (p + chunk_size > pnext || d + chunk_size > compressed_bytes.data() + compressed_bytes.size()) {
              return rex::Err<bool>(rex::ErrorCategory::Format,
                                    "Normal-compressed XEX chunk exceeds bounds");
            }
            std::memcpy(d, p, chunk_size);
            p += chunk_size;
            d += chunk_size;
          }
          p = pnext;
          cur_block = next_block;
        }
        if (lzx_decompress(compressed_bytes.data(), size_t(d - compressed_bytes.data()),
                           image_data.data(), image_data.size(),
                           compression_info->normal.window_size, nullptr, 0) != 0) {
          return rex::Err<bool>(rex::ErrorCategory::Compression,
                                "LZX decompression failed");
        }
        return true;
      }
      default:
        return rex::Err<bool>(rex::ErrorCategory::Compression,
                              "Unsupported XEX compression type");
    }
  };

  auto retail_status = try_load(kXex2RetailKey);
  if (retail_status) {
    const auto* doshdr = reinterpret_cast<const IMAGE_DOS_HEADER*>(image_data.data());
    if (doshdr->e_magic == IMAGE_DOS_SIGNATURE) {
      return image_data;
    }
  } else {
    REXCODEGEN_DEBUG("Retail XEX decrypt/decompress failed: {}", retail_status.error().what());
  }

  auto devkit_status = try_load(kXex2DevkitKey);
  if (!devkit_status) {
    return rex::Err<std::vector<uint8_t>>(devkit_status.error());
  }
  const auto* doshdr = reinterpret_cast<const IMAGE_DOS_HEADER*>(image_data.data());
  if (doshdr->e_magic != IMAGE_DOS_SIGNATURE) {
    return rex::Err<std::vector<uint8_t>>(rex::ErrorCategory::Format,
                                          "Decoded XEX image does not contain a valid PE header");
  }
  return image_data;
}

rex::Result<LoadedXexImage> LoadXexFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::IO,
                                    fmt::format("Unable to open XEX '{}'", path.string()));
  }
  file.seekg(0, std::ios::end);
  const std::streamsize file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  if (file_size <= 0) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::IO,
                                    fmt::format("Empty XEX '{}'", path.string()));
  }

  std::vector<uint8_t> file_data((size_t(file_size)));
  if (!file.read(reinterpret_cast<char*>(file_data.data()), file_size)) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::IO,
                                    fmt::format("Failed to read XEX '{}'", path.string()));
  }

  if (file_data.size() < sizeof(xex2_header)) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format,
                                    fmt::format("XEX '{}' is too small", path.string()));
  }

  const auto* header = reinterpret_cast<const xex2_header*>(file_data.data());
  const uint32_t magic = header->magic;
  if (magic != rex::memory::make_fourcc("XEX1") && magic != rex::memory::make_fourcc("XEX2")) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format,
                                    fmt::format("'{}' is not an XEX image", path.string()));
  }

  auto image_data = TRY(DecompressXexImage(header, file_data));
  const auto security_info = ReadSecurityInfo(header);

  uint32_t base_address = security_info.loadAddress;
  rex::be<uint32_t>* base_addr_opt = nullptr;
  if (GetOptHeader(header, rex::XEX_HEADER_IMAGE_BASE_ADDRESS, &base_addr_opt)) {
    base_address = *base_addr_opt;
  }

  if (image_data.size() < sizeof(IMAGE_DOS_HEADER)) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format, "Decoded XEX image is truncated");
  }
  const auto* doshdr = reinterpret_cast<const IMAGE_DOS_HEADER*>(image_data.data());
  if (doshdr->e_magic != IMAGE_DOS_SIGNATURE) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format,
                                    "Decoded XEX image has invalid DOS signature");
  }
  if (doshdr->e_lfanew < 0 || size_t(doshdr->e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > image_data.size()) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format,
                                    "Decoded XEX image has invalid PE header offset");
  }

  const auto* nthdr =
      reinterpret_cast<const IMAGE_NT_HEADERS32*>(image_data.data() + doshdr->e_lfanew);
  if (nthdr->Signature != IMAGE_NT_SIGNATURE) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format,
                                    "Decoded XEX image has invalid PE signature");
  }
  const auto* filehdr = &nthdr->FileHeader;
  const auto* opthdr = &nthdr->OptionalHeader;
  if (filehdr->Machine != IMAGE_FILE_MACHINE_POWERPCBE ||
      !(filehdr->Characteristics & IMAGE_FILE_32BIT_MACHINE) ||
      filehdr->SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER ||
      opthdr->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
      opthdr->Subsystem != IMAGE_SUBSYSTEM_XBOX) {
    return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format,
                                    "Decoded XEX image is not a valid Xbox 360 PE");
  }

  LoadedXexImage loaded{};
  loaded.baseAddress = base_address;
  loaded.imageSize = ComputeImageSize(security_info);
  loaded.entryPoint = base_address + opthdr->AddressOfEntryPoint;
  loaded.exceptionDirectoryAddr =
      base_address + opthdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
  loaded.exceptionDirectorySize = opthdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
  loaded.exportTableAddr = security_info.exportTable;

  const auto* sechdr = IMAGE_FIRST_SECTION(const_cast<IMAGE_NT_HEADERS32*>(nthdr));
  loaded.sectionNames.reserve(filehdr->NumberOfSections);
  loaded.sectionData.reserve(filehdr->NumberOfSections);
  loaded.sections.reserve(filehdr->NumberOfSections);
  for (size_t n = 0; n < filehdr->NumberOfSections; n++, sechdr++) {
    const uint32_t virtual_address = base_address + sechdr->VirtualAddress;
    const uint32_t virtual_size = sechdr->Misc.VirtualSize;
    if (virtual_size == 0) {
      continue;
    }
    if (sechdr->VirtualAddress + virtual_size > image_data.size()) {
      REXCODEGEN_WARN("Standalone XEX loader: skipping section '{}' beyond image bounds",
                      std::string(reinterpret_cast<const char*>(sechdr->Name),
                                  strnlen(reinterpret_cast<const char*>(sechdr->Name), 8)));
      continue;
    }

    loaded.sectionNames.push_back(
        std::string(reinterpret_cast<const char*>(sechdr->Name),
                    strnlen(reinterpret_cast<const char*>(sechdr->Name), 8)));
    loaded.sectionData.emplace_back(image_data.begin() + sechdr->VirtualAddress,
                                    image_data.begin() + sechdr->VirtualAddress + virtual_size);
    loaded.sections.push_back(SectionView{
        .name = loaded.sectionNames.back(),
        .baseAddress = virtual_address,
        .size = virtual_size,
        .data = loaded.sectionData.back().data(),
        .executable = (sechdr->Characteristics & kXEPESectionMemoryExecute) != 0,
    });
  }

  xex2_opt_execution_info* exec_info = nullptr;
  BinaryFileInfo info{};
  info.peTimeDateStamp = filehdr->TimeDateStamp;
  if (GetOptHeader(header, rex::XEX_HEADER_EXECUTION_INFO, &exec_info) && exec_info) {
    info.hasExecutionInfo = true;
    info.titleId = exec_info->title_id;
    info.mediaId = exec_info->media_id;
    auto version = exec_info->version();
    info.versionMajor = version.major;
    info.versionMinor = version.minor;
    info.versionBuild = version.build;
    info.versionQfe = version.qfe;
  }

  xex2_opt_import_libraries* opt_import_libraries = nullptr;
  if (GetOptHeader(header, rex::XEX_HEADER_IMPORT_LIBRARIES, &opt_import_libraries) &&
      opt_import_libraries) {
    const char* string_table[32] = {};
    for (size_t i = 0, o = 0;
         i < opt_import_libraries->string_table.size && o < opt_import_libraries->string_table.count;
         ++o) {
      const char* str = &opt_import_libraries->string_table.data[i];
      string_table[o] = str;
      i += std::strlen(str) + 1;
      if ((i % 4) != 0) {
        i += 4 - (i % 4);
      }
    }

    auto library_data = reinterpret_cast<const uint8_t*>(opt_import_libraries);
    uint32_t library_offset = opt_import_libraries->string_table.size + 12;
    uint32_t min_import_addr = std::numeric_limits<uint32_t>::max();
    while (library_offset < opt_import_libraries->size) {
      auto library = reinterpret_cast<const xex2_import_library*>(library_data + library_offset);
      if (!library->size) {
        break;
      }
      const size_t library_name_index = library->name_index & 0xFF;
      if (library_name_index >= opt_import_libraries->string_table.count ||
          !string_table[library_name_index]) {
        return rex::Err<LoadedXexImage>(rex::ErrorCategory::Format,
                                        "Invalid XEX import library string table index");
      }
      const std::string library_name =
          rex::string::utf8_find_base_name_from_guest_path(string_table[library_name_index]);
      for (uint32_t i = 0; i < library->count; i++) {
        uint32_t record_addr = library->import_table[i];
        if (!record_addr || record_addr < base_address) {
          continue;
        }
        const uint32_t offset = record_addr - base_address;
        if (offset + sizeof(rex::be<uint32_t>) > image_data.size()) {
          continue;
        }
        auto record_value =
            *reinterpret_cast<const rex::be<uint32_t>*>(image_data.data() + offset);
        uint16_t record_type = (record_value & 0xFF000000) >> 24;
        uint16_t ordinal = record_value & 0xFFFF;
        if (record_type == 1) {
          loaded.importSymbols.push_back(
              ImportSymbol{.address = record_addr, .name = fmt::format("{}@{}", library_name, ordinal)});
          min_import_addr = std::min(min_import_addr, record_addr);
        }
      }
      library_offset += library->size;
    }
    if (min_import_addr != std::numeric_limits<uint32_t>::max()) {
      loaded.importThunkTableStart = min_import_addr;
      for (const auto& section : loaded.sections) {
        if (min_import_addr >= section.baseAddress && min_import_addr < section.end()) {
          loaded.importExportRangeEnd = section.end();
          break;
        }
      }
    }
  }

  loaded.info = info;
  return loaded;
}

}  // namespace

BinaryView BinaryView::fromModule(const runtime::Module& module) {
  BinaryView view;

  // Copy metadata
  view.baseAddress_ = module.base_address();
  view.imageSize_ = module.image_size();
  view.entryPoint_ = module.entry_point();
  view.exceptionDirectoryAddr_ = module.exception_directory_address();
  view.exceptionDirectorySize_ = module.exception_directory_size();
  view.exportTableAddr_ = module.export_table_address();

  // Copy import symbols and calculate import thunk table start
  // The import thunk table (and export table after it) extends to end of .text
  uint32_t minImportAddr = std::numeric_limits<uint32_t>::max();
  for (const auto& sym : module.binary_symbols()) {
    if (sym.type == runtime::BinarySymbolType::Import) {
      // Copy symbol for Register phase to use
      view.importSymbols_.push_back(ImportSymbol{.address = sym.address, .name = sym.name});

      // Track minimum address for thunk table range
      if (sym.address != 0 && sym.address < minImportAddr) {
        minImportAddr = sym.address;
      }
    }
  }
  if (minImportAddr != std::numeric_limits<uint32_t>::max()) {
    view.importThunkTableStart_ = minImportAddr;
    REXCODEGEN_DEBUG("BinaryView: import thunk table starts at 0x{:08X}", minImportAddr);

    // Find section containing the import thunk table to determine end of import/export range
    for (const auto& section : module.binary_sections()) {
      uint32_t sectionEnd = section.virtual_address + section.virtual_size;
      if (minImportAddr >= section.virtual_address && minImportAddr < sectionEnd) {
        view.importExportRangeEnd_ = sectionEnd;
        REXCODEGEN_DEBUG("BinaryView: import/export range ends at 0x{:08X} (end of {} section)",
                         view.importExportRangeEnd_, section.name);
        break;
      }
    }
  }
  REXCODEGEN_DEBUG("BinaryView: copied {} import symbols", view.importSymbols_.size());

  // Copy section data
  const auto& binarySections = module.binary_sections();
  view.sectionNames_.reserve(binarySections.size());
  view.sectionData_.reserve(binarySections.size());
  view.sections_.reserve(binarySections.size());

  uint32_t imageEnd = view.baseAddress_ + view.imageSize_;
  for (const auto& section : binarySections) {
    // Skip sections not mapped into memory
    if (!section.host_data || section.virtual_size == 0) {
      REXCODEGEN_DEBUG("BinaryView: skipping unmapped section '{}'", section.name);
      continue;
    }
    // Skip sections that extend beyond the loaded image
    if (section.virtual_address + section.virtual_size > imageEnd) {
      REXCODEGEN_DEBUG("BinaryView: skipping section '{}' (extends past image end 0x{:08X})",
                       section.name, imageEnd);
      continue;
    }

    // Copy name
    view.sectionNames_.push_back(std::string(section.name));

    // Copy bytes
    view.sectionData_.emplace_back(section.host_data, section.host_data + section.virtual_size);

    // Create view pointing into our owned data
    view.sections_.push_back(SectionView{.name = view.sectionNames_.back(),
                                         .baseAddress = section.virtual_address,
                                         .size = section.virtual_size,
                                         .data = view.sectionData_.back().data(),
                                         .executable = section.executable});

    REXCODEGEN_DEBUG("BinaryView: section '{}' at 0x{:08X} size 0x{:X} exec={}", section.name,
                     section.virtual_address, section.virtual_size, section.executable);
  }

  REXCODEGEN_DEBUG("BinaryView: loaded {} sections, base=0x{:08X}, size=0x{:X}",
                   view.sections_.size(), view.baseAddress_, view.imageSize_);

  return view;
}

rex::Result<BinaryView> BinaryView::fromXexFile(const std::filesystem::path& path,
                                                BinaryFileInfo* info) {
  auto loaded = TRY(LoadXexFile(path));
  BinaryView view;
  view.baseAddress_ = loaded.baseAddress;
  view.imageSize_ = loaded.imageSize;
  view.entryPoint_ = loaded.entryPoint;
  view.exceptionDirectoryAddr_ = loaded.exceptionDirectoryAddr;
  view.exceptionDirectorySize_ = loaded.exceptionDirectorySize;
  view.exportTableAddr_ = loaded.exportTableAddr;
  view.importThunkTableStart_ = loaded.importThunkTableStart;
  view.importExportRangeEnd_ = loaded.importExportRangeEnd;
  view.sectionNames_ = std::move(loaded.sectionNames);
  view.sectionData_ = std::move(loaded.sectionData);
  view.sections_ = std::move(loaded.sections);
  view.importSymbols_ = std::move(loaded.importSymbols);
  if (info) {
    *info = loaded.info;
  }
  return view;
}

const uint8_t* BinaryView::translate(uint32_t addr) const {
  for (const auto& section : sections_) {
    if (auto* ptr = section.translate(addr)) {
      return ptr;
    }
  }
  return nullptr;
}

bool BinaryView::isExecutable(uint32_t addr) const {
  if (auto* section = findSection(addr)) {
    return section->executable;
  }
  return false;
}

const SectionView* BinaryView::findSection(uint32_t addr) const {
  for (const auto& section : sections_) {
    if (section.contains(addr)) {
      return &section;
    }
  }
  return nullptr;
}

const SectionView* BinaryView::findSectionByName(std::string_view name) const {
  for (const auto& section : sections_) {
    if (section.name == name) {
      return &section;
    }
  }
  return nullptr;
}

}  // namespace rex::codegen
