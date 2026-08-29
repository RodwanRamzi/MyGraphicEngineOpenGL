// GBuffer.h
#pragma once
#include <glad/glad.h>
#include <vector>
#include <iostream>

class GBuffer {
public:
    unsigned int fbo;
    std::vector<unsigned int> colorTextures; // [0]=Pos, [1]=Normal, [2]=Albedo
    unsigned int rboDepth; // Depth-Stencil buffer

    GBuffer(int width, int height) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // --- Texture 1: Position (RGB) ---
        unsigned int posTex = createTexture(width, height, GL_RGBA16F);
        // --- Texture 2: Normal (RGB) ---
        unsigned int normTex = createTexture(width, height, GL_RGBA16F);
        // --- Texture 3: Albedo + Specular (RGBA) ---
        unsigned int albTex = createTexture(width, height, GL_RGBA8);

        colorTextures = { posTex, normTex, albTex };

        // Attach textures to FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, posTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albTex, 0);

        // Tell OpenGL which attachments to draw to
        unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, attachments);

        // --- Depth/Stencil Renderbuffer ---
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

        // ✅ THE "FREEZE KILLER" CHECK
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR: Framebuffer is INCOMPLETE! (Freeze incoming if you ignore this)" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    unsigned int createTexture(int w, int h, GLenum internalFormat) {
        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return tex;
    }

    void BindForWriting() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, 1024, 720); // or your width/height
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void BindForReading() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, colorTextures[0]); // Position
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, colorTextures[1]); // Normal
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, colorTextures[2]); // Albedo
    }
};