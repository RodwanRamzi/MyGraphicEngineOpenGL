#include "FileUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <commdlg.h>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) { std::cerr << "Failed to open: " << filename << std::endl; return ""; }
    std::stringstream buffer; buffer << file.rdbuf(); return buffer.str();
}
std::string OpenFileDialog(const std::string& title, const std::string& filter, GLFWwindow* win) {
    (void)win;
    OPENFILENAMEA ofn = {};
    char fileName[MAX_PATH] = "";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = title.c_str();
    if (GetOpenFileNameA(&ofn)) return std::string(fileName);
    return "";
}