#include "../../../../extern/soil/SOIL.h"
// #include "../../../Headers/Core/Common/Common.h"
#include "../../../Headers/Core/Common/OpenGLHelper.h"
#include "../../../Headers/Core/ShaderProgram/ShaderProgram.h"
#include <vector>

class DisplayImage : public Scene {
private:
  // GLuint shaderProgram;

  // Shader *shaderPtr = nullptr;

  ShaderProgram *shaderProgramPtr = nullptr;
  // GLuint vertexShader;
  // GLuint fragmentShader;

  // OpenGLHelper::ShaderSource shaderSource;
  OpenGLHelper::Mesh *mehsPtr = nullptr;

  std::vector<GLfloat> vertices = {
      //  Position      Color             Texcoords
      -0.5f, 0.5f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
      0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
      0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
      -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
  };

  std::vector<GLuint> elements = {0, 1, 2, 2, 3, 0};

public:
  DisplayImage(const std::string &name) : Scene(name) {}

  void InitScene(GLFWwindow *window) override {
    // shaderSource = OpenGLHelper::LoadShaderFromSourceFile(
    //   "/home/lax/Coding/GraphicEngine/src/Main/Scenes/ImageTextureScenes/"
    // "Shaders/shader.glsl");

    shaderProgramPtr =
        new ShaderProgram("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                          "ImageTextureScenes/Shaders/shader.glsl");
    shaderProgramPtr->Bind();

    mehsPtr = new OpenGLHelper::Mesh(vertices, elements);
    mehsPtr->Bind();

    GLint posAttrib =
        glGetAttribLocation(shaderProgramPtr->GetProgram(), "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          0);

    GLint colAttrib =
        glGetAttribLocation(shaderProgramPtr->GetProgram(), "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(2 * sizeof(GLfloat)));

    GLint texAttrib =
        glGetAttribLocation(shaderProgramPtr->GetProgram(), "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat),
                          (void *)(5 * sizeof(GLfloat)));

    // Load Texture
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    int width, height;
    unsigned char *image =
        SOIL_load_image("/home/lax/Coding/GraphicEngine/src/Main/Scenes/"
                        "ImageTextureScenes/Assets/sample.png",
                        &width, &height, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, image);
    SOIL_free_image_data(image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  void HandleInput(GLFWwindow *window) override {}

  void Update(float dt) override {}

  void Render() override {
    glBindVertexArray(mehsPtr->vao);
    // glUseProgram(shaderProgram);
    shaderProgramPtr->Bind();

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  ~DisplayImage() {
    // glDeleteProgram(shaderProgram);
    // glDeleteShader(vertexShader);
    // glDeleteShader(fragmentShader);
  }
};
