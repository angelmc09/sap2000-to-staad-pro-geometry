#pragma once
#include <string>
#include <vector>
#include <map>
#include "SAP2000Parser.h"

#import "C:\\Program Files\\Bentley\\Engineering\\STAAD.Pro CONNECT Edition\\STAAD\\STAADPro.dll" \
    named_guids

class STAADWrapper {
public:
    static OpenSTAADUI::IOpenSTAADUIPtr Initialize(const std::wstring& filePath, int lenUnit, int forceUnit);
    static bool CreateNodes(OpenSTAADUI::IOSGeometryUIPtr geometry, std::vector<Node>& nodes);
    static bool CreateBeams(OpenSTAADUI::IOSGeometryUIPtr geometry,
        std::vector<Beam>& beams,
        const std::map<int, int>& nodeIdMap);
    static bool CreateCables(OpenSTAADUI::IOSGeometryUIPtr geometry,
        std::vector<Cable>& cables,
        const std::map<int, int>& nodeIdMap);
    static const std::map<int, int>& GetNodeMap();
    static bool STAADWrapper::CreateSupports(
        OpenSTAADUI::IOSSupportUIPtr Supports,
        const std::vector<JointRestraint>& restraints,
        const std::map<int, int>& nodeIdMap);
private:
    static std::map<int, int> nodeIdMap_;
};


