// Interfaz entre la interfaz de usuario y el nucleo de emulacion.
//
// La UI solo conoce esta clase. Hoy la implementa StubCore (no emula nada);
// el dia que el nucleo de Dolphin este portado, bastara con escribir un
// DolphinCore que herede de EmuCore. Ver docs/CORE_PORTING.md.
#pragma once

#include <string>

namespace dwu {

class Settings;

enum class Platform { Unknown, GameCube, Wii };

struct GameEntry {
  std::string path;
  std::string title;
  std::string gameId;  // 6 caracteres si se pudo leer del disco, si no vacio
  Platform platform = Platform::Unknown;
};

class EmuCore {
 public:
  virtual ~EmuCore() = default;

  // Nombre legible del nucleo (se muestra en "Acerca de").
  virtual const char* Name() const = 0;

  // true solo si detras hay un nucleo de emulacion real.
  virtual bool IsAvailable() const = 0;

  // Prepara el juego. Si falla, devuelve false y deja un mensaje CORTO en
  // `error` (solo ASCII, la fuente de OSScreen no soporta acentos).
  virtual bool Load(const GameEntry& game, const Settings& settings,
                    std::string& error) = 0;

  // Se llama una vez por frame mientras hay un juego en ejecucion.
  virtual void RunFrame() = 0;

  // Detiene el juego y libera recursos.
  virtual void Stop() = 0;
};

}  // namespace dwu
