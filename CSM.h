#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>

class CSM {
public:
    static constexpr int NUM_CASCADES = 4;
    static constexpr int SHADOW_SIZE  = 2048;

    // ============================================================
    //  UI CONTROLS — public عشان Main.cpp يقدر يعدلها من ImGui
    // ============================================================
    bool  mEnabled       = true;
    float mCascadeLambda = 0.5f;   // 0 = uniform, 1 = logarithmic

    CSM();
    ~CSM();

    void Init();

    void Update(const glm::mat4& cameraView,
                const glm::mat4& cameraProj,
                float fovDegrees,
                float aspect,
                float nearPlane,
                float farPlane,
                const glm::vec3& lightDir);

    void BindForWriting(int cascadeIndex);
    void BindForReading(GLuint textureUnit = 5);

    const std::array<glm::mat4, NUM_CASCADES>& GetLightMatrices() const { return mLightMatrices; }
    const std::array<float,     NUM_CASCADES>& GetSplitDepths()   const { return mSplitDepths;   }
    GLuint GetDepthArray() const { return mDepthArray; }

private:
    GLuint mFBO        = 0;
    GLuint mDepthArray = 0;

    std::array<glm::mat4, NUM_CASCADES> mLightMatrices;
    std::array<float,     NUM_CASCADES> mSplitDepths;

    void computeSplits(float nearP, float farP);

    glm::mat4 computeLightMatrix(const glm::mat4& camView,
                                 float fovRad, float aspect,
                                 float splitNear, float splitFar,
                                 const glm::vec3& lightDir);
};