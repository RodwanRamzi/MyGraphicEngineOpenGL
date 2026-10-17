#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "shaderClass.h"

class BloomPass {
public:
    static constexpr int MIP_COUNT = 6;

    BloomPass(int width, int height);
    ~BloomPass();

    // استقبل الـ brightPassTexture (اللي طالع من MRT في deferred_lighting)
    void Render(GLuint brightPassTexture);

    GLuint GetBloomTexture() const { return m_mipTextures[0]; }

    void Resize(int width, int height);

private:
    int m_width, m_height;

    // 6 mip levels (downsample + upsample في نفس الوقت)
    std::vector<GLuint> m_mipFBOs;
    std::vector<GLuint> m_mipTextures;
    std::vector<glm::vec2> m_mipSizes;

    GLuint m_finalBloomTexture = 0;
    GLuint m_finalBloomFBO = 0;

    GLuint m_quadVAO = 0, m_quadVBO = 0;

    std::unique_ptr<Shader> m_downsampleShader;
    std::unique_ptr<Shader> m_upsampleShader;

    void InitFullscreenQuad();
    void CreateMipChain();
    void DestroyMipChain();
};