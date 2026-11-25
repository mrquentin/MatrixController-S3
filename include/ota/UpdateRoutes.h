#pragma once

#ifndef _UPDATEROUTES_h
#define _UPDATEROUTES_h

#include <ESPAsyncWebServer.h>
#include "ota/FirmwareUpdate.h"
#include "ota/NetworkUpload.h"
#include "ota/ReleaseUpdate.h"

/// /api/system/update: the update state, the update password, uploading firmware.bin and, when this
/// firmware has a release repository, installing GitHub releases. Every request that changes the
/// firmware carries the update password in the X-Update-Password header.
void addUpdateRoutes(AsyncWebServer& server, FirmwareUpdate& update, NetworkUpload& networkUpload,
                     ReleaseUpdate& releases);

#endif
