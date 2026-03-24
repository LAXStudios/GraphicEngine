#pragma once
#include "../../../Headers/Core/Common/Common.h"
#include <GL/gl.h>
#include <cstddef>

class PolygonScene : public Scene {
private:
  VertexArray *vertexArrayPtr = nullptr;
  VertexBuffer *vertexBufferPtr = nullptr;

  ShaderProgram *shaderProgramPtr = nullptr;

public:
  PolygonScene(const std::string &name) : Scene(name) {}

  void InitScene() override {

    std::vector<float> verties{0.0f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f};

    shaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "PolygonScene/Shader/shader.glsl");
    shaderProgramPtr->Bind();

    GLuint progID = shaderProgramPtr->GetID();
    GLint linked = 0;
    glGetProgramiv(progID, GL_LINK_STATUS, &linked);
    if (!linked) {
      char log[512];
      glGetProgramInfoLog(progID, 512, nullptr, log);
      std::cerr << "Program link failed: " << log << '\n';
    }
    std::cout << "Shader program linked, ID = " << progID << '\n';

    vertexArrayPtr = new VertexArray();
    vertexBufferPtr = new VertexBuffer(
        verties.data(), static_cast<GLuint>(verties.size() * sizeof(float)));
    vertexArrayPtr->Bind();
    vertexBufferPtr->Bind();

    VertexBufferLayout layout;
    layout.AddElement<float>(2);
    vertexArrayPtr->AddBuffer(*vertexBufferPtr, layout);

    GLint enabled = 0;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    std::cout << "Attrib 0 enabled = " << enabled << '\n';

    void *ptr = nullptr;
    GLint size = 0, type = 0, normalized = 0, stride = 0;
    glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &ptr);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
    std::cout << "Attrib 0 -> size:" << size << " type:" << type
              << " norm:" << normalized << " stride:" << stride << '\n';

    vertexArrayPtr->UnBind();
    vertexBufferPtr->UnBind();

    glCall(glUseProgram(shaderProgramPtr->GetID()));
    shaderProgramPtr->setUniform3f("triangleColor", 1.0f, 0.0f, 0.0f);
    int loc = glGetUniformLocation(shaderProgramPtr->GetID(), "triangleColor");
    std::cout << "triangleColor location: " << loc << '\n';

    std::cout << "[PolygonScene] InitScene() called\n";
  }

  void HandleInput(GLFWwindow *window) override {}

  void Update(float dt) override {
    shaderProgramPtr->Bind();
    shaderProgramPtr->setUniform3f("triangleColor", 1.0f, 0.0f, 0.0f);

    std::cout << "Binding VAO: " << vertexArrayPtr->GetID()
              << "\nand VBO: " << vertexBufferPtr->GetId() << "\n";
    vertexArrayPtr->Bind();
    shaderProgramPtr->Bind();
    GLint currentProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProg);
    std::cout << "Current program before draw = " << currentProg << '\n';

    glCall(glDrawArrays(GL_TRIANGLES, 0, 3));

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      std::cerr << "GL error after Draw: " << err << '\n';
    }
  }

  void Render() override {}

  ~PolygonScene() override {}
};
