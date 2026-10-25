#include "ota/Version.h"

#include <cctype>

namespace {
    /// Reads digits at p into value and moves p past them. False if there are none.
    bool readNumber(const char*& p, int& value) {
        if (!isdigit(static_cast<unsigned char>(*p))) return false;
        value = 0;
        while (isdigit(static_cast<unsigned char>(*p))) value = value * 10 + (*p++ - '0');
        return true;
    }

    int compare(const Version::Semver& a, const Version::Semver& b) {
        if (a.major != b.major) return a.major < b.major ? -1 : 1;
        if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
        if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
        return 0;
    }
}

namespace Version {
    bool parse(const char* text, Semver& out) {
        if (!text) return false;
        const char* p = text;
        if (*p == 'v' || *p == 'V') ++p;

        Semver version;
        if (!readNumber(p, version.major) || *p++ != '.') return false;
        if (!readNumber(p, version.minor) || *p++ != '.') return false;
        if (!readNumber(p, version.patch)) return false;
        version.release = *p == '\0';
        out = version;
        return true;
    }

    bool isNewer(const char* candidate, const char* current) {
        Semver latest;
        if (!parse(candidate, latest) || !latest.release) return false;

        Semver running;
        if (!parse(current, running)) return true;
        return compare(latest, running) > 0;
    }
}
