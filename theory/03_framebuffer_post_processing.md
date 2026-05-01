# Framebuffer & Post-Processing

## Was ist ein Framebuffer?

Normalerweise rendert OpenGL direkt in den **Default Framebuffer** — das ist das Fenster, das der Spieler sieht. Ein **Framebuffer Object (FBO)** ist ein zweiter, unsichtbarer Framebuffer: du renderst die gesamte Szene dort hinein (in eine Textur), und kannst diese Textur dann weiterverarbeiten, bevor du sie auf den Bildschirm zeichnest.

Genau so funktionieren **Post-Processing-Effekte** in echten Spielen:

1. Rendere die Szene in eine Off-Screen-Textur
2. Zeichne ein fullscreen Quad mit der Textur
3. Im Fragment-Shader manipuliere die Farben → Bloom, HDR, Motion Blur, Vignette, etc.

```
Szene → [FBO Textur] → Fullscreen Quad Shader → Bildschirm
                         ↑
                    Hier passiert der Effekt
```

---

## Aufbau eines Framebuffers

Ein vollständiger FBO braucht zwei Attachments:

| Attachment | Typ | Inhalt |
|---|---|---|
| **Color Attachment** | Textur (empfohlen) | RGB-Farbe jedes Pixels |
| **Depth+Stencil Attachment** | Renderbuffer (empfohlen) | Tiefen- und Stencil-Werte |

Für das Color-Attachment nutzen wir eine Textur (weil wir sie danach im Shader lesen wollen). Für Depth brauchen wir nur Schreibzugriff, daher ein schnellerer **Renderbuffer**.

### Vollständige FBO-Einrichtung

```cpp
GLuint fbo;
glGenFramebuffers(1, &fbo);
glBindFramebuffer(GL_FRAMEBUFFER, fbo);

// ── Color Attachment: Textur ──────────────────────────────────
GLuint colorTexture;
glGenTextures(1, &colorTexture);
glBindTexture(GL_TEXTURE_2D, colorTexture);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
    windowWidth, windowHeight, 0,
    GL_RGB, GL_UNSIGNED_BYTE, nullptr);    // nullptr = leer, wird beim Rendern gefüllt
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

// Textur als Color-Attachment anhängen
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D, colorTexture, 0);

// ── Depth+Stencil Attachment: Renderbuffer ────────────────────
GLuint rbo;
glGenRenderbuffers(1, &rbo);
glBindRenderbuffer(GL_RENDERBUFFER, rbo);
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
    windowWidth, windowHeight);
glBindRenderbuffer(GL_RENDERBUFFER, 0);

glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
    GL_RENDERBUFFER, rbo);

// ── Vollständigkeit prüfen ────────────────────────────────────
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "ERROR: Framebuffer nicht komplett!\n";

glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

---

## Das Fullscreen Quad

Um die FBO-Textur auf den Bildschirm zu rendern, zeichnen wir ein Quad das den gesamten Bildschirm abdeckt. Die Koordinaten sind in **Normalized Device Coordinates (NDC)** — kein MVP nötig.

```cpp
float quadVertices[] = {
    // NDC-Position    Textur-Koordinate
    -1.0f,  1.0f,     0.0f, 1.0f,
    -1.0f, -1.0f,     0.0f, 0.0f,
     1.0f, -1.0f,     1.0f, 0.0f,

    -1.0f,  1.0f,     0.0f, 1.0f,
     1.0f, -1.0f,     1.0f, 0.0f,
     1.0f,  1.0f,     1.0f, 1.0f,
};
```

**Wichtig**: Depth Test für das Quad deaktivieren — es soll immer gezeichnet werden, egal was im Depth Buffer steht.

---

## Render-Loop Struktur

```cpp
void Render() {
    // ── Pass 1: In FBO rendern ───────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderScene();   // deine normale Szene

    // ── Pass 2: FBO-Textur auf Bildschirm rendern ────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);   // zurück zum Default Framebuffer
    glDisable(GL_DEPTH_TEST);               // Quad braucht keinen Depth Test
    glClear(GL_COLOR_BUFFER_BIT);

    postProcessShader->Bind();
    glBindVertexArray(quadVAO);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
```

---

## Post-Processing Shader Effekte

### Basis-Shader (kein Effekt)

```glsl
#shader vertex
#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    TexCoord    = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);   // kein MVP, direkte NDC
}

