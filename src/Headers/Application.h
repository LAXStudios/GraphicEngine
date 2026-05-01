#pragma once
#include "../Headers/Core/Common/Common.h"
#include "Core/Common/ErrorHandling.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "imgui.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <utility>
#include <vector>
#define NOTHING
#include "../Headers/ImGui/ImGuiLayer.h"
#include <cstdio>
#include <map>
#include <memory>
#include <string>

inline static void glfwErrorCallback(int error, const char *description) {
  std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

class Application {
private:
  GLFWwindow *_window{nullptr};
  SceneManager sceneManager{};

  ImGuiLayer *imGuiLayer;

  int frameCount;
  double lastTime;
  double fps;

  ImVec4 clearColor = ImVec4(0.129f, 0.145f, 0.160f, 1.00f);

  // Kategorie | Scenes
  std::vector<std::pair<std::string, std::vector<Scene *>>> groupedScenes;

  static void keyCallbackStatic(GLFWwindow *window, int key, int scancode,
                                int action, int mods) {
    Application *app =
        static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
      app->keyCallback(window, key, scancode, action, mods);
  }

  void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                   int mods) {

    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard)
      return;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    auto scene = sceneManager.getCurrent();
    scene->HandleInput(window, key, scancode, action, mods);
  }

  std::vector<std::pair<std::string, std::vector<Scene *>>>
  createGroupedScenesVector() {
    std::map<std::string, std::vector<Scene *>> grouped;
    std::vector<std::string> order;

    for (auto &scene : sceneManager.getScenesAsVector()) {
      const std::string &cat = scene->category();
      if (grouped.find(cat) == grouped.end()) {
        order.push_back(cat);
      }
      grouped[cat].push_back(scene.get());
    }

    std::vector<std::pair<std::string, std::vector<Scene *>>> result;
    for (const auto &cat : order) {
      result.emplace_back(cat, grouped[cat]);
    }
    return result;
  }

  static void mouseButtonCallbackStatic(GLFWwindow *window, int button,
                                        int action, int mods) {
    Application *app =
        static_cast<Application *>(glfwGetWindowUserPointer(window));

    if (app)
      app->mouseButtonCallback(window, button, action, mods);
  }

  void mouseButtonCallback(GLFWwindow *window, int button, int action,
                           int mods) {

    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse)
      return;
  }

  static void mouseCursorPosCallbackStatic(GLFWwindow *window, double xpos,
                                           double ypos) {
    Application *app =
        static_cast<Application *>(glfwGetWindowUserPointer(window));

    if (app)
      app->mouseCursorPosCallback(window, xpos, ypos);
  }

  void mouseCursorPosCallback(GLFWwindow *window, double xpos, double ypos) {

    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    if (ImGui::GetIO().WantSetMousePos)
      return;

    Scene *scene = sceneManager.getCurrent();
    scene->HandleMouseInput(window, xpos, ypos);
  }

  static void mouseScrollBackStatic(GLFWwindow *window, double xoffset,
                                    double yoffset) {
    Application *app =
        static_cast<Application *>(glfwGetWindowUserPointer(window));

    if (app)
      app->mouseScrollBack(window, xoffset, yoffset);
  }

  void mouseScrollBack(GLFWwindow *window, double xoffset, double yoffset) {

    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse)
      return;
    Scene *scene = sceneManager.getCurrent();
    scene->HandleScrollInput(window, xoffset, yoffset);
  }

  void DrawMainMenuBar(double fps, size_t sceneID) {

    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("Scenes")) {
        for (auto &group : groupedScenes) {
          if (!group.first.empty()) {
            ImGui::SeparatorText(group.first.c_str());
          } else {
            ImGui::Separator();
          }

          for (auto scene : group.second) {
            if (ImGui::MenuItem(scene->name().c_str())) {
              sceneManager.setCurrentScene(scene);
            }
          }
        }

        ImGui::EndMenu();
      }

      std::string sFPS = std::to_string(fps);

      // FPS Counter on the right Menu Bar
      ImVec2 regionMax = ImGui::GetContentRegionMax();
      ImGui::SetCursorPosX(regionMax.x - ImGui::CalcTextSize(sFPS.c_str()).x);
      ImGui::TextUnformatted(sFPS.c_str());

      std::string sSceneId = "Scene Id: " + std::to_string(sceneID) + " | ";

      std::string combined = sFPS + sSceneId;

      ImGui::SetCursorPosX(regionMax.x -
                           ImGui::CalcTextSize(combined.c_str()).x);
      ImGui::TextUnformatted(sSceneId.c_str());

      ImGui::EndMainMenuBar();
    }
  }

  static void FramebufferSizeCallback(GLFWwindow *window, int width,
                                      int height) {
    Application *app =
        static_cast<Application *>(glfwGetWindowUserPointer(window));
    app->handleResize(width, height);
  }

  void handleResize(int width, int height) {
    int fbw, fbh = 1;
    glfwGetFramebufferSize(_window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);
    float aspect = static_cast<float>(fbw) / static_cast<float>(fbh);
    Scene *scene = sceneManager.getCurrent();
    scene->OnResize(aspect);
  }

