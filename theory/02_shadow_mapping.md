# Shadow Mapping

## Was ist Shadow Mapping?

Schatten sind einer der größten Unterschiede zwischen "sieht wie ein Studentenprojekt aus" und "sieht wie ein echtes Spiel aus". OpenGL hat keine eingebaute Schatten-Funktion — du musst sie selbst implementieren.

Die Grundidee ist genial einfach:

> **Wenn du von der Lichtquelle aus schaust, ist alles was du siehst beleuchtet. Was du NICHT siehst, liegt im Schatten.**

Shadow Mapping implementiert genau das in zwei Render-Passes:

1. **Shadow Pass**: Rendere die gesamte Szene aus Sicht der Lichtquelle. Speichere nur die Tiefe (Depth) in einer Textur — der **Shadow Map**.
2. **Scene Pass**: Rendere die Szene normal. Für jeden Fragment: Vergleiche seine Tiefe (aus Lichtsicht) mit dem Wert in der Shadow Map. Ist die Tiefe größer → im Schatten.

---

## Schritt 1: Framebuffer Object (FBO) für die Shadow Map

Ein **Framebuffer Object** ist ein off-screen Rendering-Ziel. Statt auf den Bildschirm zu rendern, renderst du in eine Textur.

```cpp
// Depth-Textur erstellen (kein Farb-Attachment nötig!)
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

glGenFramebuffers(1, &depthMapFBO);

// Depth-Textur
glGenTextures(1, &depthMap);
glBindTexture(GL_TEXTURE_2D, depthMap);
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
             SHADOW_WIDTH, SHADOW_HEIGHT, 0,
             GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

// Clamp to Border: außerhalb der Shadow Map = kein Schatten
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

// Textur als Depth-Attachment an den FBO hängen
glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

// Kein Color-Output gewollt
glDrawBuffer(GL_NONE);
glReadBuffer(GL_NONE);

glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

---

## Schritt 2: Light Space Matrix berechnen

Die Lichtquelle hat ihre eigene View- und Projection-Matrix. Für ein **Directional Light** (Sonne) nutzt man eine orthographische Projektion:

```cpp
glm::vec3 lightPos = glm::vec3(-2.0f, 10.0f, -1.0f);
glm::vec3 lightDir = glm::normalize(-lightPos); // zeigt zum Ursprung

// Orthographisch, weil Sonnenlicht parallele Strahlen hat
glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 50.0f);

glm::mat4 lightView = glm::lookAt(
    lightPos,
    glm::vec3(0.0f),   // schaut zum Ursprung
    glm::vec3(0.0f, 1.0f, 0.0f)
);

glm::mat4 lightSpaceMatrix = lightProjection * lightView;
```

**Wichtig für Lego-Terrain**: Die `ortho`-Werte müssen groß genug sein um das gesamte Terrain zu umfassen. Für ein 100×100 Terrain: `glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, ...)`.

---

## Schritt 3: Shadow Pass Shader

Dieser Shader ist sehr einfach — er braucht keine Fragmente zu ausgeben, nur die Tiefe zu schreiben:

```glsl
// depth_shader.glsl

#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}

#shader fragment
#version 330 core

// Kein Output nötig — OpenGL schreibt automatisch die Tiefe
void main() {}
```

---

## Schritt 4: Scene Pass Shader (mit Shadow-Vergleich)

```glsl
// scene_shadow.glsl

#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 lightSpaceMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;  // Position des Fragments aus Lichtsicht

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos          = worldPos.xyz;
    Normal           = mat3(transpose(inverse(model))) * aNormal;
    TexCoord         = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = proj * view * worldPos;
}

