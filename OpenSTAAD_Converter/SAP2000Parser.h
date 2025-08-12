#pragma once
#include <string>
#include <vector>
#include <map>

struct SectionPositions {
    long ijoint = 0;
    long icable = 0;
    long iconnection = 0;
    long iplate = 0;
    long iforce = 0;
    long iconc = 0;
    long iload = 0;
    long isection = 0;
    long irelease = 0;
    long isupport = 0;
    long idistributed = 0;
    long iareasection = 0;
    long iareauload = 0;
    long iframewind = 0;
};

struct Node {
    int sapId = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int staadId = 0;
};

struct Beam {
    int sapId = 0;
    int startNodeId = 0;
    int endNodeId = 0;
    int staadId = 0;
};

struct Cable {
    int sapId = 0;
    int startNodeId = 0;
    int endNodeId = 0;
    int staadId = 0;
};

struct JointRestraint {
    int jointId;
    bool U1, U2, U3; // Translation restraints
    bool R1, R2, R3; // Rotation restraints
};

namespace SAP2000Parser {
    SectionPositions ParseFile(const std::string& filePath);
    std::vector<Node> ExtractNodes(const std::string& filePath, long startLine);
    std::vector<Beam> ExtractBeams(const std::string& filePath, long startLine, const std::map<int, int>& nodeIdMap);
    std::vector<Cable> ExtractCables(const std::string& filePath, long startLine, const std::map<int, int>& nodeIdMap);
    std::vector<JointRestraint> ExtractJointRestraints(const std::string& filePath, long startLine);
}