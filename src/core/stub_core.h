// Nucleo "vacio": permite probar toda la interfaz sin emular nada.
#pragma once

#include "core/emu_core.h"

namespace dwu {

class StubCore : public EmuCore {
 public:
  const char* Name() const override { return "Sin nucleo (stub)"; }
  bool IsAvailable() const override { return false; }
  bool Load(const GameEntry& game, const Settings& settings,
            std::string& error) override;
  void RunFrame() override {}
  void Stop() override {}
};

}  // namespace dwu
