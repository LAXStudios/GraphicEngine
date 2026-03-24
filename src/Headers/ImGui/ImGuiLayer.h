#pragma once
#include <GLFW/glfw3.h>

class ImGuiLayer {
private:
  GLFWwindow *_window;
  const char *_glslVersion;
  float _mainScale;

public:
  ImGuiLayer(GLFWwindow *window, float mainScale = 0.0f,
             const char *glslVersion = "#version 150");
  ~ImGuiLayer();
  void NewFrame();
  void Render();
};
