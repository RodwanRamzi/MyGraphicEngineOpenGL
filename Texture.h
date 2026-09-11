#pragma once
#include <glad/glad.h>
#include <stb/stb_image.h>  
#include <string>
#include "shaderClass.h"

class Texture {
public:
    Texture() : ID(0), unit(0), type("") {}  

    Texture(const char* image, const char* texType, GLuint slot);
    Texture(unsigned int id, const char* type, unsigned int slot);

    std::string path;
    std::string type;
    GLuint ID;
    GLuint unit;

    void texUnit(Shader& shader, const char* uniform, GLuint unit);
    void Bind();
    void Unbind();
    void Delete();
};