#include "SAP2000Parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <regex>

using namespace std;

SectionPositions SAP2000Parser::ParseFile(const string& filePath) {
    SectionPositions positions;
    cout << "Opening file: " << filePath << endl; // Debug the path
    ifstream inputFile(filePath);

    if (!inputFile.is_open()) {
        cerr << "ERROR: Failed to open file! Path: " << filePath << endl;
    }
    string line;
    long lineNumber = 1;

    while (getline(inputFile, line)) {
        replace(line.begin(), line.end(), ',', '.');

        if (line.find("JOINT COORDINATES") != string::npos) {
            positions.ijoint = lineNumber;
        }
        else if (line.find("JOINT RESTRAINT ASSIGNMENTS") != string::npos) {
            positions.isupport = lineNumber;
        }
        else if (line.find("JOINT LOADS - FORCE") != string::npos) {
            positions.iforce = lineNumber;
        }
        else if (line.find("CONNECTIVITY - FRAME") != string::npos) {
            positions.iconnection = lineNumber;
        }
        else if (line.find("CONNECTIVITY - AREA") != string::npos) {
            positions.iplate = lineNumber;
        }
        else if (line.find("CONNECTIVITY - CABLE") != string::npos) {
            positions.icable = lineNumber;
        }
        else if (line.find("FRAME LOADS - POINT") != string::npos) {
            positions.iconc = lineNumber;
        }
        else if (line.find("FRAME LOADS - DISTRIBUTED") != string::npos) {
            positions.idistributed = lineNumber;
        }
        else if (line.find("FRAME LOADS - OPEN STRUCTURE WIND") != string::npos) {
            positions.iframewind = lineNumber;
        }
        else if (line.find("FRAME SECTION ASSIGNMENTS") != string::npos) {
            positions.isection = lineNumber;
        }
        else if (line.find("AREA SECTION ASSIGNMENTS") != string::npos) {
            positions.iareasection = lineNumber;
        }
        else if (line.find("AREA LOADS - UNIFORM") != string::npos) {
            positions.iareauload = lineNumber;
        }
        else if (line.find("OPTIONS - COLORS - OUTPUT") != string::npos) {
            break;
        }
        lineNumber++;
    }
    return positions;
}

