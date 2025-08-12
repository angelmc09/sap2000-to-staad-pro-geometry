#pragma once
#include <chrono>
#include <comutil.h>
#import "C:\\Program Files\\Bentley\\Engineering\\STAAD.Pro CONNECT Edition\\STAAD\\STAADPro.dll" \
    named_guids

namespace STAADUtilities {
    VARIANT IsGeometryReady(
        OpenSTAADUI::IOSGeometryUIPtr geometry,
        int testNodeId = 1
    );
    void WaitForGeometryReady(
        OpenSTAADUI::IOSGeometryUIPtr geometry,
        int timeoutSeconds = 10,
        int checkIntervalMs = 100,
        int testNodeId = 1
    );
}