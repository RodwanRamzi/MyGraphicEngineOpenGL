#pragma once
#include <glad/glad.h>

class LUT {
public:
    GLuint textureID = 0;
    int size = 32;

    // Called with raw pixel data (loaded by stb_image in Main.cpp)
    bool loadFromMemory(const unsigned char* data, int lutSize = 32);
    void cleanup();
};