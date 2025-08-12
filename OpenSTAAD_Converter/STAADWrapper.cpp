#include "STAADWrapper.h"
#include "SAP2000Parser.h"
#include <comutil.h>
#include <comdef.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#import "C:\\Program Files\\Bentley\\Engineering\\STAAD.Pro CONNECT Edition\\STAAD\\STAADPro.dll" \
    named_guids

using namespace std;
using namespace OpenSTAADUI;

map<int, int> STAADWrapper::nodeIdMap_;

IOpenSTAADUIPtr STAADWrapper::Initialize(const std::wstring& filePath, int lenUnit, int forceUnit) {
    IOpenSTAADUIPtr staadApp;
    try {
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        staadApp.CreateInstance(__uuidof(OpenSTAAD));
        staadApp.GetActiveObject(__uuidof(OpenSTAAD));

        _variant_t varFileName(filePath.c_str());
        _variant_t varLengthUnit(lenUnit);
        _variant_t varForceUnit(forceUnit);

        staadApp->NewSTAADFile(varFileName, varLengthUnit, varForceUnit);
    }
    catch (_com_error& e) {
        cerr << "STAAD Error: " << e.ErrorMessage() << endl;
        return nullptr;
    }
    return staadApp;
}

bool STAADWrapper::CreateNodes(IOSGeometryUIPtr geometry, vector<Node>& nodes) {
    try {
        for (auto& node : nodes) {
            _variant_t varX, varY, varZ, varSapId;
            varX.vt = VT_R8; varX.dblVal = node.x;
            varY.vt = VT_R8; varY.dblVal = node.y;
            varZ.vt = VT_R8; varZ.dblVal = node.z;
            varSapId.vt = VT_I4; varSapId.lVal = node.sapId;
            geometry->CreateNode(varSapId, varX, varY, varZ);
            VARIANT result = geometry->GetNodeUniqueID(varSapId);
            if (result.vt == VT_BSTR && result.bstrVal != NULL && SysStringLen(result.bstrVal) > 0) {
                node.staadId = varSapId;
                nodeIdMap_[node.sapId] = node.staadId;
            }
        }
        return true;
    }
    catch (_com_error& e) {
        cerr << "Node Creation Error: " << e.ErrorMessage() << endl;
        return false;
    }
}
bool STAADWrapper::CreateBeams(IOSGeometryUIPtr geometry, vector<Beam>& beams, const map<int, int>& nodeIdMap) {
    try {
        for (size_t i = 0; i < beams.size(); ++i) {  // Use index to track position
            auto& beam = beams[i];
            _variant_t varStart, varEnd, varSapId;
            varStart.vt = VT_I4; varStart.lVal = nodeIdMap.at(beam.startNodeId);
            varEnd.vt = VT_I4; varEnd.lVal = nodeIdMap.at(beam.endNodeId);
            varSapId.vt = VT_I4; varSapId.lVal = beam.sapId;
            geometry->CreateBeam(varSapId, varStart, varEnd);
            VARIANT result = geometry->GetMemberUniqueID(varSapId);
            if (result.vt == VT_BSTR && result.bstrVal != NULL && SysStringLen(result.bstrVal) > 0) {
                beam.staadId = varSapId;
            }
        }
        return true;
    }
    catch (_com_error& e) {
        cerr << "Beam Creation Error: " << e.ErrorMessage() << endl;
        return false;
    }
}

bool STAADWrapper::CreateCables(IOSGeometryUIPtr geometry,
    vector<Cable>& cables,
    const map<int, int>& nodeIdMap) {
    try {
        for (auto& cable : cables) {
            _variant_t varStart, varEnd, varSapId;
            varStart.vt = VT_I4; varStart.lVal = nodeIdMap.at(cable.startNodeId);
            varEnd.vt = VT_I4; varEnd.lVal = nodeIdMap.at(cable.endNodeId);
            varSapId.vt = VT_I4; varSapId.lVal = nodeIdMap.at(cable.sapId);
            geometry->CreateBeam(varSapId, varStart, varEnd);
            VARIANT result = geometry->GetMemberUniqueID(varSapId);
            if (result.vt == VT_BSTR && result.bstrVal != NULL && SysStringLen(result.bstrVal) > 0) {
                cable.staadId = varSapId;
            }
        }
        return true;
    }
    catch (_com_error& e) {
        cerr << "Cable Creation Error: " << e.ErrorMessage() << endl;
        return false;
    }
}

const map<int, int>& STAADWrapper::GetNodeMap() {
    return nodeIdMap_;
}

