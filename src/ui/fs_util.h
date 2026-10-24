// Utilidades minimas de archivos (portables: Linux y Wii U/newlib).
#pragma once

#include <string>

namespace dwu {

// Lee un archivo de texto completo. false si no existe o no se puede leer.
bool ReadTextFile(const std::string& path, std::string& out);

// Escribe (o reemplaza) un archivo. false si no se pudo escribir completo.
bool WriteTextFile(const std::string& path, const std::string& text);

// Como `mkdir -p`. Los errores de los tramos intermedios se ignoran (en
// Wii U el primer tramo, "fs:", no es una carpeta real); el resultado dice
// si la ruta final existe y es una carpeta.
bool MakeDirs(const std::string& path);

}  // namespace dwu
