// Dibuja un Frame de texto en la TV y el GamePad con OSScreen.
//
// AVISO: esta capa solo se compila con devkitPPC + wut (la compila el
// workflow de GitHub Actions) y NO se ha probado en una consola real.
#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/menu.h"

namespace dwu {
namespace wiiu {

class Display {
 public:
  // Filas de lista que caben en el GamePad (la pantalla mas pequena).
  static constexpr size_t kListRows = 12;

  bool Init();
  void Shutdown();
  void Draw(const Frame& frame);

 private:
  void* tvBuffer_ = nullptr;
  void* drcBuffer_ = nullptr;
  uint32_t tvSize_ = 0;
  uint32_t drcSize_ = 0;
};

}  // namespace wiiu
}  // namespace dwu
