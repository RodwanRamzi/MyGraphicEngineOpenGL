//#include "Terrain.h"
//#include <iostream>
//#include <random>
//
//Terrain::Terrain(int size, float scale, float heightScale)
//    : size(size), scale(scale), heightScale(heightScale)
//{
//    std::vector<Vertex> vertices;
//    std::vector<GLuint> indices;
//    heights.resize((size + 1) * (size + 1), 0.0f);
//
//    std::random_device rd;
//    std::mt19937 gen(rd());
//    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
//
//    for (int z = 0; z <= size; z++) {
//        for (int x = 0; x <= size; x++) {
//            float fx = (float)x / size * scale - scale / 2.0f;
//            float fz = (float)z / size * scale - scale / 2.0f;
//
//            // Simple height generation
//            float height = sin(fx * 0.2f) * cos(fz * 0.2f) * heightScale;
//            height += sin(fx * 0.5f + fz * 0.3f) * 0.5f;
//
//            heights[z * (size + 1) + x] = height;
//
//            Vertex vert;
//            vert.position = glm::vec3(fx, height, fz);
//            vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
//            vert.color = glm::vec3(0.2f, 0.5f, 0.2f);
//            vert.texUV = glm::vec2((float)x / size, (float)z / size);
//            vert.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
//            vert.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
//            vertices.push_back(vert);
//        }
//    }
//
//    for (int z = 0; z < size; z++) {
//        for (int x = 0; x < size; x++) {
//            int i0 = z * (size + 1) + x;
//            int i1 = z * (size + 1) + x + 1;
//            int i2 = (z + 1) * (size + 1) + x;
//            int i3 = (z + 1) * (size + 1) + x + 1;
//
//            indices.push_back(i0);
//            indices.push_back(i2);
//            indices.push_back(i1);
//
//            indices.push_back(i1);
//            indices.push_back(i2);
//            indices.push_back(i3);
//        }
//    }
//
//    std::vector<Texture> textures;
//    mesh = Mesh(vertices, indices, textures);
//}
//
//void Terrain::Draw(Shader& shader, Camera& camera) {
//    mesh.Draw(shader, camera);
//}
//
//void Terrain::DrawShadow(Shader& shader, glm::mat4 shadowMatrices[6], int faceIndex) {
//    // Simplified shadow drawing
//    shader.Activate();
//    mesh.VAO.Bind();
//    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
//    mesh.VAO.Unbind();
//}
//
//float Terrain::GetHeightAt(float x, float z) {
//    // Simple height calculation (same as generation)
//    float height = sin(x * 0.2f) * cos(z * 0.2f) * heightScale;
//    height += sin(x * 0.5f + z * 0.3f) * 0.5f;
//    return height;
//}