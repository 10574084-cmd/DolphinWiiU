// Logica del menu, sin nada especifico de Wii U.
//
// Recibe botones y produce un "frame" de texto. La capa de Wii U (src/wiiu)
// solo traduce el GamePad a Button y dibuja el Frame; asi todo lo demas se
// puede probar en Linux (ver tests/).
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/emu_core.h"
#include "ui/settings.h"

namespace dwu {

enum class Button { Up, Down, Left, Right, A, B, Plus, Minus };

enum class Screen { Main, Games, Options, Category, About, Running };

struct Frame {
  std::string title;
  std::vector<std::string> lines;  // el cursor ya viene dibujado como "> "
  std::string footer;              // ayuda de botones
  std::string status;              // mensaje temporal (errores, avisos)
};

class Menu {
 public:
  // `core` y `settings` deben vivir mas que el Menu.
  // `visibleRows` es cuantas lineas de lista caben en pantalla (minimo 3).
  Menu(EmuCore& core, Settings& settings, std::vector<GameEntry> games,
       std::string gamesDir, size_t visibleRows);

  void HandleButton(Button button);

  // Llamar una vez por frame; avanza el nucleo si hay un juego corriendo.
  void Tick();

  Frame Render() const;

  Screen CurrentScreen() const { return screen_; }
  bool WantsQuit() const { return wantsQuit_; }

  // true una sola vez despues de que cambie alguna opcion: la app debe
  // guardar entonces el archivo de configuracion.
  bool TakeSettingsDirty();

  // Reemplaza la lista de juegos (por ejemplo tras volver a escanear).
  void SetGames(std::vector<GameEntry> games);

 private:
  static constexpr size_t kMainItems = 4;  // Jugar, Opciones, Acerca de, Salir

  void Enter(Screen s);
  void MoveCursor(int delta, size_t count);
  void Launch(const GameEntry& game);
  void StopGame();

  EmuCore& core_;
  Settings& settings_;
  std::vector<GameEntry> games_;
  std::string gamesDir_;
  size_t visibleRows_;

  Screen screen_ = Screen::Main;
  size_t cursor_ = 0;
  Category category_ = Category::Video;
  size_t categoryCursor_ = 0;  // recordado al volver desde una categoria
  std::string status_;
  std::string runningTitle_;
  bool wantsQuit_ = false;
  bool settingsDirty_ = false;
};

}  // namespace dwu
