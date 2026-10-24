#include "ui/settings.h"

#include <cstddef>

namespace dwu {

namespace {

std::vector<std::string> VolumeChoices() {
  std::vector<std::string> v;
  for (int i = 0; i <= 100; i += 10) v.push_back(std::to_string(i) + "%");
  return v;
}

std::string Trim(const std::string& s) {
  const char* ws = " \t\r\n";
  const size_t b = s.find_first_not_of(ws);
  if (b == std::string::npos) return "";
  const size_t e = s.find_last_not_of(ws);
  return s.substr(b, e - b + 1);
}

size_t Index(Opt opt) { return static_cast<size_t>(opt); }

}  // namespace

const std::vector<OptionDef>& OptionDefs() {
  static const std::vector<OptionDef> defs = {
      {Opt::ResolutionScale, Category::Video, "resolution_scale",
       "Escala de resolucion", {"1x (nativa)", "2x", "3x"}, 0},
      {Opt::AspectRatio, Category::Video, "aspect_ratio",
       "Relacion de aspecto", {"Auto", "4:3", "16:9"}, 0},
      {Opt::VSync, Category::Video, "vsync",
       "Sincronia vertical", {"Si", "No"}, 0},
      {Opt::OutputScreen, Category::Video, "output_screen",
       "Pantalla de salida", {"TV", "GamePad", "TV y GamePad"}, 2},

      {Opt::Volume, Category::Audio, "volume", "Volumen", VolumeChoices(), 8},
      {Opt::DspEngine, Category::Audio, "dsp_engine",
       "Motor DSP", {"HLE (rapido)", "LLE (preciso)"}, 0},

      {Opt::CpuCore, Category::Emulation, "cpu_core",
       "Nucleo de CPU", {"Interprete", "Interprete en cache"}, 1},
      {Opt::Overclock, Category::Emulation, "overclock",
       "Overclock de CPU", {"50%", "75%", "100%", "125%", "150%", "200%"}, 2},
      {Opt::DualCore, Category::Emulation, "dual_core",
       "Doble nucleo", {"No", "Si"}, 0},
      {Opt::RegionOverride, Category::Emulation, "region",
       "Region", {"Auto", "NTSC-U", "PAL", "NTSC-J"}, 0},

      {Opt::InputSource, Category::Controls, "input_source",
       "Fuente de control", {"GamePad", "Pro Controller", "Wiimote"}, 0},
      {Opt::Rumble, Category::Controls, "rumble",
       "Vibracion", {"Si", "No"}, 0},
  };
  return defs;
}

const OptionDef& GetOptionDef(Opt opt) { return OptionDefs()[Index(opt)]; }

std::vector<Opt> OptionsIn(Category category) {
  std::vector<Opt> out;
  for (const OptionDef& d : OptionDefs()) {
    if (d.category == category) out.push_back(d.id);
  }
  return out;
}

const char* CategoryLabel(Category category) {
  switch (category) {
    case Category::Video: return "Video";
    case Category::Audio: return "Audio";
    case Category::Emulation: return "Emulacion";
    case Category::Controls: return "Controles";
  }
  return "";
}

Settings::Settings() { ResetDefaults(); }

int Settings::Get(Opt opt) const { return values_[Index(opt)]; }

const std::string& Settings::GetLabel(Opt opt) const {
  return GetOptionDef(opt).choices[static_cast<size_t>(values_[Index(opt)])];
}

void Settings::Set(Opt opt, int index) {
  const int n = static_cast<int>(GetOptionDef(opt).choices.size());
  if (index < 0) index = 0;
  if (index > n - 1) index = n - 1;
  values_[Index(opt)] = index;
}

void Settings::Step(Opt opt, int delta) {
  const int n = static_cast<int>(GetOptionDef(opt).choices.size());
  values_[Index(opt)] = ((values_[Index(opt)] + delta) % n + n) % n;
}

void Settings::ResetDefaults() {
  for (const OptionDef& d : OptionDefs()) values_[Index(d.id)] = d.defaultIndex;
}

std::string Settings::Serialize() const {
  std::string out = "# DolphinWiiU - configuracion\n";
  for (const OptionDef& d : OptionDefs()) {
    out += d.key;
    out += '=';
    out += GetLabel(d.id);
    out += '\n';
  }
  return out;
}

void Settings::Parse(const std::string& text) {
  size_t pos = 0;
  while (pos <= text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    const std::string line = Trim(text.substr(pos, end - pos));
    pos = end + 1;

    if (line.empty() || line[0] == '#' || line[0] == ';') continue;
    const size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = Trim(line.substr(0, eq));
    const std::string value = Trim(line.substr(eq + 1));

    for (const OptionDef& d : OptionDefs()) {
      if (key != d.key) continue;
      for (size_t i = 0; i < d.choices.size(); ++i) {
        if (d.choices[i] == value) values_[Index(d.id)] = static_cast<int>(i);
      }
      break;
    }
  }
}

}  // namespace dwu
