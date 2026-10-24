#include "ui/menu.h"

#include <algorithm>
#include <utility>

namespace dwu {

namespace {

const char* PlatformLabel(Platform p) {
  switch (p) {
    case Platform::Wii: return "Wii";
    case Platform::GameCube: return "GameCube";
    case Platform::Unknown: break;
  }
  return "?";
}

std::string Cursor(bool selected) { return selected ? "> " : "  "; }

// Ventana de `rows` elementos que siempre contiene a `cursor`.
size_t WindowStart(size_t cursor, size_t count, size_t rows) {
  if (count <= rows) return 0;
  size_t start = cursor >= rows ? cursor - rows + 1 : 0;
  return std::min(start, count - rows);
}

}  // namespace

Menu::Menu(EmuCore& core, Settings& settings, std::vector<GameEntry> games,
           std::string gamesDir, size_t visibleRows)
    : core_(core),
      settings_(settings),
      games_(std::move(games)),
      gamesDir_(std::move(gamesDir)),
      visibleRows_(std::max<size_t>(visibleRows, 3)) {}

void Menu::SetGames(std::vector<GameEntry> games) {
  games_ = std::move(games);
  if (screen_ == Screen::Games && !games_.empty()) {
    cursor_ = std::min(cursor_, games_.size() - 1);
  } else if (screen_ == Screen::Games) {
    cursor_ = 0;
  }
}

bool Menu::TakeSettingsDirty() {
  const bool d = settingsDirty_;
  settingsDirty_ = false;
  return d;
}

void Menu::Enter(Screen s) {
  screen_ = s;
  cursor_ = 0;
  status_.clear();
}

void Menu::MoveCursor(int delta, size_t count) {
  if (count == 0) {
    cursor_ = 0;
    return;
  }
  const long n = static_cast<long>(count);
  cursor_ = static_cast<size_t>(((static_cast<long>(cursor_) + delta) % n + n) % n);
}

void Menu::Launch(const GameEntry& game) {
  std::string error;
  if (core_.Load(game, settings_, error)) {
    runningTitle_ = game.title;
    screen_ = Screen::Running;
    status_.clear();
  } else {
    status_ = error.empty() ? "No se pudo iniciar el juego" : error;
  }
}

void Menu::StopGame() {
  core_.Stop();
  runningTitle_.clear();
  screen_ = Screen::Games;
  status_.clear();
}

void Menu::Tick() {
  if (screen_ == Screen::Running) core_.RunFrame();
}

void Menu::HandleButton(Button b) {
  switch (screen_) {
    case Screen::Main: {
      if (b == Button::Up) MoveCursor(-1, kMainItems);
      if (b == Button::Down) MoveCursor(1, kMainItems);
      if (b != Button::A) return;
      switch (cursor_) {
        case 0: Enter(Screen::Games); break;
        case 1: Enter(Screen::Options); break;
        case 2: Enter(Screen::About); break;
        case 3: wantsQuit_ = true; break;
      }
      return;
    }

    case Screen::Games: {
      if (b == Button::Up) MoveCursor(-1, games_.size());
      if (b == Button::Down) MoveCursor(1, games_.size());
      if (b == Button::B) {
        Enter(Screen::Main);
        return;
      }
      if (b == Button::A && !games_.empty()) Launch(games_[cursor_]);
      return;
    }

    case Screen::Options: {
      if (b == Button::Up) MoveCursor(-1, kCategoryCount);
      if (b == Button::Down) MoveCursor(1, kCategoryCount);
      if (b == Button::B) {
        Enter(Screen::Main);
        return;
      }
      if (b == Button::A) {
        category_ = static_cast<Category>(cursor_);
        categoryCursor_ = cursor_;
        Enter(Screen::Category);
      }
      return;
    }

    case Screen::Category: {
      const std::vector<Opt> opts = OptionsIn(category_);
      if (b == Button::Up) MoveCursor(-1, opts.size());
      if (b == Button::Down) MoveCursor(1, opts.size());
      if (b == Button::B) {
        Enter(Screen::Options);
        cursor_ = categoryCursor_;
        return;
      }
      if (opts.empty()) return;
      if (b == Button::Left || b == Button::Right) {
        settings_.Step(opts[cursor_], b == Button::Right ? 1 : -1);
        settingsDirty_ = true;
        status_.clear();
      }
      if (b == Button::Minus) {
        // Restaura solo las opciones de esta categoria.
        for (Opt o : opts) settings_.Set(o, GetOptionDef(o).defaultIndex);
        settingsDirty_ = true;
        status_ = "Opciones de esta categoria restauradas";
      }
      return;
    }

    case Screen::About: {
      if (b == Button::B || b == Button::A) Enter(Screen::Main);
      return;
    }

    case Screen::Running: {
      if (b == Button::B) StopGame();
      return;
    }
  }
}

Frame Menu::Render() const {
  Frame f;
  f.status = status_;

  switch (screen_) {
    case Screen::Main: {
      f.title = "DolphinWiiU";
      static const char* kItems[kMainItems] = {"Jugar", "Opciones", "Acerca de", "Salir"};
      for (size_t i = 0; i < kMainItems; ++i) f.lines.push_back(Cursor(i == cursor_) + kItems[i]);
      f.footer = "Arriba/Abajo: mover   A: elegir";
      break;
    }

    case Screen::Games: {
      f.title = "Juegos";
      if (games_.empty()) {
        f.lines.push_back("No hay juegos en:");
        f.lines.push_back("  " + gamesDir_);
        f.lines.push_back("Copia ahi tus imagenes de disco");
        f.footer = "B: volver";
        break;
      }
      const size_t start = WindowStart(cursor_, games_.size(), visibleRows_);
      const size_t end = std::min(games_.size(), start + visibleRows_);
      for (size_t i = start; i < end; ++i) {
        f.lines.push_back(Cursor(i == cursor_) + games_[i].title + "  [" +
                          PlatformLabel(games_[i].platform) + "]");
      }
      f.footer = "Arriba/Abajo: mover   A: jugar   B: volver   (" +
                 std::to_string(cursor_ + 1) + "/" + std::to_string(games_.size()) + ")";
      break;
    }

    case Screen::Options: {
      f.title = "Opciones";
      for (int i = 0; i < kCategoryCount; ++i) {
        f.lines.push_back(Cursor(static_cast<size_t>(i) == cursor_) +
                          CategoryLabel(static_cast<Category>(i)));
      }
      f.footer = "Arriba/Abajo: mover   A: abrir   B: volver";
      break;
    }

    case Screen::Category: {
      f.title = std::string("Opciones - ") + CategoryLabel(category_);
      const std::vector<Opt> opts = OptionsIn(category_);
      for (size_t i = 0; i < opts.size(); ++i) {
        f.lines.push_back(Cursor(i == cursor_) + GetOptionDef(opts[i]).label + ": < " +
                          settings_.GetLabel(opts[i]) + " >");
      }
      f.footer = "Izq/Der: cambiar   -: restaurar   B: volver";
      break;
    }

    case Screen::About: {
      f.title = "Acerca de";
      f.lines.push_back("DolphinWiiU");
      f.lines.push_back(std::string("Nucleo: ") + core_.Name());
      f.lines.push_back(core_.IsAvailable() ? "Emulacion: disponible"
                                            : "Emulacion: NO disponible todavia");
      f.lines.push_back("Solo la interfaz esta lista");
      f.footer = "B: volver";
      break;
    }

    case Screen::Running: {
      f.title = runningTitle_;
      f.lines.push_back("Ejecutando...");
      f.footer = "B: detener";
      break;
    }
  }
  return f;
}

}  // namespace dwu
