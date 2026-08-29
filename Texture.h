#pragma once
#include <glad/glad.h>
#include <stb/stb_image.h>  // ✅ MUST BE HERE to find stbi_load!
#include <string>
#include "shaderClass.h"

class Texture {
public:
    Texture() : ID(0), unit(0), type("") {}  // ✅ Default constructor

    Texture(const char* image, const char* texType, GLuint slot);

    std::string type;
    GLuint ID;
    GLuint unit;

    void texUnit(Shader& shader, const char* uniform, GLuint unit);
    void Bind();
    void Unbind();
    void Delete();
};