#shader fragment
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform int effectMode;   // 0 = normal, 1 = grau, 2 = invertiert, usw.

// ── Effekte ────────────────────────────────────────────────────

vec3 grayscale(vec3 col) {
    // Menschliches Auge ist am empfindlichsten für Grün
    float gray = dot(col, vec3(0.2126, 0.7152, 0.0722));
    return vec3(gray);
}

vec3 invert(vec3 col) {
    return 1.0 - col;
}

vec3 sepia(vec3 col) {
    return vec3(
        dot(col, vec3(0.393, 0.769, 0.189)),
        dot(col, vec3(0.349, 0.686, 0.168)),
        dot(col, vec3(0.272, 0.534, 0.131))
    );
}

// Kernel-basierte Effekte (Convolution)
// Ein Kernel ist eine 3x3 Matrix die jeden Pixel mit seinen Nachbarn kombiniert
vec3 applyKernel(float kernel[9]) {
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
    vec3 result = vec3(0.0);

    // Offset-Reihenfolge: oben-links → unten-rechts
    vec2 offsets[9] = vec2[](
        vec2(-1.0, 1.0),  vec2(0.0, 1.0),  vec2(1.0, 1.0),
        vec2(-1.0, 0.0),  vec2(0.0, 0.0),  vec2(1.0, 0.0),
        vec2(-1.0,-1.0),  vec2(0.0,-1.0),  vec2(1.0,-1.0)
    );

    for (int i = 0; i < 9; i++)
        result += texture(screenTexture, TexCoord + offsets[i] * texelSize).rgb * kernel[i];

    return result;
}

vec3 blur() {
    // Box Blur: alle 9 Nachbarn gleichgewichtet mitteln
    float kernel[9] = float[](
        1.0/9.0, 1.0/9.0, 1.0/9.0,
        1.0/9.0, 1.0/9.0, 1.0/9.0,
        1.0/9.0, 1.0/9.0, 1.0/9.0
    );
    return applyKernel(kernel);
}

vec3 sharpen() {
    // Verstärkt Kanten durch negative Nachbar-Gewichte
    float kernel[9] = float[](
        -1.0, -1.0, -1.0,
        -1.0,  9.0, -1.0,
        -1.0, -1.0, -1.0
    );
    return applyKernel(kernel);
}

vec3 edgeDetection() {
    // Erkennt Kanten — schwarzer Hintergrund, weiße Konturen
    float kernel[9] = float[](
        1.0,  1.0,  1.0,
        1.0, -8.0,  1.0,
        1.0,  1.0,  1.0
    );
    return applyKernel(kernel);
}

vec3 vignette(vec3 col) {
    // Dunkelt die Bildränder ab (Film-Effekt)
    vec2 uv       = TexCoord - vec2(0.5);
    float dist    = length(uv);
    float strength = smoothstep(0.3, 0.75, dist);
    return col * (1.0 - strength);
}

void main() {
    vec3 col = texture(screenTexture, TexCoord).rgb;

    if      (effectMode == 0) FragColor = vec4(col,                    1.0);
    else if (effectMode == 1) FragColor = vec4(grayscale(col),         1.0);
    else if (effectMode == 2) FragColor = vec4(invert(col),            1.0);
    else if (effectMode == 3) FragColor = vec4(sepia(col),             1.0);
    else if (effectMode == 4) FragColor = vec4(blur(),                 1.0);
    else if (effectMode == 5) FragColor = vec4(sharpen(),              1.0);
    else if (effectMode == 6) FragColor = vec4(edgeDetection(),        1.0);
    else if (effectMode == 7) FragColor = vec4(vignette(col),          1.0);
    else                      FragColor = vec4(col,                    1.0);
}
```

---

## Vollständige Scene-Implementierung

### `PostProcessingScene.h`

```cpp
#pragma once
#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

class PostProcessingScene : public Scene {
private:
    // Szenen-Shader (normale Beleuchtung)
    ShaderProgram* sceneShaderPtr = nullptr;
    // Post-Process-Shader (Fullscreen Quad)
    ShaderProgram* postShaderPtr  = nullptr;

