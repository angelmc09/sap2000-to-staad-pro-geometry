#include "STAADUtilities.h"
#include <iostream>
#include <thread>

namespace STAADUtilities {

    VARIANT IsGeometryReady(OpenSTAADUI::IOSGeometryUIPtr geometry,
        int testNodeId) {
        VARIANT varX, varY, varZ, testNode, hs;
        VariantInit(&varX);
        VariantInit(&varY);
        VariantInit(&varZ);
        VariantInit(&testNode);
        VariantInit(&hs);
        try {
            varX.vt = VT_R8; varX.dblVal = 1;
            varY.vt = VT_R8; varY.dblVal = 1;
            varZ.vt = VT_R8; varZ.dblVal = 1; 
            testNode.vt = VT_I4;
            testNode.lVal = testNodeId;
            HRESULT hr = geometry->CreateNode(testNode, varX, varY, varZ);
            VariantClear(&varX);
            VariantClear(&varY);
            VariantClear(&varZ);
            hs = geometry->GetNodeCount();
            return hs;
        }
        catch (...) {
            std::cerr << "Exception in IsGeometryReady" << std::endl;
            VARIANT empty;
            VariantInit(&empty);
            empty.vt = VT_EMPTY;
            return empty;
        }
    }

    void WaitForGeometryReady(OpenSTAADUI::IOSGeometryUIPtr geometry,
        int timeoutSeconds,
        int checkIntervalMs,
        int testNodeId) {
        auto start = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(timeoutSeconds);

        std::cerr << "Waiting for STAAD.Pro to initialize";
        while (std::chrono::steady_clock::now() - start < timeout) {
            std::cerr << ".";

            VARIANT result = IsGeometryReady(geometry, testNodeId);

            if (result.intVal == 1.0) {
                std::cerr << " Ready!" << std::endl;
                geometry->DeleteNode(testNodeId);
                VariantClear(&result);
                return;
            }

            VariantClear(&result);

            std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));

        }

        std::cerr << "\nError: STAAD.Pro not ready after "
            << timeoutSeconds << " seconds" << std::endl;
        throw std::runtime_error("STAAD.Pro initialization timeout");
    }

}