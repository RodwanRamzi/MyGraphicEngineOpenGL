#include "Texture.h"
#include <iostream>

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

Texture::Texture(unsigned int id, const char* type, unsigned int slot)
    : ID(id), type(type), unit(slot) {
}

Texture::Texture(const char* image, const char* texType, GLuint slot)
{
    type = std::string(texType);

    int widthImg, heightImg, numColCh;
    stbi_set_flip_vertically_on_load(true);

    std::cout << "[Texture] Loading: " << image << " (type=" << texType << ", slot=" << slot << ")" << std::endl;

    unsigned char* bytes = stbi_load(image, &widthImg, &heightImg, &numColCh, 0);

    if (bytes == nullptr) {
        std::cout << "[Texture] ERROR: Failed to load texture: " << image << std::endl;
        std::cout << "[Texture] stbi_failure_reason: " << stbi_failure_reason() << std::endl;
        unsigned char whitePixel[] = { 255, 255, 255, 255 };
        bytes = stbi_load_from_memory(whitePixel, 4, &widthImg, &heightImg, &numColCh, 4);
        if (bytes == nullptr) {
            std::cerr << "[Texture] CRITICAL: Cannot even create white texture!" << std::endl;
            return;
        }
    }

    std::cout << "[Texture] Loaded " << image << " : " << widthImg << "x" << heightImg << ", channels=" << numColCh << std::endl;

    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    unit = slot;
    glBindTexture(GL_TEXTURE_2D, ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // ============================================================
    //              ✅ FIX: NORMAL MAP FORMAT MATCHING
    // ============================================================
    if (type == "normal") {
        if (numColCh == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthImg, heightImg, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
        }
        else if (numColCh == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthImg, heightImg, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
        }
        else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthImg, heightImg, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
        }
    }
    else if (type == "displacement") {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, widthImg, heightImg, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);
    }
    else if (numColCh == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, widthImg, heightImg, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    }
    else if (numColCh == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, widthImg, heightImg, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
    }
    else if (numColCh == 1) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, widthImg, heightImg, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);
    }
    else {
        throw std::invalid_argument("Automatic Texture type recognition failed");
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    // Anisotropic Filtering
    GLfloat maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
    if (maxAniso > 0.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        std::cout << "[Texture] Anisotropic filtering enabled: " << maxAniso << "x" << std::endl;
    }

    stbi_image_free(bytes);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::texUnit(Shader& shader, const char* uniform, GLuint unit)
{
    GLuint texUni = glGetUniformLocation(shader.ID, uniform);
    shader.Activate();
    glUniform1i(texUni, unit);
}

void Texture::Bind()
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Delete()
{
    glDeleteTextures(1, &ID);
}