#pragma once

/// The board's mDNS name: it answers at http://matrixcontroller.local. Also written out in the Android
/// app, the web UI's messages, ui/vite.config.ts (dev proxy) and platformio.ini (upload_port).
constexpr const char* Hostname = "matrixcontroller";
