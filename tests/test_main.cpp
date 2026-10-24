// Pruebas de la logica portable (opciones, biblioteca de juegos, menu).
// Sin dependencias externas: `make -C tests run`.
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "core/stub_core.h"
#include "ui/fs_util.h"
#include "ui/game_library.h"
#include "ui/menu.h"
#include "ui/settings.h"

using namespace dwu;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                          \
  do {                                                                       \
    ++g_checks;                                                              \
    if (!(cond)) {                                                           \
      ++g_failures;                                                          \
      std::cerr << "FALLO " << __FILE__ << ":" << __LINE__ << "  " << #cond  \
                << "\n";                                                     \
    }                                                                        \
  } while (0)

#define CHECK_EQ(a, b)                                                       \
  do {                                                                       \
    ++g_checks;                                                              \
    if (!((a) == (b))) {                                                     \
      ++g_failures;                                                          \
      std::cerr << "FALLO " << __FILE__ << ":" << __LINE__ << "  " << #a     \
                << " == " << #b << "\n";                                     \
    }                                                                        \
  } while (0)

// ---- ayudas ---------------------------------------------------------------

static std::string MakeTempDir() {
  char tmpl[] = "/tmp/dwu_test_XXXXXX";
  const char* d = mkdtemp(tmpl);
  if (!d) {
    std::cerr << "no se pudo crear carpeta temporal\n";
    std::exit(2);
  }
  return d;
}

static void WriteFile(const std::string& path, const std::string& data) {
  std::ofstream f(path, std::ios::binary);
  f.write(data.data(), static_cast<std::streamsize>(data.size()));
}

// Cabecera de disco de 0x60 bytes con id y titulo.
static std::string FakeDisc(bool wii, const std::string& id, const std::string& title) {
  std::string h(0x60, '\0');
  h.replace(0, id.size(), id);
  if (wii) {
    const unsigned char m[4] = {0x5D, 0x1C, 0x9E, 0xA3};
    h.replace(0x18, 4, reinterpret_cast<const char*>(m), 4);
  } else {
    const unsigned char m[4] = {0xC2, 0x33, 0x9F, 0x3D};
    h.replace(0x1C, 4, reinterpret_cast<const char*>(m), 4);
  }
  h.replace(0x20, title.size(), title);
  return h;
}

// Nucleo falso que si "emula": permite probar el camino de exito.
class FakeCore : public EmuCore {
 public:
  const char* Name() const override { return "Fake"; }
  bool IsAvailable() const override { return true; }
  bool Load(const GameEntry& g, const Settings&, std::string& error) override {
    if (fail) {
      error = "boom";
      return false;
    }
    loaded = g.path;
    return true;
  }
  void RunFrame() override { ++frames; }
  void Stop() override { ++stops; }

  bool fail = false;
  std::string loaded;
  int frames = 0;
  int stops = 0;
};

static std::vector<GameEntry> SomeGames(int n) {
  std::vector<GameEntry> v;
  for (int i = 0; i < n; ++i) {
    GameEntry g;
    g.path = "/g/" + std::to_string(i) + ".iso";
    g.title = "Juego " + std::to_string(i);
    g.platform = (i % 2) ? Platform::Wii : Platform::GameCube;
    v.push_back(g);
  }
  return v;
}

// ---- Settings -------------------------------------------------------------

