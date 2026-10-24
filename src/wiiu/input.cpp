#include "wiiu/input.h"

#include <vpad/input.h>

namespace dwu {
namespace wiiu {

void InputInit() { VPADInit(); }

void InputShutdown() { VPADShutdown(); }

std::vector<Button> InputPoll() {
  std::vector<Button> pressed;

  VPADStatus status;
  VPADReadError error = VPAD_READ_SUCCESS;
  if (VPADRead(VPAD_CHAN_0, &status, 1, &error) < 1 || error != VPAD_READ_SUCCESS) {
    return pressed;
  }

  struct Map {
    uint32_t mask;
    Button button;
  };
  static const Map kMap[] = {
      {VPAD_BUTTON_UP, Button::Up},       {VPAD_BUTTON_DOWN, Button::Down},
      {VPAD_BUTTON_LEFT, Button::Left},   {VPAD_BUTTON_RIGHT, Button::Right},
      {VPAD_BUTTON_A, Button::A},         {VPAD_BUTTON_B, Button::B},
      {VPAD_BUTTON_PLUS, Button::Plus},   {VPAD_BUTTON_MINUS, Button::Minus},
  };
  for (const Map& m : kMap) {
    if (status.trigger & m.mask) pressed.push_back(m.button);
  }
  return pressed;
}

}  // namespace wiiu
}  // namespace dwu