vector<Node> SAP2000Parser::ExtractNodes(const string& filePath, long startLine) {
    vector<Node> nodes;
    ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        cerr << "ERROR: Failed to open file for node extraction!" << endl;
        return nodes;
    }
    if (startLine == 1) {
        std::cerr << "Zero nodes detected!" << std::endl;
        return std::vector<Node>();
    }
    string line;
    long currentLine = 1;
    while (currentLine < startLine && getline(inputFile, line)) {
        currentLine++;
    }
    while (getline(inputFile, line) && !line.empty()) {
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());
        replace(line.begin(), line.end(), ',', '.');
        if (line.empty()) break;
        if (std::all_of(line.begin(), line.end(), isspace)) break;

        Node node;
        node.sapId = 0;
        node.x = node.y = node.z = 0.0;

        try {
            size_t jointPos = line.find("Joint=");
            if (jointPos != string::npos) {
                size_t endPos = line.find(' ', jointPos);
                string jointVal = line.substr(jointPos + 6, endPos - (jointPos + 6));
                node.sapId = stoi(jointVal);
            }

            // Find XorR= value (X coordinate)
            size_t xPos = line.find("XorR=");
            if (xPos != string::npos) {
                size_t endPos = line.find(' ', xPos);
                string xVal = line.substr(xPos + 5, endPos - (xPos + 5));
                node.x = stod(xVal);
            }

            // Find Y= value
            size_t yPos = line.find(" Y=");
            if (yPos != string::npos) {
                size_t endPos = line.find(' ', yPos + 3);
                string yVal = line.substr(yPos + 3, endPos - (yPos + 3));
                node.y = stod(yVal);
            }

            // Find Z= value
            size_t zPos = line.find(" Z=");
            if (zPos != string::npos) {
                size_t endPos = line.find(' ', zPos + 3);
                if (endPos == string::npos) endPos = line.length();
                string zVal = line.substr(zPos + 3, endPos - (zPos + 3));
                node.z = stod(zVal);
            }

            if (node.sapId != 0) {
                nodes.push_back(node);
            }
        }
        catch (const exception& e) {
            cerr << "Error parsing node line: " << line << endl;
            cerr << "Exception: " << e.what() << endl;
        }
    }
    cout << "Extracted " << nodes.size() << " nodes starting from line " << startLine << endl;
    return nodes;
}
vector<Beam> SAP2000Parser::ExtractBeams(const string& filePath, long startLine,
    const map<int, int>& nodeIdMap) {
    vector<Beam> beams;
    ifstream inputFile(filePath);

    if (!inputFile.is_open()) {
        cerr << "ERROR: Failed to open file for beam extraction!" << endl;
        return beams;
    }

    if (startLine == 1) {
            std::cerr << "Zero beams detected!" << std::endl;
            return std::vector<Beam>();
        }

    string line;
    long currentLine = 1;
        
    // Fast-forward to the start line
    while (currentLine < startLine && getline(inputFile, line)) {
        currentLine++;
    }

    while (getline(inputFile, line) && !line.empty() && (line.find("Frame=") != string::npos)) {

        line.erase(remove(line.begin(), line.end(), '\r'), line.end());
        if (line.empty()) break;
        if (std::all_of(line.begin(), line.end(), isspace)) break;

        Beam beam;
        beam.sapId = 0;
        beam.startNodeId = 0;
        beam.endNodeId = 0;


        try {
            size_t framePos = line.find("Frame=");
            if (framePos != string::npos) {
                size_t endPos = line.find(' ', framePos);
                string frameVal = line.substr(framePos + 6, endPos - (framePos + 6));
                beam.sapId = stoi(frameVal);
            }

            size_t jointIPos = line.find("JointI=");
            if (jointIPos != string::npos) {
                size_t endPos = line.find(' ', jointIPos);
                string jointIVal = line.substr(jointIPos + 7, endPos - (jointIPos + 7));
                beam.startNodeId = stoi(jointIVal);
            }

            size_t jointJPos = line.find("JointJ=");
            if (jointJPos != string::npos) {
                size_t endPos = line.find(' ', jointJPos);
                if (endPos == string::npos) endPos = line.length();
                string jointJVal = line.substr(jointJPos + 7, endPos - (jointJPos + 7));
                beam.endNodeId = stoi(jointJVal);
            }

            if (beam.sapId != 0 && beam.startNodeId != 0 && beam.endNodeId != 0) {
                if (nodeIdMap.empty() ||
                    (nodeIdMap.count(beam.startNodeId) && nodeIdMap.count(beam.endNodeId))) {
                    beams.push_back(beam);
                }
            }
        }
        catch (const exception& e) {
            cerr << "Error parsing beam line: " << line << endl;
            cerr << "Exception: " << e.what() << endl;
        }
    }
    cout << "Extracted " << beams.size() << " beams starting from line " << startLine << endl;
    return beams;
}