static void TestSettings() {
  // La tabla debe tener una entrada por cada valor del enum, en el mismo orden.
  CHECK_EQ(static_cast<int>(OptionDefs().size()), kOptionCount);
  for (int i = 0; i < kOptionCount; ++i) {
    CHECK_EQ(static_cast<int>(OptionDefs()[static_cast<size_t>(i)].id), i);
    const OptionDef& d = OptionDefs()[static_cast<size_t>(i)];
    CHECK(d.defaultIndex >= 0 && d.defaultIndex < static_cast<int>(d.choices.size()));
    CHECK(!d.choices.empty());
  }

  // Cada categoria tiene al menos una opcion.
  for (int c = 0; c < kCategoryCount; ++c) {
    CHECK(!OptionsIn(static_cast<Category>(c)).empty());
  }

  Settings s;
  CHECK_EQ(s.Get(Opt::Volume), 8);
  CHECK_EQ(s.GetLabel(Opt::Volume), std::string("80%"));
  CHECK_EQ(s.GetLabel(Opt::OutputScreen), std::string("TV y GamePad"));

  // Step da la vuelta en ambos sentidos.
  s.Set(Opt::VSync, 0);
  s.Step(Opt::VSync, -1);
  CHECK_EQ(s.Get(Opt::VSync), 1);
  s.Step(Opt::VSync, 1);
  CHECK_EQ(s.Get(Opt::VSync), 0);

  // Set recorta al rango.
  s.Set(Opt::Volume, 999);
  CHECK_EQ(s.GetLabel(Opt::Volume), std::string("100%"));
  s.Set(Opt::Volume, -5);
  CHECK_EQ(s.GetLabel(Opt::Volume), std::string("0%"));

  // Serializar y volver a leer da lo mismo.
  Settings a;
  a.Set(Opt::ResolutionScale, 2);
  a.Set(Opt::Overclock, 4);
  a.Set(Opt::InputSource, 1);
  a.Set(Opt::Rumble, 1);
  Settings b;
  b.Parse(a.Serialize());
  CHECK(a == b);

  // Entrada sucia: no debe romper nada ni cambiar lo que no entiende.
  Settings c;
  c.Parse(
      "# comentario\n"
      "\n"
      "; otro comentario\n"
      "clave_inexistente=3\n"
      "volume=150%\n"          // valor invalido
      "sin_igual\n"
      "  vsync  =  No  \r\n"   // espacios y CRLF
      "aspect_ratio=16:9");    // sin salto de linea final
  CHECK_EQ(c.GetLabel(Opt::VSync), std::string("No"));
  CHECK_EQ(c.GetLabel(Opt::AspectRatio), std::string("16:9"));
  CHECK_EQ(c.Get(Opt::Volume), 8);  // sigue en el valor por defecto

  // Cadena vacia no hace nada.
  Settings d;
  d.Parse("");
  CHECK(d == Settings());

  Settings e;
  e.Set(Opt::Overclock, 5);
  e.ResetDefaults();
  CHECK(e == Settings());
}

// ---- Archivos --------------------------------------------------------------

static void TestFsUtil() {
  const std::string dir = MakeTempDir();

  // MakeDirs crea toda la cadena y es idempotente.
  const std::string deep = dir + "/a/b/c";
  CHECK(MakeDirs(deep));
  CHECK(MakeDirs(deep));
  CHECK(MakeDirs(deep + "/"));

  // Si la ruta es un archivo, no cuenta como carpeta.
  WriteFile(dir + "/archivo", "x");
  CHECK(!MakeDirs(dir + "/archivo"));

  // Escribir, leer y reemplazar.
  const std::string f = deep + "/settings.ini";
  std::string got = "previo";
  CHECK(!ReadTextFile(f, got));
  CHECK_EQ(got, std::string("previo"));  // no se toca si falla

  CHECK(WriteTextFile(f, "uno\ndos\n"));
  CHECK(ReadTextFile(f, got));
  CHECK_EQ(got, std::string("uno\ndos\n"));
  CHECK(WriteTextFile(f, "x"));
  CHECK(ReadTextFile(f, got));
  CHECK_EQ(got, std::string("x"));

  // Archivo grande (mas de un bloque de lectura) y vacio.
  const std::string big(5000, 'z');
  CHECK(WriteTextFile(f, big));
  CHECK(ReadTextFile(f, got));
  CHECK_EQ(got, big);
  CHECK(WriteTextFile(f, ""));
  CHECK(ReadTextFile(f, got));
  CHECK(got.empty());

  // Escribir en carpeta inexistente falla limpiamente.
  CHECK(!WriteTextFile(dir + "/nada/x.ini", "x"));

  // Ciclo completo de configuracion: guardar y recargar.
  Settings a;
  a.Set(Opt::ResolutionScale, 1);
  a.Set(Opt::Volume, 3);
  CHECK(WriteTextFile(dir + "/ajustes.ini", a.Serialize()));
  std::string text;
  CHECK(ReadTextFile(dir + "/ajustes.ini", text));
  Settings b;
  b.Parse(text);
  CHECK(a == b);

  std::string cmd = "rm -rf '" + dir + "'";
  if (std::system(cmd.c_str()) != 0) std::cerr << "aviso: no se pudo borrar " << dir << "\n";
}

