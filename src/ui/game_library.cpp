#include "ui/game_library.h"

#include <dirent.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace dwu {

namespace {

std::string Lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::string ExtensionOf(const std::string& name) {
  const size_t dot = name.find_last_of('.');
  if (dot == std::string::npos) return "";
  return Lower(name.substr(dot + 1));
}

std::string BaseName(const std::string& path) {
  const size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string StripExtension(const std::string& name) {
  const size_t dot = name.find_last_of('.');
  return (dot == std::string::npos || dot == 0) ? name : name.substr(0, dot);
}

// La fuente de OSScreen solo dibuja ASCII imprimible.
std::string ToAscii(const std::string& s) {
  std::string out;
  for (unsigned char c : s) out += (c >= 32 && c < 127) ? static_cast<char>(c) : '?';
  const size_t last = out.find_last_not_of(' ');
  return last == std::string::npos ? "" : out.substr(0, last + 1);
}

// Lee la cabecera de un disco sin comprimir (.iso / .gcm).
bool ProbeDiscHeader(const std::string& path, GameEntry& out) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  unsigned char h[0x60];
  const size_t n = std::fread(h, 1, sizeof h, f);
  std::fclose(f);
  if (n < sizeof h) return false;

  const bool wii = h[0x18] == 0x5D && h[0x19] == 0x1C && h[0x1A] == 0x9E && h[0x1B] == 0xA3;
  const bool gc = h[0x1C] == 0xC2 && h[0x1D] == 0x33 && h[0x1E] == 0x9F && h[0x1F] == 0x3D;
  if (!wii && !gc) return false;

  out.platform = wii ? Platform::Wii : Platform::GameCube;

  std::string id(reinterpret_cast<const char*>(h), 6);
  const bool idOk = std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isalnum(c) != 0; });
  if (idOk) out.gameId = id;

  std::string title;
  for (size_t i = 0x20; i < 0x60 && h[i] != 0; ++i) title += static_cast<char>(h[i]);
  title = ToAscii(title);
  if (!title.empty()) out.title = title;
  return true;
}

}  // namespace

bool HasGameExtension(const std::string& fileName) {
  static const char* kExtensions[] = {"iso", "gcm", "ciso", "gcz", "wbfs", "rvz",
                                      "wia", "tgc",  "wad",  "dol", "elf"};
  const std::string ext = ExtensionOf(fileName);
  for (const char* e : kExtensions) {
    if (ext == e) return true;
  }
  return false;
}

GameEntry DescribeFile(const std::string& path) {
  GameEntry g;
  g.path = path;
  const std::string name = BaseName(path);
  g.title = ToAscii(StripExtension(name));

  const std::string ext = ExtensionOf(name);
  if (ext == "gcm" || ext == "tgc") g.platform = Platform::GameCube;
  if (ext == "wbfs" || ext == "wad") g.platform = Platform::Wii;
  if (ext == "iso" || ext == "gcm") ProbeDiscHeader(path, g);
  return g;
}

std::vector<GameEntry> ScanGames(const std::string& directory) {
  std::vector<GameEntry> games;
  DIR* dir = opendir(directory.c_str());
  if (!dir) return games;

  const std::string prefix =
      (!directory.empty() && directory.back() == '/') ? directory : directory + "/";
  while (const dirent* entry = readdir(dir)) {
    const std::string name = entry->d_name;
    if (name.empty() || name[0] == '.') continue;
    if (!HasGameExtension(name)) continue;
    games.push_back(DescribeFile(prefix + name));
  }
  closedir(dir);

  std::sort(games.begin(), games.end(), [](const GameEntry& a, const GameEntry& b) {
    const std::string ta = Lower(a.title), tb = Lower(b.title);
    return ta != tb ? ta < tb : a.path < b.path;
  });
  return games;
}

}  // namespace dwu
