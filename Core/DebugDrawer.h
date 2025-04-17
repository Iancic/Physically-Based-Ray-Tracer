#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include "ShaderClass.h"

#include "btBulletDynamicsCommon.h"

struct DebugDrawer : public btIDebugDraw
{
    ShaderClass* debugShader = new ShaderClass("../assets/shaders", "vDebug.glsl", "fDebug.glsl");
    GLuint lineVAO, lineVBO; // OpenGL handles
    std::vector<float> lineVertices; // Store line vertex data
    int debugMode;

    DebugDrawer() : lineVAO(0), lineVBO(0), debugMode(0)
    {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
    }

    ~DebugDrawer()
    {
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);
    }

    void setShader(Camera* camera)
    {
        glUseProgram(debugShader->ID);

        // VP
        //Perspective
        glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float)SCRWIDTH / (float)SCRHEIGHT, 0.1f, 3000000.0f);

        // View
        glm::mat4 view = glm::lookAtRH(glm::vec3(camera->camPos.x, camera->camPos.y, camera->camPos.z),
                                    glm::vec3(camera->camTarget.x, camera->camTarget.y, camera->camTarget.z),
                                    glm::vec3(0, 1, 0));

        glm::mat4 VP = projection * view;
        debugShader->setMat4("VP", VP);
    }

    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
    {
        lineVertices.push_back(from.getX());
        lineVertices.push_back(from.getY());
        lineVertices.push_back(from.getZ());

        lineVertices.push_back(color.getX());
        lineVertices.push_back(color.getY());
        lineVertices.push_back(color.getZ());

        lineVertices.push_back(to.getX());
        lineVertices.push_back(to.getY());
        lineVertices.push_back(to.getZ());

        lineVertices.push_back(color.getX());
        lineVertices.push_back(color.getY());
        lineVertices.push_back(color.getZ());

    }

    void setDebugMode(int mode) override
    {
        debugMode = mode;
    }

    int getDebugMode() const override
    {
        return debugMode;
    }

    void clearLines()
    {
        lineVertices.clear();
    }

    void flushLines() override
    {
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), &lineVertices[0], GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size() / 6));

        glBindVertexArray(0);

        clearLines();
    }

    void drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&) override {}
    void reportErrorWarning(const char* warningString) override
    {
        std::cerr << "Bullet Warning: " << warningString << std::endl;
    }

    void draw3dText(const btVector3&, const char*) override {}
};