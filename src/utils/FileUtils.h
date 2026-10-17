#pragma once
#include <string>
#include <GLFW/glfw3.h>
std::string readFile(const std::string& filename);
std::string OpenFileDialog(const std::string& title,
                            const std::string& filter = "All Files\0*.*\0",
                            GLFWwindow* win = NULL);