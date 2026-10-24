# DolphinWiiU

Interfaz para un emulador de Wii y GameCube en Wii U, con la idea de
integrar más adelante el núcleo de [Dolphin](https://dolphin-emu.org/).

## Estado

| Parte | Estado |
|---|---|
| Menú (Jugar, Opciones, Acerca de, Salir) | Escrito, lógica con pruebas automáticas en Linux |
| Opciones (Video, Audio, Emulación, Controles) con guardado en `settings.ini` | Escrito, con pruebas |
| Lista de juegos leyendo la cabecera de `.iso` / `.gcm` | Escrito, con pruebas |
| Dibujo en TV/GamePad y lectura del GamePad (`src/wiiu`) | Compila con devkitPPC + wut en GitHub Actions. **Sin probar en una consola real**: no se sabe aún si se ve y responde bien |
| **Núcleo de emulación** | **No existe.** Se usa `StubCore`, que rechaza todos los juegos |

Hoy la app **no ejecuta ningún juego**. Ver [docs/CORE_PORTING.md](docs/CORE_PORTING.md)
para saber qué falta y por qué es un trabajo grande.

## Estructura

```
src/core/   interfaz EmuCore (el contrato con el núcleo) y StubCore
src/ui/     menú, opciones, biblioteca de juegos, archivos (portable)
src/wiiu/   pantalla (OSScreen), GamePad (VPAD) y main — solo Wii U
tests/      pruebas de src/core y src/ui, corren en Linux
docs/       notas para integrar el núcleo
meta/       meta.xml para el Homebrew Launcher / Aroma
```

La lógica no depende de la consola: `src/wiiu` solo traduce el GamePad a
botones y dibuja el texto que produce el menú. Por eso casi todo se puede
probar sin una Wii U.

## Probar la lógica (Linux)

```sh
make -C tests run
```

Necesita `g++` con C++17. Compila con AddressSanitizer y UBSan.

## Compilar para Wii U

Necesita [devkitPro](https://devkitpro.org/wiki/Getting_Started) con
`devkitPPC` y `wut`:

```sh
export DEVKITPRO=/opt/devkitpro
make
```

Genera `DolphinWiiU.rpx`. El workflow de GitHub Actions
(`.github/workflows/build.yml`) hace lo mismo en cada push.

## Usarlo en la consola

Los datos van en la SD, en `fs:/vol/external01/DolphinWiiU/`
(`games/` para tus imágenes de disco, `settings.ini` se crea solo).
Si tu entorno monta la SD con otro prefijo, cámbialo en `src/wiiu/main.cpp`.

## Controles

| Botón | Acción |
|---|---|
| Arriba / Abajo | Mover |
| A | Elegir / jugar |
| B | Volver / detener |
| Izquierda / Derecha | Cambiar el valor de una opción |
| − | Restaurar las opciones de la categoría |
