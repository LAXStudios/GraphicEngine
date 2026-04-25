# Graphic Engine (LaxEngine)

# About

This is a small little Project for learning Graphics Programming with `OpenGL`, `GLFW`, `ImGui` and building an own "Engine". This "Engine" is build from diverse public sources for learning `OpenGL`. Mainly [LearnOpenGL.com](https://learnopengl.com).

I am not a professional C++ or Graphics developer, nor a CS Student, this is just a hobby project.

# Bugs
There is currently a Bug in GLFW, which concerns the compatibility with wayland and the its mouse behavior. The mouse wont hide when said so, you have to unfocus and refocus the window.

# Bugs that kept me awake.
### 1. Texture Loading, RAII and rule of 5 
Thx to `@madeso`, `@derhass` for pointing out that I coded a double free by copying the texture object.
Solution was to implement RAII and Ro5 to prevent the copieng, and force moving the object. As an addition I created the Texture Manager to handle the loaded Textures and made use of smart pointers.

### 2. ShaderProgram
In my ShaderProgram Class I unbound the shader after calling a uniform, therefore no valid shader was active and when calling
```cpp
myModel->Draw(*shaderProgramPtr);
```
it didn't worked.
# Overview of features (displayed in Scenes)
- A Quad with the basic Gradient (QuadColoredScene)
- Just a Polygon (PolygonScene)
- Start Polygon with cool gradient (ColoredPolyGonScene)
- Just a Image (ImageTextureScene) _bugged_
- A Texture which is spinning (TransformationScene)
- Some Cubes with diffrent rotations and cute pics of a cat (CatCubes3DScene) _bugged_
- A Scene with Cubes and the introduction of a FPS camera (MovementScene) _bugged_
- Just a colored Cube with a light (ColorLightingScene)
- Basic Cube with changing color and a point light (BasicMaterialScene)
- Using Diffuse Texture on top an other Texture with lighting (BasicDiffuseMapScene) _bugged_
- Directional Light Shader with pointing on multiple Objects (BasicDirectionalLightScene) _bugged_
- Flashlight pointed from camera position (BasicFlashLightScene) _bugged_
- Cubes with Textures (diffuse and specular) lighted by multiple Point Lights (BasicMultipleLightsScene)
- Scene with loading  a 3D Model (backpack) via assimp and applying diffuse Texture (ModelLoadingScene)

> Info 
> Some Scenes are Bugged due to the switch from SOIL to stb_image and the updated Texture Class.


# Use of AI
I used Lumo by Proton for basic questions, with the second "major" bug, I switched to claude code, and it pointed the bug out (embarrassing).
My target with this repo is to learn OpenGL and C++. AI is not my start point, more a google on steroids which I use to clarify and ask questions. 
I am now using Sonnet 4.7 and claude code in terminal for major bugs. I have a claude Pro subscription.
