#pragma once

#include "GLFW/glfw3.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>

class Scene {
private:
  size_t _id;
  std::string _name;

  bool _initialized = false;

  // INFO: Alles wird über den Namen gemacht, nicht über ID. Daher ist die
  // untere Methode Obsolete.

  // TODO Fix dass es immer pro Runtime eine Unique ID ist.
  // BUG: ID gibt immer 0 zurück
  static size_t generateId() {
    static std::atomic<size_t> counter{0};
    return ++counter;
  }

public:
  Scene() : _id(generateId()) {}
  Scene(std::string name) : _name(name) {}

  void ensureInitialized(GLFWwindow *window) {
    if (!_initialized) {
      InitScene();
      InitScene(window);
      _initialized = true;
    }
  }

  virtual ~Scene() = default;
  virtual void InitScene() {}
  virtual void InitScene(GLFWwindow *window) {}

  // Gets the Input Data through the Main-Loop.
  virtual void HandleInput(GLFWwindow *window) {}
  // Gets the Input Data through the GLFW key Callback.
  virtual void HandleInput(GLFWwindow *window, int key, int scancode,
                           int action, int mods) {}

  virtual void Update(float dt) = 0;
  virtual void Render() = 0;
  virtual void HandleMouseInput(GLFWwindow *window, double xpos, double ypos) {}
  virtual void ImGuiLayer() {}

  virtual void OnResize(float aspectRatio) {}

  std::size_t id() const noexcept { return _id; }

  std::string name() const noexcept { return _name; }
};
