#include "wiiu/display.h"

#include <coreinit/cache.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/screen.h>

#include <string>

namespace dwu {
namespace wiiu {

namespace {

// Columnas seguras en ambas pantallas (el GamePad es la mas estrecha).
constexpr size_t kMaxCols = 64;

// Filas de texto: titulo, hueco, lista, hueco, aviso, pie.
constexpr int kTitleRow = 0;
constexpr int kListRow = 2;
constexpr int kStatusRow = kListRow + static_cast<int>(Display::kListRows) + 1;
constexpr int kFooterRow = kStatusRow + 1;

std::string Clip(const std::string& s) {
  return s.size() <= kMaxCols ? s : s.substr(0, kMaxCols - 3) + "...";
}

void DrawTo(OSScreenID screen, void* buffer, uint32_t size, const Frame& f) {
  OSScreenClearBufferEx(screen, 0);
  OSScreenPutFontEx(screen, 0, kTitleRow, Clip(f.title).c_str());

  int row = kListRow;
  for (const std::string& line : f.lines) {
    if (row >= kListRow + static_cast<int>(Display::kListRows)) break;
    OSScreenPutFontEx(screen, 0, row++, Clip(line).c_str());
  }
  if (!f.status.empty()) OSScreenPutFontEx(screen, 0, kStatusRow, Clip(f.status).c_str());
  OSScreenPutFontEx(screen, 0, kFooterRow, Clip(f.footer).c_str());

  DCFlushRange(buffer, size);
  OSScreenFlipBuffersEx(screen);
}

}  // namespace

bool Display::Init() {
  OSScreenInit();
  tvSize_ = OSScreenGetBufferSizeEx(SCREEN_TV);
  drcSize_ = OSScreenGetBufferSizeEx(SCREEN_DRC);
  tvBuffer_ = MEMAllocFromDefaultHeapEx(tvSize_, 0x100);
  drcBuffer_ = MEMAllocFromDefaultHeapEx(drcSize_, 0x100);
  if (!tvBuffer_ || !drcBuffer_) {
    Shutdown();
    return false;
  }
  OSScreenSetBufferEx(SCREEN_TV, tvBuffer_);
  OSScreenSetBufferEx(SCREEN_DRC, drcBuffer_);
  OSScreenEnableEx(SCREEN_TV, true);
  OSScreenEnableEx(SCREEN_DRC, true);
  return true;
}

void Display::Shutdown() {
  if (tvBuffer_) MEMFreeToDefaultHeap(tvBuffer_);
  if (drcBuffer_) MEMFreeToDefaultHeap(drcBuffer_);
  tvBuffer_ = drcBuffer_ = nullptr;
}

void Display::Draw(const Frame& frame) {
  if (!tvBuffer_ || !drcBuffer_) return;
  DrawTo(SCREEN_TV, tvBuffer_, tvSize_, frame);
  DrawTo(SCREEN_DRC, drcBuffer_, drcSize_, frame);
}

}  // namespace wiiu
}  // namespace dwu
