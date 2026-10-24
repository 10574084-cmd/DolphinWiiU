// Opciones del emulador, definidas en una tabla para que la UI, el guardado
// y las pruebas usen exactamente la misma fuente de verdad.
//
// Todas las opciones son de tipo "lista de valores": se guarda el indice de
// la opcion elegida. Los textos son ASCII porque la fuente de OSScreen (Wii U)
// no dibuja acentos ni enes.
#pragma once

#include <array>
#include <string>
#include <vector>

namespace dwu {

enum class Category { Video, Audio, Emulation, Controls };
constexpr int kCategoryCount = 4;

// El orden de este enum DEBE coincidir con el orden de la tabla en settings.cpp.
enum class Opt {
  ResolutionScale,
  AspectRatio,
  VSync,
  OutputScreen,
  Volume,
  DspEngine,
  CpuCore,
  Overclock,
  DualCore,
  RegionOverride,
  InputSource,
  Rumble,
};
constexpr int kOptionCount = 12;

struct OptionDef {
  Opt id;
  Category category;
  const char* key;    // clave en settings.ini
  const char* label;  // texto en pantalla
  std::vector<std::string> choices;
  int defaultIndex;
};

const std::vector<OptionDef>& OptionDefs();
const OptionDef& GetOptionDef(Opt opt);
std::vector<Opt> OptionsIn(Category category);
const char* CategoryLabel(Category category);

class Settings {
 public:
  Settings();

  int Get(Opt opt) const;                    // indice elegido
  const std::string& GetLabel(Opt opt) const;  // texto de la opcion elegida
  void Set(Opt opt, int index);              // se recorta al rango valido
  void Step(Opt opt, int delta);             // avanza/retrocede con vuelta
  void ResetDefaults();

  // Formato: lineas "clave=Texto de la opcion". Se guarda el texto (no el
  // indice) para que el archivo sea legible y sobreviva a cambios de orden.
  std::string Serialize() const;
  // Ignora claves desconocidas, valores invalidos, comentarios y lineas
  // vacias. Lo que no se pueda leer queda como estaba.
  void Parse(const std::string& text);

  bool operator==(const Settings& other) const {
    return values_ == other.values_;
  }

 private:
  std::array<int, kOptionCount> values_;
};

}  // namespace dwu