bool STAADWrapper::CreateSupports(
    OpenSTAADUI::IOSSupportUIPtr Supports,
    const std::vector<JointRestraint>& restraints,
    const std::map<int, int>& nodeIdMap)
{
    if (!Supports) {
        std::cerr << "Invalid COM Supports object" << std::endl;
        return false;
    }

    try {
        std::map<std::tuple<bool, bool, bool, bool, bool, bool>, _variant_t> supportDefinitions;

        for (const auto& restraint : restraints) {
            if (!nodeIdMap.count(restraint.jointId)) {
                std::cerr << "Warning: Joint " << restraint.jointId << " not found in node map" << std::endl;
                continue;
            }

            int staadId = nodeIdMap.at(restraint.jointId);
            auto restraintKey = std::make_tuple(
                restraint.U1, restraint.U2, restraint.U3,
                restraint.R1, restraint.R2, restraint.R3
            );

            // Create or reuse support definition
            if (supportDefinitions.find(restraintKey) == supportDefinitions.end()) {
                SAFEARRAY* psaRelease = nullptr;
                SAFEARRAY* psaSpring = nullptr;
                bool releaseAccessed = false;
                bool springAccessed = false;

                try {
                    // Create and initialize SAFEARRAYs
                    psaRelease = SafeArrayCreateVector(VT_R8, 0, 6);
                    psaSpring = SafeArrayCreateVector(VT_R8, 0, 6);
                    if (!psaRelease || !psaSpring) throw std::bad_alloc();

                    // Access and populate data
                    double* pReleaseData = nullptr;
                    double* pSpringData = nullptr;
                    SafeArrayAccessData(psaRelease, (void**)&pReleaseData);
                    releaseAccessed = true;
                    SafeArrayAccessData(psaSpring, (void**)&pSpringData);
                    springAccessed = true;

                    // Initialize release data
                    pReleaseData[0] = restraint.U1 ? 1.0 : 0.0;
                    pReleaseData[1] = restraint.U2 ? 1.0 : 0.0;
                    pReleaseData[2] = restraint.U3 ? 1.0 : 0.0;
                    pReleaseData[3] = restraint.R1 ? 1.0 : 0.0;
                    pReleaseData[4] = restraint.R2 ? 1.0 : 0.0;
                    pReleaseData[5] = restraint.R3 ? 1.0 : 0.0;

                    // Initialize spring data
                    memset(pSpringData, 0, sizeof(double) * 6);

                    // Unaccess data
                    SafeArrayUnaccessData(psaRelease);
                    releaseAccessed = false;
                    SafeArrayUnaccessData(psaSpring);
                    springAccessed = false;

                    // Create variants
                    _variant_t varReleaseSpec;
                    varReleaseSpec.vt = VT_ARRAY | VT_R8;
                    varReleaseSpec.parray = psaRelease;

                    _variant_t varSpringSpec;
                    varSpringSpec.vt = VT_ARRAY | VT_R8;
                    varSpringSpec.parray = psaSpring;

                    // Determine support type
                    bool allFixed = restraint.U1 && restraint.U2 && restraint.U3 &&
                        restraint.R1 && restraint.R2 && restraint.R3;

                    _variant_t supportId;
                    if (allFixed) {
                        supportId = Supports->CreateSupportFixed();
                    }
                    else {
                        supportId = Supports->CreateSupportFixedBut(varReleaseSpec, varSpringSpec);
                    }

                    if (supportId.vt == VT_I4 && supportId.lVal == -1) {
                        throw _com_error(E_FAIL);
                    }

                    // Store the support definition
                    supportDefinitions[restraintKey] = supportId;

                    // Release ownership to the variant
                    psaRelease = nullptr;
                    psaSpring = nullptr;
                }
                catch (...) {
                    // Cleanup in reverse order of creation
                    if (springAccessed) SafeArrayUnaccessData(psaSpring);
                    if (releaseAccessed) SafeArrayUnaccessData(psaRelease);
                    if (psaSpring) SafeArrayDestroy(psaSpring);
                    if (psaRelease) SafeArrayDestroy(psaRelease);
                    throw;
                }
            }

            // Assign support to node
            _variant_t varNodeId(staadId);
            _variant_t result = Supports->AssignSupportToNode(varNodeId, supportDefinitions[restraintKey]);
            if (result.vt == VT_I4 && result.lVal == -1) {
                std::cerr << "Failed to assign support to node " << staadId << std::endl;
            }
        }
        return true;
    }
    catch (_com_error& e) {
        std::cerr << "COM Error in CreateSupports: " << e.ErrorMessage() << std::endl;
        return false;
    }
    catch (std::exception& e) {
        std::cerr << "STD Error in CreateSupports: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "Unknown error in CreateSupports" << std::endl;
        return false;
    }
}