#shader fragment
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perspective divide → Normalized Device Coordinates (-1 bis 1)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // NDC zu Textur-Koordinaten (0 bis 1)
    projCoords = projCoords * 0.5 + 0.5;

    // Falls außerhalb der Shadow Map → kein Schatten
    if (projCoords.z > 1.0) return 0.0;

    // Tiefe des nächsten Objekts aus Lichtsicht (aus Shadow Map)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Tiefe des aktuellen Fragments aus Lichtsicht
    float currentDepth = projCoords.z;

    // Shadow Bias: verhindert "Shadow Acne" (Selbst-Beschattungs-Artefakte)
    // Je flacher der Winkel zwischen Normal und Licht, desto mehr Bias nötig
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // PCF: Percentage Closer Filtering für weiche Schatten
    // Statt eines Punktes samplen wir einen 3x3 Bereich
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;  // Durchschnitt der 9 Samples

    return shadow;
}

void main() {
    vec3 color    = texture(diffuseTexture, TexCoord).rgb;
    vec3 normal   = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Phong Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Ambient (immer sichtbar, auch im Schatten)
    vec3 ambient = 0.3 * color;

    // Diffuse (wird durch Schatten blockiert)
    vec3 diffuse = diff * color;

    // shadow = 0.0 → voll beleuchtet, 1.0 → voll im Schatten
    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);

    vec3 result = ambient + (1.0 - shadow) * diffuse;
    FragColor = vec4(result, 1.0);
}
```

---

## Vollständige Scene-Implementierung

### `ShadowMappingScene.h`

```cpp
#pragma once
#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <vector>

class ShadowMappingScene : public Scene {
private:
    // Zwei Shader: einer für Shadow Pass, einer für Scene Pass
    ShaderProgram* depthShaderPtr  = nullptr;
    ShaderProgram* sceneShaderPtr  = nullptr;

    // Geometrie
    VertexArray*  floorVAO    = nullptr;
    VertexBuffer* floorVBO    = nullptr;
    VertexArray*  cubeVAO     = nullptr;
    VertexBuffer* cubeVBO     = nullptr;

    // Texturen
    unsigned int woodTexture;

    // Shadow Map FBO
    GLuint depthMapFBO = 0;
    GLuint depthMap    = 0;
    const unsigned int SHADOW_W = 2048;
    const unsigned int SHADOW_H = 2048;

    // Licht
    glm::vec3 lightPos = glm::vec3(-2.0f, 8.0f, -1.0f);

    // Kamera
    FPSCamera camera{ glm::vec3(0.0f, 4.0f, 10.0f) };
    float deltaTime   = 0.0f;
    float lastX       = 640.0f;
    float lastY       = 400.0f;
    bool  firstMouse  = true;
    bool  cursorHidden = true;
    float aspectRatio = 1280.0f / 800.0f;

    // Debug
    bool  showDepthMap   = false;
    float lightAngle     = 0.0f;
    bool  animateLight   = true;

public:
    ShadowMappingScene(const std::string& name) : Scene(name) {}

    void InitScene(GLFWwindow* window) override {
        depthShaderPtr = new ShaderProgram(
            programPath("Main/Scenes/ShadowMappingScene/Shaders/depth.glsl"));
        sceneShaderPtr = new ShaderProgram(
            programPath("Main/Scenes/ShadowMappingScene/Shaders/scene_shadow.glsl"));

        setupGeometry();
        setupShadowMap();

        woodTexture = TextureManager::Get().LoadTexture(
            programPath("Main/Scenes/CatCubes3DScene/Assets/soka_blue_cutie.png"));

        // Scene-Shader konfigurieren
        sceneShaderPtr->Bind();
        sceneShaderPtr->setUniform1i("diffuseTexture", 0);
        sceneShaderPtr->setUniform1i("shadowMap", 1);

        glEnable(GL_DEPTH_TEST);

        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Update(float dt) override {
        deltaTime = dt;
        if (animateLight) {
            lightAngle += dt * 0.5f;
            lightPos.x = sin(lightAngle) * 8.0f;
            lightPos.z = cos(lightAngle) * 8.0f;
        }
    }

    void HandleInput(GLFWwindow* window) override {
        if (!cursorHidden) return;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD,  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT,     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT,    deltaTime);
    }

