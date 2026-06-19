#pragma once
#include <QObject>

class QTimer;

// Polls the OS to detect whether a fullscreen application (game, video player,
// etc.) is the foreground window. Emits fullscreenAppStarted/Stopped so the
// globe can hide itself and not float on top of fullscreen apps.
//
// Platform support:
//   Windows — GetForegroundWindow + GetMonitorInfo (exclusive + borderless).
//   macOS   — CGWindowListCopyWindowInfo (borderless fullscreen).
//   Linux   — Not implemented; always returns false (no-op, no crash).
class FullscreenWatcher : public QObject {
    Q_OBJECT
public:
    explicit FullscreenWatcher(QObject *parent = nullptr);
    ~FullscreenWatcher() override;

    void start();
    void stop();
    bool isRunning() const;

signals:
    void fullscreenAppStarted();
    void fullscreenAppStopped();

private slots:
    void onPoll();

private:
    QTimer *m_timer = nullptr;
    bool m_prevState = false;

    static constexpr int POLL_INTERVAL_MS = 2000;
    static bool checkFullscreen();
};