    // Szenen-Geometrie
    VertexArray*  cubeVAO  = nullptr;
    VertexBuffer* cubeVBO  = nullptr;
    VertexArray*  floorVAO = nullptr;
    VertexBuffer* floorVBO = nullptr;

    // Fullscreen Quad
    VertexArray*  quadVAO  = nullptr;
    VertexBuffer* quadVBO  = nullptr;

    // FBO
    GLuint fbo          = 0;
    GLuint colorTexture = 0;
    GLuint rbo          = 0;

    unsigned int brickTexture;

    FPSCamera camera{ glm::vec3(0.0f, 2.0f, 8.0f) };
    float deltaTime    = 0.0f;
    float lastX        = 640.0f;
    float lastY        = 400.0f;
    bool  firstMouse   = true;
    bool  cursorHidden = true;
    float aspectRatio  = 1280.0f / 800.0f;
    int   windowWidth  = 1280;
    int   windowHeight = 800;

    int   effectMode   = 0;
    const char* effectNames[8] = {
        "Normal", "Graustufen", "Invertiert", "Sepia",
        "Blur", "Sharpen", "Kanten", "Vignette"
    };

public:
    PostProcessingScene(const std::string& name) : Scene(name) {}

    void InitScene(GLFWwindow* window) override {
        sceneShaderPtr = new ShaderProgram(
            programPath("Main/Scenes/PostProcessingScene/Shaders/scene.glsl"));
        postShaderPtr = new ShaderProgram(
            programPath("Main/Scenes/PostProcessingScene/Shaders/postprocess.glsl"));

        setupQuad();
        setupSceneGeometry();
        setupFBO(windowWidth, windowHeight);

        brickTexture = TextureManager::Get().LoadTexture(
            programPath("Main/Scenes/CatCubes3DScene/Assets/soka_blue_cutie.png"));

        postShaderPtr->Bind();
        postShaderPtr->setUniform1i("screenTexture", 0);

        sceneShaderPtr->Bind();
        sceneShaderPtr->setUniform1i("texture0", 0);

        glEnable(GL_DEPTH_TEST);

        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Update(float dt) override { deltaTime = dt; }

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

    void OnResize(float ar) override {
        aspectRatio = ar;
        // FBO muss neu erstellt werden wenn das Fenster die Größe ändert!
        windowWidth  = static_cast<int>(ar * windowHeight);
        recreateFBO(windowWidth, windowHeight);
    }

    void Render() override {
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);

        // ── Pass 1: Szene in FBO rendern ─────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShaderPtr->Bind();
        sceneShaderPtr->setUniformMatrix4fv("view", view);
        sceneShaderPtr->setUniformMatrix4fv("proj", proj);
        bindTexture(brickTexture, 0);

        // Boden
        glm::mat4 model = glm::mat4(1.0f);
        sceneShaderPtr->setUniformMatrix4fv("model", model);
        floorVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        floorVAO->UnBind();

        // Würfel
        for (int i = 0; i < 5; i++) {
            model = glm::translate(glm::mat4(1.0f),
                glm::vec3((i - 2) * 2.0f, 0.5f, 0.0f));
            model = glm::rotate(model, glm::radians(i * 18.0f), glm::vec3(0,1,0));
            sceneShaderPtr->setUniformMatrix4fv("model", model);
            cubeVAO->Bind();
            glDrawArrays(GL_TRIANGLES, 0, 36);
            cubeVAO->UnBind();
        }

        // ── Pass 2: Fullscreen Quad mit Post-Processing ───────────
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        postShaderPtr->Bind();
        postShaderPtr->setUniform1i("effectMode", effectMode);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTexture);

        quadVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        quadVAO->UnBind();
    }

    void ImGuiLayer() override {
        ImGui::Begin("Post-Processing", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Effekt:");
        for (int i = 0; i < 8; i++) {
            if (ImGui::RadioButton(effectNames[i], effectMode == i))
                effectMode = i;
            if (i < 7) ImGui::SameLine();
        }
        ImGui::Separator();
        ImGui::Text("FBO Größe: %dx%d", windowWidth, windowHeight);
        ImGui::Text("TAB: Kamera-Kontrolle");
        ImGui::End();
    }

    ~PostProcessingScene() {
        delete sceneShaderPtr;
        delete postShaderPtr;
        delete cubeVAO;  delete cubeVBO;
        delete floorVAO; delete floorVBO;
        delete quadVAO;  delete quadVBO;
        if (fbo)          glDeleteFramebuffers(1, &fbo);
        if (colorTexture) glDeleteTextures(1, &colorTexture);
        if (rbo)          glDeleteRenderbuffers(1, &rbo);
    }

private:
    void setupFBO(int w, int h) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Color-Textur
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        // Depth+Stencil Renderbuffer
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "ERROR::FRAMEBUFFER: Nicht komplett!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void recreateFBO(int w, int h) {
        // Alt löschen
        if (fbo)          { glDeleteFramebuffers(1, &fbo);   fbo = 0; }
        if (colorTexture) { glDeleteTextures(1, &colorTexture); colorTexture = 0; }
        if (rbo)          { glDeleteRenderbuffers(1, &rbo);  rbo = 0; }
        setupFBO(w, h);
    }

    void setupQuad() {
        float verts[] = {
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
        };
        quadVAO = new VertexArray();
        quadVBO = new VertexBuffer(verts, sizeof(verts));
        quadVAO->Bind(); quadVBO->Bind();
        VertexBufferLayout layout;
        layout.AddElement<float>(2); // NDC-Position
        layout.AddElement<float>(2); // TexCoord
        quadVAO->AddBuffer(*quadVBO, layout);
        quadVAO->UnBind(); quadVBO->UnBind();
    }

    void setupSceneGeometry() {
        // Boden
        float floor[] = {
            -8.f,0.f,-8.f, 0.f,8.f,  8.f,0.f, 8.f, 8.f,0.f,  8.f,0.f,-8.f, 8.f,8.f,
             8.f,0.f, 8.f, 8.f,0.f, -8.f,0.f,-8.f, 0.f,8.f,  -8.f,0.f, 8.f, 0.f,0.f,
        };
        floorVAO = new VertexArray();
        floorVBO = new VertexBuffer(floor, sizeof(floor));
        floorVAO->Bind(); floorVBO->Bind();
        VertexBufferLayout fl;
        fl.AddElement<float>(3);
        fl.AddElement<float>(2);
        floorVAO->AddBuffer(*floorVBO, fl);
        floorVAO->UnBind(); floorVBO->UnBind();

        // Würfel (pos3 + tex2, wie in CatCubes3DScene)
        cubeVAO = new VertexArray();
        cubeVBO = new VertexBuffer(cubeVerts.data(),
            static_cast<GLuint>(cubeVerts.size() * sizeof(float)));
        cubeVAO->Bind(); cubeVBO->Bind();
        VertexBufferLayout cl;
        cl.AddElement<float>(3);
        cl.AddElement<float>(2);
        cubeVAO->AddBuffer(*cubeVBO, cl);
        cubeVAO->UnBind(); cubeVBO->UnBind();
    }

    const std::vector<float> cubeVerts = {
        -0.5f,-0.5f,-0.5f,0.0f,0.0f,  0.5f,-0.5f,-0.5f,1.0f,0.0f,
         0.5f, 0.5f,-0.5f,1.0f,1.0f,  0.5f, 0.5f,-0.5f,1.0f,1.0f,
        -0.5f, 0.5f,-0.5f,0.0f,1.0f, -0.5f,-0.5f,-0.5f,0.0f,0.0f,
        -0.5f,-0.5f, 0.5f,0.0f,0.0f,  0.5f,-0.5f, 0.5f,1.0f,0.0f,
         0.5f, 0.5f, 0.5f,1.0f,1.0f,  0.5f, 0.5f, 0.5f,1.0f,1.0f,
        -0.5f, 0.5f, 0.5f,0.0f,1.0f, -0.5f,-0.5f, 0.5f,0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,1.0f,0.0f, -0.5f, 0.5f,-0.5f,1.0f,1.0f,
        -0.5f,-0.5f,-0.5f,0.0f,1.0f, -0.5f,-0.5f,-0.5f,0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,0.0f,0.0f, -0.5f, 0.5f, 0.5f,1.0f,0.0f,
         0.5f, 0.5f, 0.5f,1.0f,0.0f,  0.5f, 0.5f,-0.5f,1.0f,1.0f,
         0.5f,-0.5f,-0.5f,0.0f,1.0f,  0.5f,-0.5f,-0.5f,0.0f,1.0f,
         0.5f,-0.5f, 0.5f,0.0f,0.0f,  0.5f, 0.5f, 0.5f,1.0f,0.0f,
        -0.5f,-0.5f,-0.5f,0.0f,1.0f,  0.5f,-0.5f,-0.5f,1.0f,1.0f,
         0.5f,-0.5f, 0.5f,1.0f,0.0f,  0.5f,-0.5f, 0.5f,1.0f,0.0f,
        -0.5f,-0.5f, 0.5f,0.0f,0.0f, -0.5f,-0.5f,-0.5f,0.0f,1.0f,
        -0.5f, 0.5f,-0.5f,0.0f,1.0f,  0.5f, 0.5f,-0.5f,1.0f,1.0f,
         0.5f, 0.5f, 0.5f,1.0f,0.0f,  0.5f, 0.5f, 0.5f,1.0f,0.0f,
        -0.5f, 0.5f, 0.5f,0.0f,0.0f, -0.5f, 0.5f,-0.5f,0.0f,1.0f,
    };
};
```

---

## Shader-Dateien

### `Shaders/scene.glsl` (simple Textur-Scene)

```glsl
#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec2 TexCoord;

