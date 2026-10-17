#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    // 16-byte group first (natural alignment)
    glm::vec3 position { 0.0f };   // offset  0, size 12 (padding 4)
    glm::vec3 normal   { 0.0f };   // offset 16, size 12 (padding 4)
    
    glm::vec3 color    { 0.0f };   // offset 32, size 12
    glm::vec3 tangent  { 0.0f };   // offset 44, size 12
    glm::vec3 bitangent{ 0.0f };   // offset 56, size 12
    
    glm::vec2 texUV    { 0.0f };   // offset 68, size  8
    
    glm::ivec4 joints  { 0, 0, 0, 0 };         // offset 76, size 16 (align to 4)
    glm::vec4  weights { 0.0f, 0.0f, 0.0f, 0.0f }; // offset 92, size 16
};
// Total = 108 bytes, أو 112 مع alignment

class VBO {
public:
    GLuint ID = 0;
    VBO() = default;
    VBO(std::vector<Vertex>& vertices);

    void Bind();
    void Unbind();
    void Delete();
};