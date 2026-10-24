// Lee el GamePad y lo convierte a los botones logicos del menu.
#pragma once

#include <vector>

#include "ui/menu.h"

namespace dwu {
namespace wiiu {

void InputInit();
void InputShutdown();

// Botones que se acaban de PULSAR desde la ultima llamada (no los que
// siguen mantenidos).
std::vector<Button> InputPoll();

}  // namespace wiiu
}  // namespace dwu