    void HandleMouseInput(GLFWwindow* window, double xpos, double ypos) override {
        if (!cursorHidden) return;
        float x = static_cast<float>(xpos);
        float y = static_cast<float>(ypos);
        if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
        camera.ProcessMouseMovement(x - lastX, lastY - y);
        lastX = x; lastY = y;
    }

    void HandleInput(GLFWwindow* window, int key, int scancode, int action, int mods) override {
        if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
            cursorHidden = !cursorHidden;
            glfwSetInputMode(window, GLFW_CURSOR,
                cursorHidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }
    }

    void OnResize(float ar) override { aspectRatio = ar; }

    void Render() override {
        // ── PASS 1: Shadow Map rendern ──────────────────────────
        glm::mat4 lightSpaceMatrix = computeLightSpaceMatrix();

        glViewport(0, 0, SHADOW_W, SHADOW_H);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShaderPtr->Bind();
        depthShaderPtr->setUniformMatrix4fv("lightSpaceMatrix", lightSpaceMatrix);

        renderScene(depthShaderPtr);   // Szene aus Lichtsicht rendern

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ── PASS 2: Normale Szene mit Schatten rendern ──────────
        int fbW, fbH;
        // Viewport zurück auf Fenster-Größe setzen
        // (eigentlich solltest du die Fenstergröße speichern)
        glViewport(0, 0, static_cast<int>(aspectRatio * 800), 800);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);

        sceneShaderPtr->Bind();
        sceneShaderPtr->setUniformMatrix4fv("view", view);
        sceneShaderPtr->setUniformMatrix4fv("proj", proj);
        sceneShaderPtr->setUniformMatrix4fv("lightSpaceMatrix", lightSpaceMatrix);
        sceneShaderPtr->setUniform3fv("lightPos",  lightPos);
        sceneShaderPtr->setUniform3fv("viewPos",   camera.Position);

        // Diffuse-Textur auf Unit 0
        bindTexture(woodTexture, 0);

        // Shadow Map auf Unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        renderScene(sceneShaderPtr);
    }

    void ImGuiLayer() override {
        ImGui::Begin("Shadow Mapping", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Licht-Position: %.1f, %.1f, %.1f",
            lightPos.x, lightPos.y, lightPos.z);
        ImGui::Checkbox("Licht animieren", &animateLight);
        if (!animateLight) {
            ImGui::SliderFloat("Licht X", &lightPos.x, -20.0f, 20.0f);
            ImGui::SliderFloat("Licht Y", &lightPos.y,  1.0f,  20.0f);
            ImGui::SliderFloat("Licht Z", &lightPos.z, -20.0f, 20.0f);
        }
        ImGui::Separator();
        ImGui::Text("Shadow Map Auflösung: %dx%d", SHADOW_W, SHADOW_H);
        ImGui::Text("Tipp: Höhere Auflösung = schärfere Schatten");
        ImGui::End();
    }

    ~ShadowMappingScene() {
        delete depthShaderPtr;
        delete sceneShaderPtr;
        delete floorVAO;
        delete floorVBO;
        delete cubeVAO;
        delete cubeVBO;
        if (depthMapFBO) glDeleteFramebuffers(1, &depthMapFBO);
        if (depthMap)    glDeleteTextures(1, &depthMap);
    }

