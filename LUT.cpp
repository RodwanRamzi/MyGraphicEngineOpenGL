#include "LUT.h"
#include <iostream>

bool LUT::loadFromMemory(const unsigned char* data, int lutSize) {
    size = lutSize;

    if (!data) {
        std::cerr << "[LUT] Null data" << std::endl;
        return false;
    }

    if (textureID != 0) glDeleteTextures(1, &textureID);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, size, size, size,
        0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);

    std::cout << "[LUT] Created 3D texture (" << size << "^3)" << std::endl;
    return true;
}

void LUT::cleanup() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}