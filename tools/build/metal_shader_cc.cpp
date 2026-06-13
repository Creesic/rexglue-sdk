// Build-time Metal shader compiler for rexglue built-in shaders.
//
// Usage: rex-metal-shader-cc [--depfile <path>] [--define <name=value>]
//                            [--identifier <name>] [--metal-debug]
//                            [--metal-sdk <sdk>] [--metal-std <std>]
//                            [--metal-min-version-flag <flag>]
//                            <input> <output.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <spawn.h>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace {

struct ShaderDefine {
  std::string name;
  std::string value;

  std::string argument() const { return name + "=" + value; }
};

bool IsCIdentifier(std::string_view value) {
  if (value.empty()) {
    return false;
  }
  auto is_alpha_or_underscore = [](unsigned char c) {
    return std::isalpha(c) || c == '_';
  };
  auto is_alnum_or_underscore = [](unsigned char c) {
    return std::isalnum(c) || c == '_';
  };
  if (!is_alpha_or_underscore(static_cast<unsigned char>(value.front()))) {
    return false;
  }
  for (char c : value.substr(1)) {
    if (!is_alnum_or_underscore(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
}

bool ParseDefine(std::string_view text, ShaderDefine* out) {
  size_t equals = text.find('=');
  if (equals == std::string_view::npos || equals == 0 ||
      equals + 1 == text.size()) {
    return false;
  }
  std::string_view name = text.substr(0, equals);
  std::string_view value = text.substr(equals + 1);
  if (!IsCIdentifier(name)) {
    return false;
  }
  for (char c : value) {
    if (c == '\n' || c == '\r') {
      return false;
    }
  }
  out->name.assign(name);
  out->value.assign(value);
  return true;
}

std::string IdentifierFromFilename(const std::string& filename) {
  auto last_dot = filename.rfind('.');
  std::string stem =
      last_dot == std::string::npos ? filename : filename.substr(0, last_dot);
  std::replace(stem.begin(), stem.end(), '.', '_');
  return stem;
}

void AppendDefines(std::vector<std::string>* args,
                   const std::vector<ShaderDefine>& defines) {
  for (const auto& define : defines) {
    args->push_back("-D");
    args->push_back(define.argument());
  }
}

int RunCommand(const std::vector<std::string>& args) {
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (const auto& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  pid_t pid = 0;
  int result = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
  if (result != 0) {
    std::fprintf(stderr, "failed to spawn %s: %s\n", argv[0], std::strerror(result));
    return result;
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    std::perror("waitpid");
    return 1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return 1;
}

int RunCommandCapture(const std::vector<std::string>& args, std::string* out) {
  int stdout_pipe[2];
  if (pipe(stdout_pipe) != 0) {
    std::perror("pipe");
    return 1;
  }

  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
  posix_spawn_file_actions_addclose(&actions, stdout_pipe[1]);

  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (const auto& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  pid_t pid = 0;
  int result =
      posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
  posix_spawn_file_actions_destroy(&actions);
  close(stdout_pipe[1]);
  if (result != 0) {
    close(stdout_pipe[0]);
    std::fprintf(stderr, "failed to spawn %s: %s\n", argv[0],
                 std::strerror(result));
    return result;
  }

  out->clear();
  char buffer[4096];
  for (;;) {
    ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer));
    if (bytes_read > 0) {
      out->append(buffer, static_cast<size_t>(bytes_read));
      continue;
    }
    if (bytes_read == 0) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    std::perror("read");
    close(stdout_pipe[0]);
    return 1;
  }
  close(stdout_pipe[0]);

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    std::perror("waitpid");
    return 1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return 1;
}

std::string TrimWhitespace(std::string value) {
  auto is_space = [](unsigned char c) { return std::isspace(c); };
  while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  size_t start = 0;
  while (start < value.size() &&
         is_space(static_cast<unsigned char>(value[start]))) {
    ++start;
  }
  if (start != 0) {
    value.erase(0, start);
  }
  return value;
}

std::string ResolveXcrunTool(const std::string& sdk, const char* tool) {
  std::string path;
  int result = RunCommandCapture({"xcrun", "-sdk", sdk, "-find", tool}, &path);
  if (result != 0) {
    return {};
  }
  return TrimWhitespace(std::move(path));
}

bool ReadBinaryFile(const std::filesystem::path& path,
                    std::vector<uint8_t>* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "failed to open %s\n", path.string().c_str());
    return false;
  }
  in.seekg(0, std::ios::end);
  std::streamoff size = in.tellg();
  in.seekg(0, std::ios::beg);
  if (size < 0) {
    return false;
  }
  out->resize(static_cast<size_t>(size));
  if (size > 0) {
    in.read(reinterpret_cast<char*>(out->data()), size);
  }
  return bool(in) || in.eof();
}

bool WriteMetallibHeader(const std::filesystem::path& path,
                         const std::string& identifier,
                         const std::vector<uint8_t>& bytes) {
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    std::fprintf(stderr, "failed to create %s: %s\n",
                 path.parent_path().string().c_str(), ec.message().c_str());
    return false;
  }

  std::ofstream out(path, std::ios::binary);
  if (!out) {
    std::fprintf(stderr, "failed to write %s\n", path.string().c_str());
    return false;
  }

  out << "#pragma once\n\n#include <cstdint>\n\n";
  out << "const uint8_t " << identifier << "_metallib[] = {";
  for (size_t i = 0; i < bytes.size(); ++i) {
    if ((i % 12) == 0) {
      out << "\n  ";
    }
    char value[8];
    std::snprintf(value, sizeof(value), "0x%02X,", bytes[i]);
    out << value;
    if ((i % 12) != 11) {
      out << ' ';
    }
  }
  out << "\n};\n";
  return bool(out);
}

}  // namespace

