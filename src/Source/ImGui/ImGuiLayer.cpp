#include "../../Headers/ImGui/ImGuiLayer.h"
#include "../../../extern/imgui/backend/imgui_impl_glfw.h"
#include "../../../extern/imgui/backend/imgui_impl_opengl3.h"
#include "../../../extern/imgui/imgui.h"
#include <GLFW/glfw3.h>

ImGuiLayer::ImGuiLayer(GLFWwindow *window, float mainScale,
                       const char *glslVersion)
    : _window(window), _mainScale(mainScale), _glslVersion(glslVersion) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(_window, true);
  ImGui_ImplOpenGL3_Init(glslVersion);
}

ImGuiLayer::~ImGuiLayer() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::NewFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::Render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
