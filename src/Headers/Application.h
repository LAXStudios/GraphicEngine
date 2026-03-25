#pragma once
#include "../Headers/Core/Common/Common.h"
#include "Core/Common/ErrorHandling.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <exception>
#define NOTHING
#include "../Headers/ImGui/ImGuiLayer.h"
#include <cstdio>
#include <memory>
#include <stdatomic.h>
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

  // TODO Andere Lösung dafür finden
  // bool firstRun = true;

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

  void DrawMainMenuBar(double fps) {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("Scenes")) {
        for (std::unique_ptr<Scene> &scene : sceneManager.getScenesAsVector()) {
          if (ImGui::MenuItem(scene->name().c_str())) {
            sceneManager.setCurrentScene(scene.get());
          }
        }

        ImGui::EndMenu();
      }

      std::string s = std::to_string(fps);
      const char *fpsPtr = s.c_str();

      // FPS Counter on the right Menu Bar
      ImVec2 regionMax = ImGui::GetContentRegionMax();
      ImGui::SetCursorPosX(regionMax.x - ImGui::CalcTextSize(fpsPtr).x);
      ImGui::TextUnformatted(fpsPtr);

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

      DrawMainMenuBar(fps);

      auto scene = sceneManager.getCurrent();
      scene->ImGuiLayer();

      imGuiLayer->Render();

      int displayW, displayH;
      glfwGetFramebufferSize(_window, &displayW, &displayH);
      glViewport(0, 0, displayW, displayH);

      glfwSwapBuffers(_window);
      glfwPollEvents();
    }
  }

  ~Application() {
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
