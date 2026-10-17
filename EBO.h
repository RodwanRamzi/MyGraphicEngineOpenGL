#pragma once

#include <glad/glad.h>
#include <vector>

class EBO {
public:
    GLuint ID = 0;
    EBO() = default;
    EBO(std::vector<GLuint>& indices);

    void Bind();
    void Unbind();
    void Delete();
};