vector<Cable> SAP2000Parser::ExtractCables(const string& filePath, long startLine,
    const map<int, int>& nodeIdMap) {
    vector<Cable> cables;
    ifstream inputFile(filePath);

    if (!inputFile.is_open()) {
        cerr << "ERROR: Failed to open file for cable extraction!" << endl;
        return cables;
    }

    if (startLine == 1) {
        std::cerr << "Zero cables detected!" << std::endl;
        return std::vector<Cable>();
    }

    string line;
    long currentLine = 1;

    while (currentLine < startLine && getline(inputFile, line)) {
        currentLine++;
    }

    while (getline(inputFile, line) && !line.empty()) {
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());
        if (line.empty()) break;
        if (std::all_of(line.begin(), line.end(), isspace)) break;

        Cable cable;
        cable.sapId = 0;
        cable.startNodeId = 0;
        cable.endNodeId = 0;

        try {

            size_t framePos = line.find("Cable=");
            if (framePos != string::npos) {
                size_t endPos = line.find(' ', framePos);
                string frameVal = line.substr(framePos + 6, endPos - (framePos + 6));
                cable.sapId = stoi(frameVal);
            }

            size_t jointIPos = line.find("JointI=");
            if (jointIPos != string::npos) {
                size_t endPos = line.find(' ', jointIPos);
                string jointIVal = line.substr(jointIPos + 7, endPos - (jointIPos + 7));
                cable.startNodeId = stoi(jointIVal);
            }

            size_t jointJPos = line.find("JointJ=");
            if (jointJPos != string::npos) {
                size_t endPos = line.find(' ', jointJPos);
                if (endPos == string::npos) endPos = line.length();
                string jointJVal = line.substr(jointJPos + 7, endPos - (jointJPos + 7));
                cable.endNodeId = stoi(jointJVal);
            }

            if (cable.sapId != 0 && cable.startNodeId != 0 && cable.endNodeId != 0) {
                if (nodeIdMap.empty() ||
                    (nodeIdMap.count(cable.startNodeId) && nodeIdMap.count(cable.endNodeId))) {
                    cables.push_back(cable);
                }
            }
        }
        catch (const exception& e) {
            cerr << "Error parsing cable line: " << line << endl;
            cerr << "Exception: " << e.what() << endl;
        }
    }

    cout << "Extracted " << cables.size() << " cable starting from line " << startLine << endl;
    return cables;
}

std::vector<JointRestraint> SAP2000Parser::ExtractJointRestraints(
    const std::string& filePath,
    long startLine
) {
    std::map<int, JointRestraint> restraintsMap;
    std::ifstream inputFile(filePath);

    if (!inputFile.is_open()) {
        std::cerr << "ERROR: Failed to open file for restraint extraction!" << std::endl;
        return std::vector<JointRestraint>();
    }

    if (startLine==1) {
        std::cerr << "Zero restraints detected!" << std::endl;
        return std::vector<JointRestraint>();
    }

    std::string line;
    long currentLine = 1;

    // Skip to start line
    while (currentLine < startLine && getline(inputFile, line)) {
        currentLine++;
    }

    // Parse restraints
    while (getline(inputFile, line) && !line.empty()) {
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());
        if (line.empty() || all_of(line.begin(), line.end(), isspace)) continue;
        if (line.empty()) break;
        if (std::all_of(line.begin(), line.end(), isspace)) break;

        JointRestraint restraint = {};  // Initialize all members to zero/false
        restraint.jointId = 0;         // Explicitly initialize
        restraint.U1 = restraint.U2 = restraint.U3 = false;
        restraint.R1 = restraint.R2 = restraint.R3 = false;

        try {
            // Extract joint ID
            size_t jointPos = line.find("Joint=");
            if (jointPos != std::string::npos) {
                size_t endPos = line.find(' ', jointPos);
                if (endPos != std::string::npos) {
                    restraint.jointId = stoi(line.substr(jointPos + 6, endPos - (jointPos + 6)));
                }
            }

            // Only process if we got a valid joint ID
            if (restraint.jointId > 0) {
                // Extract restraints
                auto parseRestraint = [&line](const std::string& field) {
                    size_t pos = line.find(field + "=");
                    if (pos != std::string::npos) {
                        size_t endPos = line.find(' ', pos);
                        std::string val = line.substr(pos + field.size() + 1, 3); // Get "Yes" or "No"
                        return (val == "Yes");
                    }
                    return false;
                    };

                restraint.U1 = parseRestraint("U1");
                restraint.U2 = parseRestraint("U2");
                restraint.U3 = parseRestraint("U3");
                restraint.R1 = parseRestraint("R1");
                restraint.R2 = parseRestraint("R2");
                restraint.R3 = parseRestraint("R3");

                restraintsMap[restraint.jointId] = restraint;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing restraint line: " << line << std::endl;
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }

    // Convert map to vector
    std::vector<JointRestraint> restraints;
    restraints.reserve(restraintsMap.size());
    for (const auto& pair : restraintsMap) {
        restraints.push_back(pair.second);
    }

    std::cout << "Extracted " << restraints.size() << " joint restraints\n";
    return restraints;
}