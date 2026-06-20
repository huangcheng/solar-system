#pragma once

struct BodyConfig;

namespace bodyapp {

// Runs a standalone body widget app. Encapsulates QApplication setup, singleton
// mutex (keyed off config.bodyId), translator, AssetManager, SunModel (used for
// day/night shading on planets; ignored by emissive Sun), CameraController,
// CelestialWidget, TimeController, SystemTray, SettingsDialog, FullscreenWatcher.
// Returns app.exec().
//
// Earth is intentionally NOT migrated here: its main.cpp has Earth-specific
// geolocation (LocationProvider) and a sun-sub-point tooltip that the other
// bodies don't need. Refactoring it would risk regression for no gain.
int runBodyApp(int argc, char *argv[], const BodyConfig& config);

} // namespace bodyapp
