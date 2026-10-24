// Punto de entrada de DolphinWiiU para Wii U (devkitPPC + wut).
//
// AVISO: compila solo con la toolchain de Wii U y no se ha probado en una
// consola real. Toda la logica del menu esta en src/ui y SI tiene pruebas.
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <sysapp/launch.h>
#include <whb/proc.h>

#include <string>

#include "core/stub_core.h"
#include "ui/fs_util.h"
#include "ui/game_library.h"
#include "ui/menu.h"
#include "ui/settings.h"
#include "wiiu/display.h"
#include "wiiu/input.h"

namespace {

// Carpeta de datos en la SD. Si en tu entorno la SD se monta con otro
// prefijo, este es el unico sitio que hay que cambiar.
const char* const kBaseDir = "fs:/vol/external01/DolphinWiiU";
const std::string kGamesDir = std::string(kBaseDir) + "/games";
const std::string kSettingsPath = std::string(kBaseDir) + "/settings.ini";

}  // namespace

int main(int, char**) {
  WHBProcInit();

  dwu::wiiu::Display display;
  const bool displayOk = display.Init();
  dwu::wiiu::InputInit();

  dwu::MakeDirs(kGamesDir);  // si falla, el menu mostrara la lista vacia

  dwu::Settings settings;
  std::string saved;
  if (dwu::ReadTextFile(kSettingsPath, saved)) settings.Parse(saved);

  dwu::StubCore core;
  dwu::Menu menu(core, settings, dwu::ScanGames(kGamesDir), kGamesDir,
                 dwu::wiiu::Display::kListRows);

  bool quitRequested = false;
  while (displayOk && WHBProcIsRunning()) {
    for (dwu::Button b : dwu::wiiu::InputPoll()) menu.HandleButton(b);
    menu.Tick();

    if (menu.TakeSettingsDirty()) dwu::WriteTextFile(kSettingsPath, settings.Serialize());

    if (menu.WantsQuit() && !quitRequested) {
      quitRequested = true;
      SYSLaunchMenu();  // vuelve al menu Home; WHBProcIsRunning() pasara a false
    }

    display.Draw(menu.Render());
    OSSleepTicks(OSMillisecondsToTicks(16));
  }

  dwu::wiiu::InputShutdown();
  display.Shutdown();
  WHBProcShutdown();
  return 0;
}