private:
    glm::mat4 computeLightSpaceMatrix() {
        glm::mat4 lightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 50.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        return lightProj * lightView;
    }

    void renderScene(ShaderProgram* shader) {
        // Boden
        glm::mat4 model = glm::mat4(1.0f);
        shader->setUniformMatrix4fv("model", model);
        floorVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        floorVAO->UnBind();

        // Würfel 1
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        shader->setUniformMatrix4fv("model", model);
        cubeVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        cubeVAO->UnBind();

        // Würfel 2 (gedreht)
        model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.5f, 1.0f));
        model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        shader->setUniformMatrix4fv("model", model);
        cubeVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        cubeVAO->UnBind();

        // Würfel 3
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 1.5f, -1.0f));
        shader->setUniformMatrix4fv("model", model);
        cubeVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        cubeVAO->UnBind();
    }

    void setupShadowMap() {
        glGenFramebuffers(1, &depthMapFBO);

        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     SHADOW_W, SHADOW_H, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void setupGeometry() {
        // Boden (großes Quad mit Normals)
        float floorVerts[] = {
            // pos                  normal           texcoord
            -10.f, 0.f,-10.f,  0.f,1.f,0.f,  0.0f,10.0f,
             10.f, 0.f, 10.f,  0.f,1.f,0.f, 10.0f, 0.0f,
             10.f, 0.f,-10.f,  0.f,1.f,0.f, 10.0f,10.0f,
             10.f, 0.f, 10.f,  0.f,1.f,0.f, 10.0f, 0.0f,
            -10.f, 0.f,-10.f,  0.f,1.f,0.f,  0.0f,10.0f,
            -10.f, 0.f, 10.f,  0.f,1.f,0.f,  0.0f, 0.0f,
        };
        floorVAO = new VertexArray();
        floorVBO = new VertexBuffer(floorVerts, sizeof(floorVerts));
        floorVAO->Bind(); floorVBO->Bind();
        VertexBufferLayout floorLayout;
        floorLayout.AddElement<float>(3); // pos
        floorLayout.AddElement<float>(3); // normal
        floorLayout.AddElement<float>(2); // tex
        floorVAO->AddBuffer(*floorVBO, floorLayout);
        floorVAO->UnBind(); floorVBO->UnBind();

        // Würfel mit Normals (vereinfacht, nur eine Fläche als Beispiel —
        // vollständige Vertex-Daten wie in BasicDiffuseMapScene verwenden)
        cubeVAO = new VertexArray();
        cubeVBO = new VertexBuffer(cubeWithNormals.data(),
            static_cast<GLuint>(cubeWithNormals.size() * sizeof(float)));
        cubeVAO->Bind(); cubeVBO->Bind();
        VertexBufferLayout cubeLayout;
        cubeLayout.AddElement<float>(3); // pos
        cubeLayout.AddElement<float>(3); // normal
        cubeLayout.AddElement<float>(2); // tex
        cubeVAO->AddBuffer(*cubeVBO, cubeLayout);
        cubeVAO->UnBind(); cubeVBO->UnBind();
    }

    // Cube-Vertices mit Normals (pos3 + normal3 + tex2)
    const std::vector<float> cubeWithNormals = {
        -0.5f,-0.5f,-0.5f, 0.f,0.f,-1.f, 0.0f,0.0f,
         0.5f,-0.5f,-0.5f, 0.f,0.f,-1.f, 1.0f,0.0f,
         0.5f, 0.5f,-0.5f, 0.f,0.f,-1.f, 1.0f,1.0f,
         0.5f, 0.5f,-0.5f, 0.f,0.f,-1.f, 1.0f,1.0f,
        -0.5f, 0.5f,-0.5f, 0.f,0.f,-1.f, 0.0f,1.0f,
        -0.5f,-0.5f,-0.5f, 0.f,0.f,-1.f, 0.0f,0.0f,

        -0.5f,-0.5f, 0.5f, 0.f,0.f, 1.f, 0.0f,0.0f,
         0.5f,-0.5f, 0.5f, 0.f,0.f, 1.f, 1.0f,0.0f,
         0.5f, 0.5f, 0.5f, 0.f,0.f, 1.f, 1.0f,1.0f,
         0.5f, 0.5f, 0.5f, 0.f,0.f, 1.f, 1.0f,1.0f,
        -0.5f, 0.5f, 0.5f, 0.f,0.f, 1.f, 0.0f,1.0f,
        -0.5f,-0.5f, 0.5f, 0.f,0.f, 1.f, 0.0f,0.0f,

        -0.5f, 0.5f, 0.5f,-1.f,0.f,0.f, 1.0f,0.0f,
        -0.5f, 0.5f,-0.5f,-1.f,0.f,0.f, 1.0f,1.0f,
        -0.5f,-0.5f,-0.5f,-1.f,0.f,0.f, 0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,-1.f,0.f,0.f, 0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,-1.f,0.f,0.f, 0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,-1.f,0.f,0.f, 1.0f,0.0f,

         0.5f, 0.5f, 0.5f, 1.f,0.f,0.f, 1.0f,0.0f,
         0.5f, 0.5f,-0.5f, 1.f,0.f,0.f, 1.0f,1.0f,
         0.5f,-0.5f,-0.5f, 1.f,0.f,0.f, 0.0f,1.0f,
         0.5f,-0.5f,-0.5f, 1.f,0.f,0.f, 0.0f,1.0f,
         0.5f,-0.5f, 0.5f, 1.f,0.f,0.f, 0.0f,0.0f,
         0.5f, 0.5f, 0.5f, 1.f,0.f,0.f, 1.0f,0.0f,

        -0.5f,-0.5f,-0.5f, 0.f,-1.f,0.f, 0.0f,1.0f,
         0.5f,-0.5f,-0.5f, 0.f,-1.f,0.f, 1.0f,1.0f,
         0.5f,-0.5f, 0.5f, 0.f,-1.f,0.f, 1.0f,0.0f,
         0.5f,-0.5f, 0.5f, 0.f,-1.f,0.f, 1.0f,0.0f,
        -0.5f,-0.5f, 0.5f, 0.f,-1.f,0.f, 0.0f,0.0f,
        -0.5f,-0.5f,-0.5f, 0.f,-1.f,0.f, 0.0f,1.0f,

        -0.5f, 0.5f,-0.5f, 0.f,1.f,0.f, 0.0f,1.0f,
         0.5f, 0.5f,-0.5f, 0.f,1.f,0.f, 1.0f,1.0f,
         0.5f, 0.5f, 0.5f, 0.f,1.f,0.f, 1.0f,0.0f,
         0.5f, 0.5f, 0.5f, 0.f,1.f,0.f, 1.0f,0.0f,
        -0.5f, 0.5f, 0.5f, 0.f,1.f,0.f, 0.0f,0.0f,
        -0.5f, 0.5f,-0.5f, 0.f,1.f,0.f, 0.0f,1.0f,
    };
};
```

---

## Shadow Acne — und wie man es behebt

**Shadow Acne** sind Streifen-Artefakte die entstehen, weil die Shadow Map eine endliche Auflösung hat. Ein Fragment vergleicht sich mit dem nächstgelegenen Shadow-Map-Texel, aber durch floating-point Ungenauigkeiten beschattet sich das Objekt selbst.

```
Fragment-Tiefe: 0.50012
Shadow Map:     0.50011  → Fragment denkt, es ist im Schatten von sich selbst!
```

**Lösung: Bias**
```glsl
float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
```

Der Bias "hebt" die Fragment-Tiefe leicht an, sodass sie sich nicht selbst beschattet. Zu großer Bias erzeugt **Peter Panning** (Schatten floaten vom Objekt weg).

---

## PCF: Weiche Schatten

Ohne PCF haben Schatten harte Kanten. PCF samplet einen Bereich der Shadow Map und mittelt:

```glsl
// 3x3 PCF = 9 Samples → leicht weiche Kanten
// 5x5 PCF = 25 Samples → noch weicher, aber teurer
float shadow = 0.0;
vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
}
shadow /= 9.0;
```

---

## Häufige Fehler

1. **Viewport vergessen zurückzusetzen**: Nach dem Shadow Pass ist der Viewport `2048x2048`. Vor dem Scene Pass muss er zurück zur Fenstergröße — sonst rendert die Szene in eine Ecke.

2. **`GL_CLAMP_TO_BORDER` vergessen**: Objekte außerhalb der Shadow Map bekommen Schatten-Wert 0 oder 1 (je nach Wrapping). `CLAMP_TO_BORDER` mit weißer Border (1.0) sorgt für "kein Schatten" außerhalb.

3. **FBO Completeness prüfen**:
```cpp
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "Framebuffer nicht komplett!\n";
```

4. **lightSpaceMatrix muss in beide Shader**: Der Depth-Shader braucht sie für Pass 1, der Scene-Shader für den Vergleich in Pass 2.