void main() {
    gl_Position = proj * view * model * vec4(aPos, 1.0);
    TexCoord    = aTexCoord;
}

#shader fragment
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture0;

void main() {
    FragColor = texture(texture0, TexCoord);
}
```

### `Shaders/postprocess.glsl`

Identisch mit dem großen Effekt-Shader oben (mit `effectMode` uniform).

---

## Wichtige Erweiterungen

### HDR (High Dynamic Range)

Statt `GL_RGB` (0–255 pro Kanal) nutzt du `GL_RGBA16F` für die FBO-Textur. Das erlaubt Helligkeitswerte über 1.0, die du dann mit **Tone Mapping** auf 0–1 reduzierst:

```cpp
// FBO-Textur mit 16-bit float Genauigkeit
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
```

```glsl
// Reinhard Tone Mapping im Post-Process-Shader
vec3 hdrColor = texture(screenTexture, TexCoord).rgb;
vec3 mapped   = hdrColor / (hdrColor + vec3(1.0));
// Gamma Correction
mapped = pow(mapped, vec3(1.0 / 2.2));
FragColor = vec4(mapped, 1.0);
```

### Bloom

Bloom braucht **zwei FBOs**:
1. Rendere die Szene in FBO 1
2. Filtere nur helle Pixel heraus (Brightness Threshold)
3. Blur diese hellen Pixel (in FBO 2) — mehrfach für stärkere Unschärfe
4. Addiere FBO 1 + FBO 2

```glsl
// Brightness-Extraktion
float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
if (brightness > 1.0)
    brightColor = vec4(color, 1.0);
```

---

## Häufige Fehler

1. **Depth Test vergessen zu deaktivieren für den Quad-Pass**: Wenn `GL_DEPTH_TEST` aktiv ist, kann der Quad (der bei z=0 liegt) durch vorherige Geometrie verdeckt werden.

2. **FBO-Größe stimmt nicht mit Viewport überein**: Wenn du das Fenster verkleinerst ohne `recreateFBO()` aufzurufen, rendert OpenGL in eine zu große Textur und skaliert sie runter — es sieht verschwommen aus.

3. **`glClear` im Default Framebuffer**: Vergisst du `glClear` nach `glBindFramebuffer(GL_FRAMEBUFFER, 0)`, scheint die vorherige Szene durch.

4. **sRGB / Gamma**: Wenn Farben nach Post-Processing zu dunkel oder gesättigt wirken, liegt es oft an fehlendem Gamma-Correction. OpenGL rendert in linearem Farbraum, Bildschirme erwarten sRGB (Gamma ≈ 2.2).
