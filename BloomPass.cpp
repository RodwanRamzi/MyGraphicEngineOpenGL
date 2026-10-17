#include "BloomPass.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

BloomPass::BloomPass(int width, int height)
    : m_width(width), m_height(height)
{
    m_downsampleShader = std::make_unique<Shader>(
        "bloom_downsample.vert", "bloom_downsample.frag");
    m_upsampleShader = std::make_unique<Shader>(
        "bloom_upsample.vert", "bloom_upsample.frag");

    InitFullscreenQuad();
    CreateMipChain();
}

BloomPass::~BloomPass() {
    DestroyMipChain();
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers(1, &m_quadVBO);
}

void BloomPass::InitFullscreenQuad() {
    float quadVertices[] = {
        // pos        // texcoord
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        (void*)(2 * sizeof(float)));

    m_quadVAO = vao;
    m_quadVBO = vbo;
    glBindVertexArray(0);
}

void BloomPass::CreateMipChain() {
    m_mipFBOs.resize(MIP_COUNT);
    m_mipTextures.resize(MIP_COUNT);
    m_mipSizes.resize(MIP_COUNT);

    int w = m_width / 2;
    int h = m_height / 2;

    glGenFramebuffers(MIP_COUNT, m_mipFBOs.data());
    glGenTextures(MIP_COUNT, m_mipTextures.data());

    for (int i = 0; i < MIP_COUNT; ++i) {
        m_mipSizes[i] = glm::vec2(w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, m_mipFBOs[i]);
        glBindTexture(GL_TEXTURE_2D, m_mipTextures[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0,
            GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_mipTextures[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "[Bloom] FBO " << i << " incomplete!\n";

        w = (w > 1) ? w / 2 : 1;
        h = (h > 1) ? h / 2 : 1;
    }

    // Final bloom output (نفس مقاس mip[0] لأنه هي الاخير في الـ upsample)
    glGenFramebuffers(1, &m_finalBloomFBO);
    glGenTextures(1, &m_finalBloomTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, m_finalBloomFBO);
    glBindTexture(GL_TEXTURE_2D, m_finalBloomTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
        (int)m_mipSizes[0].x, (int)m_mipSizes[0].y, 0,
        GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, m_finalBloomTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomPass::DestroyMipChain() {
    glDeleteTextures(MIP_COUNT, m_mipTextures.data());
    glDeleteFramebuffers(MIP_COUNT, m_mipFBOs.data());
    glDeleteTextures(1, &m_finalBloomTexture);
    glDeleteFramebuffers(1, &m_finalBloomFBO);
}

void BloomPass::Render(GLuint brightPassTexture) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(m_quadVAO);

    // ============================================================
    // Pass 1: Downsample Chain
    // ============================================================
    m_downsampleShader->Activate();

    GLuint inputTex = brightPassTexture;

    for (int i = 0; i < MIP_COUNT; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_mipFBOs[i]);
        glViewport(0, 0, (int)m_mipSizes[i].x, (int)m_mipSizes[i].y);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTex);
        glUniform1i(glGetUniformLocation(m_downsampleShader->ID, "srcTexture"), 0);

        // لو مش أول pass، الـ texel size بتاع الـ mip اللي فات
        glm::vec2 srcSize = (i == 0)
            ? glm::vec2(m_width, m_height)
            : m_mipSizes[i - 1];
        glUniform2f(glGetUniformLocation(m_downsampleShader->ID, "srcTexelSize"),
            1.0f / srcSize.x, 1.0f / srcSize.y);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        inputTex = m_mipTextures[i]; // الـ mip الحالي هيبقي input للـ pass اللي بعده
    }

    // ============================================================
    // Pass 2: Upsample Chain (additive blending)
    // ============================================================
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    m_upsampleShader->Activate();

    for (int i = MIP_COUNT - 1; i > 0; --i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_mipFBOs[i - 1]);
        glViewport(0, 0, (int)m_mipSizes[i - 1].x, (int)m_mipSizes[i - 1].y);

        // srcTexture = الـ mip i (الأصغر)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_mipTextures[i]);
        glUniform1i(glGetUniformLocation(m_upsampleShader->ID, "srcTexture"), 0);

        // higherMipTexture = الـ mip i-1 (الأكبر، اللي كنا حسبناه في الـ downsample)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_mipTextures[i - 1]);
        glUniform1i(glGetUniformLocation(m_upsampleShader->ID, "higherMipTexture"), 1);

        glUniform2f(glGetUniformLocation(m_upsampleShader->ID, "srcTexelSize"),
            1.0f / m_mipSizes[i].x, 1.0f / m_mipSizes[i].y);
        glUniform1f(glGetUniformLocation(m_upsampleShader->ID, "filterRadius"), 1.0f);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);

    // ملاحظة: النتيجة النهائية موجودة في m_mipTextures[0]
    // نستخدم m_mipTextures[0] كـ bloom texture نهائي
}

void BloomPass::Resize(int width, int height) {
    if (width == m_width && height == m_height) return;

    m_width = width;
    m_height = height;

    DestroyMipChain();
    CreateMipChain();
}