// ---- Biblioteca de juegos --------------------------------------------------

static void TestGameLibrary() {
  CHECK(HasGameExtension("a.iso"));
  CHECK(HasGameExtension("A.ISO"));
  CHECK(HasGameExtension("x.wbfs"));
  CHECK(HasGameExtension("x.rvz"));
  CHECK(!HasGameExtension("notas.txt"));
  CHECK(!HasGameExtension("iso"));
  CHECK(!HasGameExtension(""));

  const std::string dir = MakeTempDir();
  WriteFile(dir + "/zelda.iso", FakeDisc(false, "GZLE01", "Zelda Wind Waker"));
  WriteFile(dir + "/mario.iso", FakeDisc(true, "RMGE01", "Super Mario Galaxy"));
  WriteFile(dir + "/basura.iso", std::string(10, 'x'));  // demasiado corto
  WriteFile(dir + "/Banjo.wbfs", "x");
  WriteFile(dir + "/notas.txt", "hola");
  WriteFile(dir + "/.oculto.iso", "x");

  GameEntry z = DescribeFile(dir + "/zelda.iso");
  CHECK(z.platform == Platform::GameCube);
  CHECK_EQ(z.gameId, std::string("GZLE01"));
  CHECK_EQ(z.title, std::string("Zelda Wind Waker"));

  GameEntry m = DescribeFile(dir + "/mario.iso");
  CHECK(m.platform == Platform::Wii);
  CHECK_EQ(m.gameId, std::string("RMGE01"));

  // Sin cabecera valida cae al nombre del archivo.
  GameEntry j = DescribeFile(dir + "/basura.iso");
  CHECK(j.platform == Platform::Unknown);
  CHECK_EQ(j.title, std::string("basura"));
  CHECK(j.gameId.empty());

  // wbfs se reconoce como Wii por la extension.
  CHECK(DescribeFile(dir + "/Banjo.wbfs").platform == Platform::Wii);

  // Archivo inexistente no revienta.
  CHECK(DescribeFile(dir + "/no_existe.iso").platform == Platform::Unknown);

  // Escaneo: ordenado sin distinguir mayusculas, sin ocultos ni txt.
  std::vector<GameEntry> games = ScanGames(dir);
  CHECK_EQ(static_cast<int>(games.size()), 4);
  if (games.size() == 4) {
    CHECK_EQ(games[0].title, std::string("Banjo"));
    CHECK_EQ(games[1].title, std::string("basura"));
    CHECK_EQ(games[2].title, std::string("Super Mario Galaxy"));
    CHECK_EQ(games[3].title, std::string("Zelda Wind Waker"));
  }
  CHECK(ScanGames(dir + "/no_existe").empty());
  CHECK(ScanGames(dir + "/").size() == 4);  // barra final

  // Titulos no ASCII se vuelven '?'.
  WriteFile(dir + "/Pok\xc3\xa9mon.iso", "x");
  bool found = false;
  for (const GameEntry& g : ScanGames(dir)) {
    if (g.title == "Pok??mon") found = true;
  }
  CHECK(found);

  std::string cmd = "rm -rf '" + dir + "'";
  if (std::system(cmd.c_str()) != 0) std::cerr << "aviso: no se pudo borrar " << dir << "\n";
}

// ---- Menu ------------------------------------------------------------------

static void Press(Menu& m, Button b, int times = 1) {
  for (int i = 0; i < times; ++i) m.HandleButton(b);
}

static bool HasLinePrefix(const Frame& f, const std::string& prefix) {
  for (const std::string& l : f.lines) {
    if (l.compare(0, prefix.size(), prefix) == 0) return true;
  }
  return false;
}

