# Portar el núcleo de Dolphin a Wii U

Estado actual: **no hay núcleo de emulación**. La interfaz usa `StubCore`
(`src/core/stub_core.*`), que rechaza cualquier juego con el mensaje
"Nucleo no integrado aun". Este documento explica cómo se enchufará uno real
y qué problemas hay que resolver antes. Las estimaciones de dificultad son
una valoración, no algo medido en una consola.

## El contrato: `EmuCore`

Toda la interfaz habla solo con `src/core/emu_core.h`:

| Método | Qué debe hacer |
|---|---|
| `Name()` / `IsAvailable()` | Se muestran en "Acerca de". `IsAvailable()` solo debe ser `true` con emulación real. |
| `Load(game, settings, error)` | Preparar el juego con las opciones elegidas. Si falla, devolver `false` y un mensaje corto en ASCII. |
| `RunFrame()` | Avanzar un frame. Se llama una vez por frame mientras se juega. |
| `Stop()` | Detener y liberar todo. |

Para conectar un núcleo: crear `src/core/dolphin_core.*` con una clase que
herede de `EmuCore` y cambiar `StubCore core;` por la nueva clase en
`src/wiiu/main.cpp`. Las opciones de `Settings` (resolución, DSP, CPU,
overclock, región…) ya llegan a `Load()`; falta traducirlas a la
configuración interna de Dolphin.

## Por qué no es "solo compilar Dolphin"

Dolphin es un proyecto grande pensado para PC y móviles modernos. Los
puntos que más pesan para Wii U:

1. **Arquitectura del host.** Dolphin solo soporta hosts de 64 bits
   (x86-64 y ARM64) y sus compiladores JIT son para esas dos. La CPU de
   Wii U ("Espresso") es PowerPC de 32 bits y *big-endian*. Habría que
   quitar supuestos de 64 bits y de endianness, y no se puede reutilizar
   ningún JIT existente: se partiría de los intérpretes de CPU.
2. **Rendimiento.** Los intérpretes son mucho más lentos que un JIT. La CPU
   de Wii U es de la misma familia que las del Wii y GameCube (por eso vWii
   las ejecuta nativamente por hardware), pero emularlas por software en
   ella probablemente no llegue a velocidad completa en la mayoría de juegos.
   Hay que medirlo pronto, antes de invertir en lo demás.
3. **Vídeo.** Los backends de Dolphin son OpenGL, Vulkan, D3D, Metal y
   software. Para homebrew de Wii U haría falta un backend nuevo sobre GX2,
   la API gráfica de la consola.
4. **Audio, entrada y sistema de archivos.** Hay que adaptar salida de audio
   (AX), GamePad/Pro Controller/Wiimote, y la lectura de discos desde la SD
   o USB.
5. **Licencia.** Dolphin es GPL-2.0-o-posterior. Si se incorpora su código,
   DolphinWiiU debe publicarse con una licencia compatible. Todavía no hay
   archivo `LICENSE` en este repositorio: hay que elegirla antes de copiar
   código de Dolphin.

## Alternativas que ya existen

Wii U ya ejecuta juegos de Wii en modo vWii y de GameCube mediante
herramientas como Nintendont. Para jugar hoy, eso es lo práctico; este
proyecto solo tiene sentido si el objetivo es tener un núcleo propio.

## Hoja de ruta propuesta

0. **Hecho:** menú, opciones persistentes, lector de discos, interfaz
   `EmuCore` y pruebas de la lógica portable.
1. **Viabilidad de compilación.** Añadir Dolphin como submódulo
   (`external/dolphin`) y compilar solo `Common`, el núcleo de CPU y
   `DiscIO` con devkitPPC, sin vídeo ni audio. Objetivo: que enlace.
2. **CPU sin pantalla.** Arrancar un `.dol` de prueba sencillo con el
   intérprete y comprobar que ejecuta instrucciones correctamente. Medir
   instrucciones por segundo.
3. **Vídeo en GX2.** Primero una salida mínima (framebuffer externo, XFB),
   después el backend de GPU completo.
4. **Audio** con AX.
5. **Entrada** (GamePad, Pro Controller, Wiimote).
6. **Lectura de discos** completa (`.iso`, `.wbfs`, `.rvz`…) desde SD/USB.

El paso 2 decide si el proyecto es viable: si el rendimiento medido es
inaceptable, conviene saberlo antes de escribir el backend de vídeo.
