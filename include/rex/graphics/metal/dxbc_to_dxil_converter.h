#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct IDxbcConverter;

namespace rex::graphics::metal {

class DxbcToDxilConverter {
 public:
  DxbcToDxilConverter();
  ~DxbcToDxilConverter();

  bool Initialize();

  bool Convert(const std::vector<uint8_t>& dxbc_data,
               std::vector<uint8_t>& dxil_data_out,
               std::string* error_message = nullptr);

 private:
  IDxbcConverter* GetThreadConverter(std::string* error_message = nullptr);
  bool ConvertViaCommandLine(const std::vector<uint8_t>& dxbc_data,
                             std::vector<uint8_t>& dxil_data_out,
                             const std::string& shader_id,
                             std::string* error_message = nullptr);
  bool WriteFile(const std::string& path, const std::vector<uint8_t>& data);

  bool is_available_ = false;
  bool use_cli_fallback_ = false;
  std::string cli_fallback_path_;
  std::wstring extra_options_;
};

}  // namespace rex::graphics::metal