static void TestMenuNavigation() {
  StubCore core;
  Settings s;
  Menu m(core, s, SomeGames(3), "/juegos", 5);

  CHECK(m.CurrentScreen() == Screen::Main);
  Frame f = m.Render();
  CHECK_EQ(f.title, std::string("DolphinWiiU"));
  CHECK_EQ(static_cast<int>(f.lines.size()), 4);
  CHECK(HasLinePrefix(f, "> Jugar"));

  // Arriba desde el primero da la vuelta a "Salir".
  Press(m, Button::Up);
  CHECK(HasLinePrefix(m.Render(), "> Salir"));
  Press(m, Button::Down);
  CHECK(HasLinePrefix(m.Render(), "> Jugar"));

  // Opciones -> Video -> volver -> volver.
  Press(m, Button::Down);
  Press(m, Button::A);
  CHECK(m.CurrentScreen() == Screen::Options);
  Press(m, Button::A);
  CHECK(m.CurrentScreen() == Screen::Category);
  CHECK_EQ(m.Render().title, std::string("Opciones - Video"));
  Press(m, Button::B);
  CHECK(m.CurrentScreen() == Screen::Options);
  Press(m, Button::B);
  CHECK(m.CurrentScreen() == Screen::Main);

  // Acerca de: dice la verdad sobre el nucleo.
  Press(m, Button::Down, 2);
  Press(m, Button::A);
  CHECK(m.CurrentScreen() == Screen::About);
  bool admitsNoCore = false;
  for (const std::string& l : m.Render().lines) {
    if (l.find("NO disponible") != std::string::npos) admitsNoCore = true;
  }
  CHECK(admitsNoCore);
  Press(m, Button::B);
  CHECK(m.CurrentScreen() == Screen::Main);

  // Salir.
  CHECK(!m.WantsQuit());
  Press(m, Button::Up);  // Salir
  Press(m, Button::A);
  CHECK(m.WantsQuit());
}

static void TestMenuOptions() {
  StubCore core;
  Settings s;
  Menu m(core, s, {}, "/juegos", 5);

  // Opciones -> Audio (segunda categoria) -> primera opcion = Volumen.
  Press(m, Button::Down);
  Press(m, Button::A);
  Press(m, Button::Down);
  Press(m, Button::A);
  CHECK_EQ(m.Render().title, std::string("Opciones - Audio"));
  CHECK(!m.TakeSettingsDirty());

  Press(m, Button::Right);
  CHECK_EQ(s.GetLabel(Opt::Volume), std::string("90%"));
  CHECK(m.TakeSettingsDirty());
  CHECK(!m.TakeSettingsDirty());  // solo una vez

  Press(m, Button::Left, 2);
  CHECK_EQ(s.GetLabel(Opt::Volume), std::string("70%"));
  CHECK(m.Render().lines[0].find("< 70% >") != std::string::npos);

  // Mover a la segunda opcion y cambiarla no toca el volumen.
  Press(m, Button::Down);
  Press(m, Button::Right);
  CHECK_EQ(s.GetLabel(Opt::DspEngine), std::string("LLE (preciso)"));
  CHECK_EQ(s.GetLabel(Opt::Volume), std::string("70%"));

  // Menos restaura solo esta categoria.
  s.Set(Opt::VSync, 1);  // otra categoria
  Press(m, Button::Minus);
  CHECK_EQ(s.GetLabel(Opt::Volume), std::string("80%"));
  CHECK_EQ(s.GetLabel(Opt::DspEngine), std::string("HLE (rapido)"));
  CHECK_EQ(s.GetLabel(Opt::VSync), std::string("No"));
  CHECK(m.TakeSettingsDirty());

  // Al volver, el cursor queda en la categoria que se abrio (Audio).
  Press(m, Button::B);
  CHECK(HasLinePrefix(m.Render(), "> Audio"));
}

