// config.h — build-time configuration for QEditNA
//
// DATA SAFETY: kWriteEnabled gates every path that touches the disk.
// It MUST stay false for all v0.x test builds (see CLAUDE.md).
#pragma once

#include "constants.h"

namespace qed {
namespace config {

// Master write-protect flag. false => no file write ever reaches the disk.
#ifdef QEDITNA_ENABLE_WRITE
constexpr bool kWriteEnabled = true;
#else
constexpr bool kWriteEnabled = false;
#endif

// Create a .bak copy before overwriting an existing file.
constexpr bool kBackupOnWrite = true;

// Rendering defaults.
constexpr wchar_t kFontFace[] = L"Consolas";
constexpr int     kFontHeight = 16;   // logical units, negative applied later

} // namespace config
} // namespace qed
