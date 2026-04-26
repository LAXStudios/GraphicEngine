#pragma once

#include "Scene.h"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class SceneManager {
private:
  std::vector<std::unique_ptr<Scene>> scenes;

  Scene *currentScene;

public:
  bool startSceneIsSet = false;

  void push(std::unique_ptr<Scene> scene) {
    scenes.emplace_back(std::move(scene));
  }

  // WARNING: Momentan obsolete
  Scene *getById(size_t id) {
    auto it = std::find_if(
        scenes.begin(), scenes.end(),
        [&](const std::unique_ptr<Scene> &p) { return p && p->id() == id; });
    return (it != scenes.end()) ? it->get() : nullptr;
  }

  Scene *getByName(std::string name) {
    for (std::unique_ptr<Scene> &scene : scenes) {
      if (scene->name() == name) {
        return scene.get();
      }
    }
    return nullptr;
  }

  std::vector<std::unique_ptr<Scene>> &getScenesAsVector() { return scenes; }

  /*
  void setCurrentScene(Scene *scene) {
    for (std::unique_ptr<Scene> &s : scenes) {
      if (s->name() == scene->name()) {
        if (!startSceneIsSet) {
          startSceneIsSet = true;
        }
        currentScene = s;
      }
    }
  }
*/
  void setCurrentScene(Scene *scene) { currentScene = scene; }

  void setSceneVectorFirstAsStartPoint() {
    if (!scenes.empty()) {
      currentScene = scenes.front().get();
      startSceneIsSet = true;
    }
  }

  Scene *current() { return scenes.empty() ? nullptr : scenes.back().get(); }

  Scene *getCurrent() {
    // std::cout << "currentScene:" << currentScene->get()->name() << "\n";
    return currentScene;
  }
};