public:
  void Init() {

    sceneManager.setSceneVectorFirstAsStartPoint();

    groupedScenes = createGroupedScenesVector();

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
      printf("Error: GLFW Init. %d", glfwGetError(NULL));
      return;
    }

    const char *glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    float mainScale =
        ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    _window = glfwCreateWindow((int)(1280 * mainScale), (int)(800 * mainScale),
                               "Brick Terrain", nullptr, nullptr);

    if (_window == nullptr) {
      printf("Error: Window is nullptr. %d", glfwGetError(NULL));
      return;
    }

    glfwSetWindowUserPointer(_window, this);

    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1); // HINT: VSync

    glewExperimental = GL_TRUE;
    glewInit();

    glCall(glEnable(GL_DEPTH_TEST));

    lastTime = glfwGetTime();

    int major, minor;
    glfwGetVersion(&major, &minor, nullptr);

    std::cout << "GLFW Version: " << major << "." << minor << "\n";

    imGuiLayer = new ImGuiLayer(_window, mainScale);

    if (glfwRawMouseMotionSupported())
      glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    else
      std::cout << "Not Supported\n";

    glfwSetKeyCallback(_window, keyCallbackStatic);
    glfwSetMouseButtonCallback(_window, mouseButtonCallbackStatic);
    glfwSetCursorPosCallback(_window, mouseCursorPosCallbackStatic);
    glfwSetScrollCallback(_window, mouseScrollBackStatic);
    glfwSetFramebufferSizeCallback(_window, FramebufferSizeCallback);

    // BUG: Wayland tiled fenster sofort, doch es wird kein Resize Event
    // ausgelöst, darum direkt zu begin, doch es funktioniert nicht.
    // int w, h;
    // glCall(glfwGetFramebufferSize(_window, &w, &h));
    // FramebufferSizeCallback(_window, w, h);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity,
           GLsizei length, const GLchar *msg, const void *userParam) {
          std::cerr << "[GL DEBUG] " << msg << '\n';
        },
        nullptr);
  }

  void Run() {

    float previousTime = glfwGetTime();

    int displayW, displayH;
    glfwGetFramebufferSize(_window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);

    if (auto *scene = sceneManager.getCurrent()) {
      scene->ensureInitialized(_window);
      float aspect =
          static_cast<float>(displayW) / static_cast<float>(displayH);
      scene->OnResize(aspect);
    }

    while (!glfwWindowShouldClose(_window)) {
      float currentTime = glfwGetTime();
      float deltaTime = currentTime - previousTime;
      previousTime = currentTime;

      glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w,
                   clearColor.z * clearColor.w, clearColor.w);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // FPS
      ++frameCount;

      if (currentTime - lastTime >= 0.5) {
        fps = frameCount / (currentTime - lastTime);
        frameCount = 0;
        lastTime = currentTime;
      }

      // Scene Stuff
      if (auto *scene = sceneManager.getCurrent()) {
        scene->ensureInitialized(_window);
        scene->HandleInput(_window);
        scene->Update(deltaTime);
        scene->Render();
      }

      imGuiLayer->NewFrame();

      DrawMainMenuBar(fps, sceneManager.getCurrent()->Id());

      auto scene = sceneManager.getCurrent();
      scene->ImGuiLayer();

      imGuiLayer->Render();

      glfwSwapBuffers(_window);
      glfwPollEvents();

      int fbw, fbh;
      glfwGetFramebufferSize(_window, &fbw, &fbh);
      if (fbw != displayW || fbh != displayH) {
        displayW = fbw;
        displayH = fbh;
        handleResize(displayW, displayH);
      }
    }
  }

  ~Application() {
    delete imGuiLayer;
    glfwDestroyWindow(_window);
    glfwTerminate();
  }

  void RegisterScene(std::unique_ptr<Scene> scene) {
    sceneManager.push(std::move(scene));
  }

  void SetCurrentScene(const std::string &sceneName) {
    Scene *scene = sceneManager.getByName(sceneName);
    if (scene != nullptr) {
      sceneManager.setCurrentScene(scene);
    }
  }
};
