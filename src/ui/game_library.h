// Busca discos en una carpeta y lee los datos basicos de su cabecera.
#pragma once

#include <string>
#include <vector>

#include "core/emu_core.h"

namespace dwu {

// true si el nombre tiene una extension de imagen de disco que Dolphin
// reconoce (iso, gcm, ciso, gcz, wbfs, rvz, wia, tgc, wad, dol, elf).
bool HasGameExtension(const std::string& fileName);

// Describe un archivo concreto. Para .iso y .gcm lee la cabecera del disco
// (id de juego, titulo y si es Wii o GameCube); para el resto usa el nombre
// del archivo y deduce la plataforma por la extension cuando se puede.
GameEntry DescribeFile(const std::string& path);

// Lista los juegos de una carpeta (sin recursion), ordenados por titulo.
// Devuelve una lista vacia si la carpeta no existe. Los titulos salen en
// ASCII: cualquier otro byte se cambia por '?'.
std::vector<GameEntry> ScanGames(const std::string& directory);

}  // namespace dwu