static void TestMenuGames() {
  FakeCore core;
  Settings s;
  Menu m(core, s, SomeGames(12), "/juegos", 5);

  Press(m, Button::A);  // Jugar
  CHECK(m.CurrentScreen() == Screen::Games);

  Frame f = m.Render();
  CHECK_EQ(static_cast<int>(f.lines.size()), 5);  // solo caben 5
  CHECK(HasLinePrefix(f, "> Juego 0"));
  CHECK(f.lines[0].find("[GameCube]") != std::string::npos);
  CHECK(f.lines[1].find("[Wii]") != std::string::npos);
  CHECK(f.footer.find("(1/12)") != std::string::npos);

  // Bajar 7: la ventana debe seguir al cursor.
  Press(m, Button::Down, 7);
  f = m.Render();
  CHECK_EQ(static_cast<int>(f.lines.size()), 5);
  CHECK(HasLinePrefix(f, "  Juego 3"));  // ventana 3..7
  CHECK(f.lines[4].find("> Juego 7") == 0);

  // Dar la vuelta hacia abajo y hacia arriba.
  Press(m, Button::Down, 5);  // 12 -> vuelve a 0
  CHECK(HasLinePrefix(m.Render(), "> Juego 0"));
  Press(m, Button::Up);
  f = m.Render();
  CHECK(f.lines.back().find("> Juego 11") == 0);

  // Lanzar con exito.
  Press(m, Button::A);
  CHECK(m.CurrentScreen() == Screen::Running);
  CHECK_EQ(core.loaded, std::string("/g/11.iso"));
  CHECK_EQ(m.Render().title, std::string("Juego 11"));

  // Tick avanza el nucleo solo mientras se juega.
  m.Tick();
  m.Tick();
  CHECK_EQ(core.frames, 2);

  // B detiene y vuelve a la lista.
  Press(m, Button::B);
  CHECK_EQ(core.stops, 1);
  CHECK(m.CurrentScreen() == Screen::Games);
  m.Tick();
  CHECK_EQ(core.frames, 2);  // ya no avanza

  // Fallo al cargar: se queda en la lista y muestra el motivo.
  core.fail = true;
  Press(m, Button::A);
  CHECK(m.CurrentScreen() == Screen::Games);
  CHECK_EQ(m.Render().status, std::string("boom"));
  Press(m, Button::Down);  // el mensaje de error persiste hasta cambiar de pantalla
  CHECK_EQ(m.Render().status, std::string("boom"));
  Press(m, Button::B);
  CHECK(m.CurrentScreen() == Screen::Main);
  CHECK(m.Render().status.empty());
}

static void TestMenuStubAndEmpty() {
  // Con el nucleo stub, jugar nunca arranca y avisa claramente.
  StubCore core;
  Settings s;
  Menu m(core, s, SomeGames(2), "/juegos", 5);
  Press(m, Button::A);
  Press(m, Button::A);
  CHECK(m.CurrentScreen() == Screen::Games);
  CHECK(!m.Render().status.empty());

  // Lista vacia: explica donde poner los juegos y A no hace nada.
  Menu e(core, s, {}, "/vol/external_usb/juegos", 5);
  Press(e, Button::A);
  Frame f = e.Render();
  bool mentionsDir = false;
  for (const std::string& l : f.lines) {
    if (l.find("/vol/external_usb/juegos") != std::string::npos) mentionsDir = true;
  }
  CHECK(mentionsDir);
  Press(e, Button::A);
  Press(e, Button::Down);
  Press(e, Button::Up);
  CHECK(e.CurrentScreen() == Screen::Games);

  // visibleRows demasiado pequeno se corrige a 3.
  Menu tiny(core, s, SomeGames(10), "/j", 0);
  Press(tiny, Button::A);
  CHECK_EQ(static_cast<int>(tiny.Render().lines.size()), 3);

  // SetGames con el cursor al final no deja un indice fuera de rango.
  FakeCore fc;
  Menu r(fc, s, SomeGames(10), "/j", 5);
  Press(r, Button::A);
  Press(r, Button::Down, 9);
  r.SetGames(SomeGames(3));
  Press(r, Button::A);  // no debe crashear ni cargar fuera de rango
  CHECK_EQ(fc.loaded, std::string("/g/2.iso"));
  r.SetGames({});
  Press(r, Button::A);  // lista vacia, sin crash
  CHECK(r.CurrentScreen() == Screen::Running);  // sigue el juego anterior
}

int main() {
  TestSettings();
  TestFsUtil();
  TestGameLibrary();
  TestMenuNavigation();
  TestMenuOptions();
  TestMenuGames();
  TestMenuStubAndEmpty();

  std::cout << g_checks << " comprobaciones, " << g_failures << " fallos\n";
  return g_failures == 0 ? 0 : 1;
}
