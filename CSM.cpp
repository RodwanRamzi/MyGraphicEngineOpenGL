#include "CSM.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

CSM::CSM() { Init(); }

CSM::~CSM()
{
    if (mDepthArray) glDeleteTextures(1, &mDepthArray);
    if (mFBO)        glDeleteFramebuffers(1, &mFBO);
}

void CSM::Init()
{
    // ---- Depth texture array ----
    glGenTextures(1, &mDepthArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, mDepthArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                 SHADOW_SIZE, SHADOW_SIZE, NUM_CASCADES,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border);

    // ---- FBO ----
    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mDepthArray, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[CSM] Framebuffer incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    std::cout << "[CSM] Initialized: " << NUM_CASCADES
              << " cascades @ " << SHADOW_SIZE << "x" << SHADOW_SIZE << std::endl;
}

void CSM::computeSplits(float nearP, float farP)
{
    for (int i = 1; i <= NUM_CASCADES; ++i) {
        float p = (float)i / NUM_CASCADES;
        float logSplit = nearP * std::pow(farP / nearP, p);
        float uniSplit = nearP + (farP - nearP) * p;
        mSplitDepths[i - 1] = mCascadeLambda * logSplit + (1.0f - mCascadeLambda) * uniSplit;
    }
}

glm::mat4 CSM::computeLightMatrix(const glm::mat4& camView,
                                  float fovRad, float aspect,
                                  float splitNear, float splitFar,
                                  const glm::vec3& lightDir)
{
    glm::mat4 invView = glm::inverse(camView);
    float tanHalf = std::tan(fovRad * 0.5f);

    // 8 corners of the frustum slice (view space → world)
    glm::vec3 corners[8];
    int idx = 0;
    for (int x = -1; x <= 1; x += 2)
    for (int y = -1; y <= 1; y += 2)
    for (int z = 0; z <= 1; ++z) {
        float d = (z == 0) ? splitNear : splitFar;
        glm::vec4 pt(x * tanHalf * aspect * d,
                     y * tanHalf * d,
                     -d, 1.0f);
        corners[idx++] = glm::vec3(invView * pt);
    }

    // center + bounding sphere
    glm::vec3 center(0.0f);
    for (auto& c : corners) center += c;
    center /= 8.0f;

    float radius = 0.0f;
    for (auto& c : corners)
        radius = glm::max(radius, glm::length(c - center));
    radius = std::ceil(radius * 16.0f) / 16.0f;

    // ============================================================
    //  TEXEL SNAPPING
    // ============================================================
    glm::vec3 up = (std::abs(lightDir.y) > 0.99f)
                   ? glm::vec3(1, 0, 0)
                   : glm::vec3(0, 1, 0);
    glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), -lightDir, up);

    // Transform center to light view space
    glm::vec4 centerLS = lightView * glm::vec4(center, 1.0f);

    // World units per texel
    float texelSize = (radius * 2.0f) / float(SHADOW_SIZE);

    // Snap
    centerLS.x = std::floor(centerLS.x / texelSize) * texelSize;
    centerLS.y = std::floor(centerLS.y / texelSize) * texelSize;

    // Back to world
    glm::vec4 centerWS = glm::inverse(lightView) * centerLS;
    center = glm::vec3(centerWS);

    // ============================================================
    //  Final light matrices
    // ============================================================
    glm::mat4 lightViewFinal = glm::lookAt(center - lightDir * (radius * 2.0f),
                                           center, up);
    glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius,
                                     0.1f, radius * 4.0f);

    return lightProj * lightViewFinal;
}

void CSM::Update(const glm::mat4& cameraView,
                 const glm::mat4& cameraProj,
                 float fovDegrees, float aspect,
                 float nearPlane, float farPlane,
                 const glm::vec3& lightDir)
{
    if (!mEnabled) return;

    computeSplits(nearPlane, farPlane);

    float fovRad = glm::radians(fovDegrees);
    float prevSplit = nearPlane;
    for (int i = 0; i < NUM_CASCADES; ++i) {
        mLightMatrices[i] = computeLightMatrix(cameraView, fovRad, aspect,
                                               prevSplit, mSplitDepths[i], lightDir);
        prevSplit = mSplitDepths[i];
    }
}

void CSM::BindForWriting(int cascadeIndex)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              mDepthArray, 0, cascadeIndex);
    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void CSM::BindForReading(GLuint textureUnit)
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, mDepthArray);
}