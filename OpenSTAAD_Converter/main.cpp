#define NOMINMAX
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <windows.h>
#include <comdef.h>
#include <iostream>
#include <string>
#include <filesystem>
#include "SAP2000Parser.h"
#include "STAADWrapper.h"
#include "STAADUtilities.h"

namespace fs = std::filesystem;
using namespace std;

std::wstring UTF8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
    wide.resize(size - 1);
    return wide;
}

bool FileExistsWinAPI(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

void PrintLengthUnits() {
    cout << "\nLength units available:\n";
    cout << "0: Inch\n1: Feet\n2: Feet\n3: Centimeter\n4: Meter\n5: Millimeter\n6: Decimeter\n7: Kilometer\n";
}

void PrintForceUnits() {
    cout << "\nForce units available:\n";
    cout << "0: Kilopound\n1: Pound\n2: Kilogram\n3: Metric Ton\n4: Newton\n5: Kilo Newton\n6: Mega Newton\n7: DecaNewton\n";
}

int main(int argc, char* argv[]) {
    std::cerr << "\nCreated by Angel Monroy Canales\n";
    std::cerr << "Contact info: monolitoingenieria@gmail.com\n\n";
    std::cerr << "Checking system configuration, please wait...\n";

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM" << std::endl;
        return 1;
    }

    try {
        std::wstring widePath;

        if (argc >= 2) {
            widePath = UTF8ToWide(argv[1]);
        }
        else {
            std::wcout << L"Enter SAP2000 .$2k file path: ";
            std::wstring input;
            std::getline(std::wcin, input);
            widePath = input;
        }

        if (!widePath.empty() && widePath.front() == L'"' && widePath.back() == L'"') {
            widePath = widePath.substr(1, widePath.size() - 2);
        }

        std::wcout << L"Processing file: " << widePath << std::endl;

        if (!FileExistsWinAPI(widePath)) {
            std::wcerr << L"ERROR: File not found (WinAPI check failed)\n";

            wchar_t shortPath[MAX_PATH];
            if (GetShortPathNameW(widePath.c_str(), shortPath, MAX_PATH)) {
                std::wcout << L"Trying short path: " << shortPath << std::endl;
                if (FileExistsWinAPI(shortPath)) {
                    widePath = shortPath;
                }
                else {
                    std::wcerr << L"Short path also not found\n";
                }
            }
            else {
                std::wcerr << L"Could not get short path name\n";
            }

            std::wcerr << L"Last error: " << GetLastError() << std::endl;
            std::wcerr << L"Attempted path: " << widePath << std::endl;
            return 1;
        }

        fs::path filePath(widePath);

        fs::path outputDir = R"(C:\temp)";
        if (!fs::exists(outputDir)) {
            if (!fs::create_directory(outputDir)) {
                std::cerr << "Failed to create output directory at C:\\temp" << std::endl;
                CoUninitialize();
                return 1;
            }
        }

        auto sections = SAP2000Parser::ParseFile(filePath.string());
        auto nodes = SAP2000Parser::ExtractNodes(filePath.string(), sections.ijoint + 1);
        auto beams = SAP2000Parser::ExtractBeams(filePath.string(), sections.iconnection + 1, {});
        auto cables = SAP2000Parser::ExtractCables(filePath.string(), sections.icable + 1, {});
        auto restraints = SAP2000Parser::ExtractJointRestraints(filePath.string(), sections.isupport + 1);

        std::string outputName = filePath.stem().string() + ".std";
        fs::path outputPath = outputDir / outputName;

        int lenUnit = -1;
        int forceUnit = -1;

        PrintLengthUnits();
        cout << "Enter length unit from file input: ";
        cin >> lenUnit;
        if (lenUnit < 0 || lenUnit > 7) {
            cerr << "Invalid option.\n";
            return 1;
        }

        PrintForceUnits();
        cout << "Enter force unit from file input: ";
        cin >> forceUnit;
        if (forceUnit < 0 || forceUnit > 7) {
            cerr << "Invalid option.\n";
            return 1;
        }

        auto staadApp = STAADWrapper::Initialize(outputPath.wstring(), lenUnit, forceUnit);
        if (staadApp == nullptr) {
            cerr << "Failed to initialize STAAD" << endl;
            CoUninitialize();
            return 1;
        }

        OpenSTAADUI::IOSGeometryUIPtr geometry = staadApp->GetGeometry();
        //staadApp->SetSilentMode(1);

        try {
            STAADUtilities::WaitForGeometryReady(geometry);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
        if (!STAADWrapper::CreateNodes(geometry, nodes)) {
            std::cerr << "Failed to create nodes" << std::endl;
        }

        if (!STAADWrapper::CreateBeams(geometry, beams, STAADWrapper::GetNodeMap())) {
            std::cerr << "Failed to create beams" << std::endl;
        }

        if (!STAADWrapper::CreateCables(geometry, cables, STAADWrapper::GetNodeMap())) {
            std::cerr << "Failed to create cables" << std::endl;
        }
        /*
        if (!STAADWrapper::CreatePlates(geometry, plates, STAADWrapper::GetNodeMap())) {
            std::cerr << "Failed to create plates" << std::endl;
        }
        */

        OpenSTAADUI::IOSSupportUIPtr Supports = staadApp->GetSupport();

        if (!STAADWrapper::CreateSupports(Supports, restraints, STAADWrapper::GetNodeMap())) {
            std::cerr << "Failed to create supports" << std::endl;
        }

        std::wcout << L"Conversion complete! Saved to: " << outputPath.wstring() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
    }

    CoUninitialize();
    std::cout << "Program finished successfully.\n";
    system("pause");
    return 0;
}