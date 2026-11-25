#include "Nvs.h"

#include <Preferences.h>

namespace Nvs {
    std::string read(const char* space, const char* key) {
        Preferences prefs;
        // Opened read-write so a missing namespace is created instead of logged as an error;
        // isKey() avoids the library's error log for keys that were never saved
        prefs.begin(space, false);
        std::string value = prefs.isKey(key) ? prefs.getString(key).c_str() : "";
        prefs.end();
        return value;
    }

    void write(const char* space, const char* key, const std::string& value) {
        Preferences prefs;
        prefs.begin(space, false);
        prefs.putString(key, value.c_str());
        prefs.end();
    }

    void remove(const char* space, const char* key) {
        Preferences prefs;
        prefs.begin(space, false);
        prefs.remove(key);
        prefs.end();
    }
}
