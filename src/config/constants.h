// constants.h — compile-time constants for QEditNA
#pragma once

#define QEDITNA_VERSION_MAJOR 0
#define QEDITNA_VERSION_MINOR 1
#define QEDITNA_VERSION_PATCH 0
#define QEDITNA_VERSION_STRING L"0.1.0"
#define QEDITNA_ENABLE_WRITE 1

#define QEDITNA_APP_NAME      L"QEditNA"
#define QEDITNA_WINDOW_CLASS  L"QEditNAMainWindow"

namespace qed {

// Editor layout / behaviour constants.
constexpr int  kTabWidth        = 4;    // spaces per tab when rendering
constexpr int  kStatusBarPadPx  = 4;    // horizontal padding inside status bar
constexpr int  kMaxLineLength   = 1 << 20;

} // namespace qed
