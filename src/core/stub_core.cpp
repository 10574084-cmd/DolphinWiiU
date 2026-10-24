#include "core/stub_core.h"

namespace dwu {

bool StubCore::Load(const GameEntry&, const Settings&, std::string& error) {
  error = "Nucleo no integrado aun";
  return false;
}

}  // namespace dwu
