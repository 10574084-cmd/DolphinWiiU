#include "ui/fs_util.h"

#include <sys/stat.h>

#include <cstdio>
#include <utility>

namespace dwu {

bool ReadTextFile(const std::string& path, std::string& out) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::string data;
  char buf[1024];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) data.append(buf, n);
  const bool ok = std::ferror(f) == 0;
  std::fclose(f);
  if (ok) out = std::move(data);
  return ok;
}

bool WriteTextFile(const std::string& path, const std::string& text) {
  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  const bool wrote = std::fwrite(text.data(), 1, text.size(), f) == text.size();
  const bool closed = std::fclose(f) == 0;
  return wrote && closed;
}

bool MakeDirs(const std::string& path) {
  for (size_t i = 1; i <= path.size(); ++i) {
    if (i != path.size() && path[i] != '/') continue;
    const std::string part = path.substr(0, i);
    if (part.empty() || part.back() == '/') continue;
    mkdir(part.c_str(), 0777);  // el error (ya existe, etc.) se ignora a proposito
  }
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

}  // namespace dwu
