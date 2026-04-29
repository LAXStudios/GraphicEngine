#pragma once

// clang-format off
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../../../../extern/imgui/backend/imgui_impl_glfw.h"
#include "../../../../extern/imgui/backend/imgui_impl_opengl3.h"
#include "../../../../extern/imgui/imgui.h"
#include "../../Scene/Scene.h"
#include "../../Scene/SceneManager.h"
#include "../VertexBuffer/VertexBuffer.h"
#include "../VertexBufferLayout/VertexBufferLayout.h"
#include "../ShaderProgram/ShaderProgram.h"
#include "../Texture/Texture.h"
#include "../TextureManager/TextureManager.h"
#include "../VertexArray/VertexArray.h"
#include "../../../Headers/Core/Camera/FPSCamera.h"
#include "../IndexBuffer/IndexBuffer.h"
#include "../Mesh/Mesh.h"
#include "../Renderer/RendererWrapper.h"
#include "ErrorHandling.h"
#include <cstddef>
#include <cstdio>
#include <cmath>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// clang-format on
