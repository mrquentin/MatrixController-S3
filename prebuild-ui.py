import os
import hashlib

Import("env")

# Builds the web UI into include/ui/WebAssets.h (npm run build:full) when its sources changed since
# the last build, which hash.txt remembers.
UI = "ui"
SOURCES = ["src", "public", "index.html", "package-lock.json", "vite.config.ts", "hexdump.py"]
HASH_FILE = "hash.txt"


def sources_hash():
    digest = hashlib.sha256()
    for source in SOURCES:
        path = os.path.join(UI, source)
        files = [path] if os.path.isfile(path) else sorted(
            os.path.join(root, name) for root, _, names in os.walk(path) for name in names)
        for file in files:
            digest.update(file.replace(os.sep, "/").encode())
            with open(file, "rb") as f:
                digest.update(f.read())
    return digest.hexdigest()


current = sources_hash()
previous = open(HASH_FILE).read().strip() if os.path.exists(HASH_FILE) else None
if current != previous or not os.path.exists(os.path.join("include", "ui", "WebAssets.h")):
    print("Web UI changed, building it")
    if not os.path.exists(os.path.join(UI, "node_modules")) and env.Execute(f"npm ci --prefix {UI}") != 0:
        env.Exit(1)
    if env.Execute(f"npm run build:full --prefix {UI}") != 0:
        env.Exit(1)
    with open(HASH_FILE, "w") as f:
        f.write(current)