int main(int argc, char** argv) {
  std::string metal_sdk = "macosx";
  std::string metal_std = "macos-metal2.4";
  std::string metal_min_version_flag = "-mmacosx-version-min=15.0";
  std::string depfile_path;
  std::string identifier_override;
  std::vector<ShaderDefine> defines;
  bool metal_debug = false;

  int arg_idx = 1;
  while (arg_idx < argc && argv[arg_idx][0] == '-') {
    if (std::strcmp(argv[arg_idx], "--depfile") == 0) {
      if (arg_idx + 1 >= argc) {
        std::fprintf(stderr, "--depfile requires a path\n");
        return 1;
      }
      depfile_path = argv[arg_idx + 1];
      arg_idx += 2;
    } else if (std::strcmp(argv[arg_idx], "--define") == 0) {
      if (arg_idx + 1 >= argc) {
        std::fprintf(stderr, "--define requires NAME=VALUE\n");
        return 1;
      }
      ShaderDefine define;
      if (!ParseDefine(argv[arg_idx + 1], &define)) {
        std::fprintf(stderr, "invalid --define value: %s\n", argv[arg_idx + 1]);
        return 1;
      }
      defines.push_back(std::move(define));
      arg_idx += 2;
    } else if (std::strcmp(argv[arg_idx], "--identifier") == 0) {
      if (arg_idx + 1 >= argc) {
        std::fprintf(stderr, "--identifier requires a name\n");
        return 1;
      }
      if (!IsCIdentifier(argv[arg_idx + 1])) {
        std::fprintf(stderr, "invalid --identifier value: %s\n", argv[arg_idx + 1]);
        return 1;
      }
      identifier_override = argv[arg_idx + 1];
      arg_idx += 2;
    } else if (std::strcmp(argv[arg_idx], "--metal-debug") == 0) {
      metal_debug = true;
      ++arg_idx;
    } else if (std::strcmp(argv[arg_idx], "--metal-sdk") == 0) {
      if (arg_idx + 1 >= argc) {
        std::fprintf(stderr, "--metal-sdk requires an SDK name\n");
        return 1;
      }
      metal_sdk = argv[arg_idx + 1];
      arg_idx += 2;
    } else if (std::strcmp(argv[arg_idx], "--metal-std") == 0) {
      if (arg_idx + 1 >= argc) {
        std::fprintf(stderr, "--metal-std requires a standard name\n");
        return 1;
      }
      metal_std = argv[arg_idx + 1];
      arg_idx += 2;
    } else if (std::strcmp(argv[arg_idx], "--metal-min-version-flag") == 0) {
      if (arg_idx + 1 >= argc) {
        std::fprintf(stderr, "--metal-min-version-flag requires a compiler flag\n");
        return 1;
      }
      metal_min_version_flag = argv[arg_idx + 1];
      arg_idx += 2;
    } else {
      std::fprintf(stderr, "unknown flag: %s\n", argv[arg_idx]);
      return 1;
    }
  }

  if (argc - arg_idx != 2) {
    std::fprintf(stderr, "Usage: %s [options] <input> <output.h>\n", argv[0]);
    return 1;
  }

  std::filesystem::path input_path = argv[arg_idx];
  std::filesystem::path output_path = argv[arg_idx + 1];
  std::string identifier = identifier_override.empty()
                               ? IdentifierFromFilename(input_path.filename().string())
                               : identifier_override;

  auto tmp_dir = std::filesystem::temp_directory_path();
  auto tag = identifier + "_" + std::to_string(::getpid());
  auto air_path = tmp_dir / (tag + ".air");
  auto lib_path = tmp_dir / (tag + ".metallib");

  auto cleanup = [&] {
    std::error_code ec;
    std::filesystem::remove(air_path, ec);
    std::filesystem::remove(lib_path, ec);
  };

  std::string metal_tool = ResolveXcrunTool(metal_sdk, "metal");
  if (metal_tool.empty()) {
    std::fprintf(stderr, "failed to locate metal with xcrun -sdk %s -find metal\n",
                 metal_sdk.c_str());
    return 1;
  }
  std::string metallib_tool = ResolveXcrunTool(metal_sdk, "metallib");
  if (metallib_tool.empty()) {
    std::fprintf(stderr,
                 "failed to locate metallib with xcrun -sdk %s -find metallib\n",
                 metal_sdk.c_str());
    return 1;
  }

  std::vector<std::string> metal_cmd = {
      metal_tool, "-x", "metal",
      "-std=" + metal_std, metal_min_version_flag,
      "-D", "SHADING_LANGUAGE_MSL_XE=1", "-w"};
  if (metal_debug) {
    metal_cmd.push_back("-frecord-sources");
    metal_cmd.push_back("-gline-tables-only");
  }
  AppendDefines(&metal_cmd, defines);
  std::string input_dir = input_path.parent_path().string();
  if (!input_dir.empty()) {
    metal_cmd.push_back("-I");
    metal_cmd.push_back(input_dir);
  }
  if (!depfile_path.empty()) {
    metal_cmd.push_back("-MD");
    metal_cmd.push_back("-MT");
    metal_cmd.push_back(output_path.string());
    metal_cmd.push_back("-MF");
    metal_cmd.push_back(depfile_path);
  }
  metal_cmd.push_back("-c");
  metal_cmd.push_back(input_path.string());
  metal_cmd.push_back("-o");
  metal_cmd.push_back(air_path.string());
  if (RunCommand(metal_cmd) != 0) {
    std::fprintf(stderr, "metal failed for %s\n", input_path.string().c_str());
    cleanup();
    return 1;
  }

  if (RunCommand({metallib_tool, air_path.string(), "-o", lib_path.string()}) != 0) {
    std::fprintf(stderr, "metallib failed for %s\n", input_path.string().c_str());
    cleanup();
    return 1;
  }

  std::vector<uint8_t> bytes;
  if (!ReadBinaryFile(lib_path, &bytes)) {
    cleanup();
    return 1;
  }
  cleanup();

  return WriteMetallibHeader(output_path, identifier, bytes) ? 0 : 1;
}
