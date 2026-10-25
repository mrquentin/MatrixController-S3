#pragma once

#ifndef _VERSION_h
#define _VERSION_h

/// Compares firmware versions as version.py writes them: release builds are their tag ("v1.2.3"),
/// other builds what git describe says ("v1.2.3-4-gabc1234", "v1.2.3+20261025-1200", or just a
/// commit like "a25165f" before the first release). No Arduino dependencies, so it's unit tested.
namespace Version {
    struct Semver {
        int major = 0;
        int minor = 0;
        int patch = 0;
        /// False when something follows the patch number: a build after that release
        bool release = true;
    };

    /// Reads "v1.2.3" or "1.2.3" and whatever follows. Returns false if it doesn't start like one.
    bool parse(const char* text, Semver& out);

    /// True if the release candidate is newer than the running version current. Builds made after a
    /// release (a suffix) already contain it, so only a higher version counts. When current isn't
    /// based on any release, every release is newer. candidate must be a plain release version.
    bool isNewer(const char* candidate, const char* current);
}

#endif
