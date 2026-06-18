#include "PlatformWindow.h"
#include <QWidget>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#    define DWMWA_WINDOW_CORNER_PREFERENCE 33
#  endif
#  ifndef DWMWA_SYSTEMBACKDROP_TYPE
#    define DWMWA_SYSTEMBACKDROP_TYPE 38
#  endif
#  pragma comment(lib, "dwmapi.lib")
#endif

namespace PlatformWindow {
void applyDwmFramelessAttributes(QWidget *widget) {
#ifdef Q_OS_WIN
    if (!widget) return;
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return;
    const int doNotRound = 1;
    ::DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &doNotRound, sizeof(doNotRound));
    const int backdropNone = 1;
    ::DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropNone, sizeof(backdropNone));
    const int ncDisabled = 1; // DWMNCRP_DISABLED
    ::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncDisabled, sizeof(ncDisabled));
#else
    (void)widget;
#endif
}
void refreshComposition(QWidget *widget) {
#ifdef Q_OS_WIN
    if (!widget) return;
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return;
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    widget->update();
#else
    (void)widget;
#endif
}
}
