#include "../Headers/Application.h"
#include <memory>
#define NOTHING
#include "Scenes/ScenesCollection.h"
#include <thread>

std::thread::id mainThreadId;

int main(int argc, char *argv[]) {

  mainThreadId = std::this_thread::get_id();
  std::cout << "Main Thread: " << mainThreadId << std::endl;

  Application application = Application();

  // INFO: Register Scenes must be done before Init, ore you have to set the
  // first scene manual.
  application.RegisterScene(
      std::make_unique<ColoredPolygonScene>("ColoredPolygonScene"));
  application.RegisterScene(std::make_unique<PolygonScene>("PolygonScene"));
  application.RegisterScene(std::make_unique<QuadColored>("QuadColored"));
  application.RegisterScene(std::make_unique<DisplayImage>("DisplayImage"));
  application.RegisterScene(
      std::make_unique<TransformationScene>("TransformationScene"));
  application.RegisterScene(
      std::make_unique<CatCubes3DScene>("CatCubes3DScene"));
  application.RegisterScene(std::make_unique<MovementScene>("MovementScene"));
  application.RegisterScene(
      std::make_unique<ColorLightningScene>("ColorLightningScene"));

  application.Init();

  application.Run();
}
