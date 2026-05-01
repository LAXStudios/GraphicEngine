# Instanced Rendering

## Warum brauchst du das?

In deiner `CatCubes3DScene` rufst du `renderer.draw(...)` in einer Schleife auf — einmal pro Cube, 10 Mal. Das funktioniert für 10 Objekte problemlos. Aber stell dir vor, du willst ein Lego-Terrain mit **50.000 Bricks** rendern.

Jeder `draw`-Aufruf ist ein **Draw Call** — ein teurer Befehl an die GPU, bei dem die CPU sagt: "Zeichne dieses Objekt jetzt." Die GPU muss warten, bis die CPU den nächsten Befehl schickt. Bei 50.000 Draw Calls pro Frame (60 FPS) macht das **3 Millionen CPU→GPU-Kommunikationen pro Sekunde**. Das ist der Bottleneck, keine Rechenleistung.

**Instanced Rendering** löst das: Du schickst **alle 50.000 Positionen einmalig** zur GPU und sagst dann mit einem einzigen Draw Call: "Zeichne dieses Mesh 50.000 mal, hier sind alle Transformationen."

| Methode | Draw Calls | Typische FPS (50k Bricks) |
|---|---|---|
| Naive Schleife | 50.000 | ~2 FPS |
| Instanced Rendering | 1 | ~200 FPS |

---

## Das Grundprinzip

Normal funktioniert ein Vertex-Shader so:
```glsl
// Jeder Vertex bekommt dieselbe uniform-Matrix
uniform mat4 model;
```

Du rufst `setUniformMatrix4fv("model", matrix)` für jedes Objekt auf — das ist die CPU→GPU-Kommunikation, die teuer ist.

Bei Instancing gibst du die Matrices **nicht als Uniform**, sondern als **Vertex Attribute** direkt im VBO mit:
```glsl
// layout(location = 2) ist jetzt eine per-INSTANZ Matrix, nicht per-Vertex
layout(location = 2) in mat4 instanceModel;
```

Die GPU liest `instanceModel` nicht für jeden Vertex neu, sondern für jede **Instanz**. Das steuert `glVertexAttribDivisor`.

### glVertexAttribDivisor

```cpp
glVertexAttribDivisor(attrib_index, divisor);
```

- `divisor = 0` → Attribut wechselt pro Vertex (normal, Standard)
- `divisor = 1` → Attribut wechselt pro Instanz (einmal pro Objekt)
- `divisor = 2` → Attribut wechselt alle 2 Instanzen, usw.

### mat4 als Vertex Attribute

Eine `mat4` braucht 4 Attribute-Slots (weil ein Slot maximal `vec4` fasst):

```cpp
// mat4 = 4 vec4 = 4 Slots
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);

glEnableVertexAttribArray(3);
glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));

glEnableVertexAttribArray(4);
glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));

glEnableVertexAttribArray(5);
glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

// Alle 4 Slots sollen sich pro Instanz ändern, nicht pro Vertex
glVertexAttribDivisor(2, 1);
glVertexAttribDivisor(3, 1);
glVertexAttribDivisor(4, 1);
glVertexAttribDivisor(5, 1);
```

---

## Schritt-für-Schritt Implementierung

### Schritt 1: Die Instanz-Matrices berechnen

```cpp
std::vector<glm::mat4> instanceMatrices;
instanceMatrices.reserve(instanceCount);

for (int x = 0; x < gridWidth; x++) {
    for (int z = 0; z < gridDepth; z++) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x * spacing, 0.0f, z * spacing));
        instanceMatrices.push_back(model);
    }
}
```

### Schritt 2: Instance-VBO erstellen

```cpp
GLuint instanceVBO;
glGenBuffers(1, &instanceVBO);
glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
glBufferData(
    GL_ARRAY_BUFFER,
    instanceMatrices.size() * sizeof(glm::mat4),
    instanceMatrices.data(),
    GL_STATIC_DRAW   // oder DYNAMIC_DRAW wenn du sie zur Laufzeit änderst
);
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

### Schritt 3: Vertex Attribute konfigurieren (muss nach VAO-Bind passieren)

```cpp
glBindVertexArray(vaoID);
glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

for (int i = 0; i < 4; i++) {
    glEnableVertexAttribArray(2 + i);
    glVertexAttribPointer(
        2 + i,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::mat4),
        (void*)(i * sizeof(glm::vec4))
    );
    glVertexAttribDivisor(2 + i, 1);
}

glBindVertexArray(0);
```

### Schritt 4: Instanced Draw Call

```cpp
// Statt:
// for (int i = 0; i < count; i++) renderer.draw(vao, shader, 36);

// Einmal:
glBindVertexArray(vaoID);
glDrawArraysInstanced(GL_TRIANGLES, 0, 36, instanceCount);
glBindVertexArray(0);
```

### Schritt 5: Vertex Shader anpassen

```glsl
#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
// Instanz-Matrix (4 Slots, weil mat4)
layout(location = 2) in mat4 instanceModel;

uniform mat4 view;
uniform mat4 proj;

out vec2 TexCoord;

void main() {
    gl_Position = proj * view * instanceModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
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

---

## Vollständige Scene-Implementierung

Erstelle folgende Struktur:
```
src/Main/Scenes/InstancedScene/
├── InstancedScene.h
└── Shaders/
    └── instanced.glsl
```

### `InstancedScene.h`

```cpp
#pragma once
#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vector>

class InstancedScene : public Scene {
private:
    ShaderProgram* shaderPtr  = nullptr;
    VertexArray*   vaoPtr     = nullptr;
    VertexBuffer*  vboPtr     = nullptr;

    GLuint instanceVBO = 0;
    GLuint instanceCount = 0;

    unsigned int texture;

    glm::mat4 view;
    glm::mat4 proj;

    FPSCamera camera{ glm::vec3(0.0f, 5.0f, 15.0f) };
    float deltaTime   = 0.0f;
    float lastX       = 640.0f;
    float lastY       = 400.0f;
    bool  firstMouse  = true;
    bool  cursorHidden = true;
    float aspectRatio = 1280.0f / 800.0f;

    // Grid-Konfiguration (änderbar via ImGui)
    int   gridW   = 50;
    int   gridD   = 50;
    float spacing = 1.1f;

public:
    InstancedScene(const std::string& name) : Scene(name) {}

    void InitScene(GLFWwindow* window) override {
        shaderPtr = new ShaderProgram(
            programPath("Main/Scenes/InstancedScene/Shaders/instanced.glsl"));

        // Cube-Geometrie (Position + TexCoord, kein Normal für jetzt)
        vaoPtr = new VertexArray();
        vboPtr = new VertexBuffer(cubeVertices.data(),
            static_cast<GLuint>(cubeVertices.size() * sizeof(float)));

        vaoPtr->Bind();
        vboPtr->Bind();

        VertexBufferLayout layout;
        layout.AddElement<float>(3); // Position
        layout.AddElement<float>(2); // TexCoord
        vaoPtr->AddBuffer(*vboPtr, layout);

        // Instance VBO aufbauen
        rebuildInstanceBuffer();

        vaoPtr->UnBind();

        texture = TextureManager::Get().LoadTexture(
            programPath("Main/Scenes/CatCubes3DScene/Assets/soka_blue_cutie.png"));

        shaderPtr->Bind();
        shaderPtr->setUniform1i("texture0", 0);

        proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 200.0f);

        glEnable(GL_DEPTH_TEST);

        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

    void Update(float dt) override {
        deltaTime = dt;
    }

    void OnResize(float ar) override {
        aspectRatio = ar;
    }

    void Render() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        view = camera.GetViewMatrix();
        proj = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 500.0f);

        shaderPtr->Bind();
        shaderPtr->setUniformMatrix4fv("view", view);
        shaderPtr->setUniformMatrix4fv("proj", proj);

        bindTexture(texture, 0);

        // Ein einziger Draw Call für ALLE Instanzen
        vaoPtr->Bind();
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, instanceCount);
        vaoPtr->UnBind();
    }

    void ImGuiLayer() override {
        ImGui::Begin("Instanced Rendering", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Instanzen: %u", instanceCount);
        ImGui::Text("Draw Calls: 1");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Separator();

        bool rebuild = false;
        rebuild |= ImGui::SliderInt("Grid Breite", &gridW, 1, 200);
        rebuild |= ImGui::SliderInt("Grid Tiefe",  &gridD, 1, 200);
        rebuild |= ImGui::SliderFloat("Abstand",   &spacing, 0.5f, 3.0f);

        if (rebuild) {
            vaoPtr->Bind();
            rebuildInstanceBuffer();
            vaoPtr->UnBind();
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1,1,0,1), "TAB: Kamera-Kontrolle");
        ImGui::End();
    }

    ~InstancedScene() {
        delete shaderPtr;
        delete vaoPtr;
        delete vboPtr;
        if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    }

private:
    void rebuildInstanceBuffer() {
        // Alten Buffer löschen falls vorhanden
        if (instanceVBO) {
            glDeleteBuffers(1, &instanceVBO);
            instanceVBO = 0;
        }

        // Matrices für jede Instanz berechnen
        std::vector<glm::mat4> matrices;
        matrices.reserve(gridW * gridD);

        for (int x = 0; x < gridW; x++) {
            for (int z = 0; z < gridD; z++) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(
                    x * spacing - (gridW * spacing * 0.5f),
                    0.0f,
                    z * spacing - (gridD * spacing * 0.5f)
                ));
                matrices.push_back(model);
            }
        }

        instanceCount = static_cast<GLuint>(matrices.size());

        // VBO für die Instanz-Matrices anlegen
        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER,
            matrices.size() * sizeof(glm::mat4),
            matrices.data(),
            GL_DYNAMIC_DRAW);

        // mat4 belegt 4 aufeinanderfolgende Attribute-Slots (2,3,4,5)
        // weil ein Slot nur vec4 (= 4 floats) halten kann
        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(2 + i);
            glVertexAttribPointer(
                2 + i,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(glm::mat4),
                (void*)(i * sizeof(glm::vec4))
            );
            // divisor=1: Attribut wechselt einmal pro INSTANZ, nicht pro Vertex
            glVertexAttribDivisor(2 + i, 1);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    const std::vector<float> cubeVertices = {
        -0.5f,-0.5f,-0.5f, 0.0f,0.0f,  0.5f,-0.5f,-0.5f, 1.0f,0.0f,
         0.5f, 0.5f,-0.5f, 1.0f,1.0f,  0.5f, 0.5f,-0.5f, 1.0f,1.0f,
        -0.5f, 0.5f,-0.5f, 0.0f,1.0f, -0.5f,-0.5f,-0.5f, 0.0f,0.0f,

        -0.5f,-0.5f, 0.5f, 0.0f,0.0f,  0.5f,-0.5f, 0.5f, 1.0f,0.0f,
         0.5f, 0.5f, 0.5f, 1.0f,1.0f,  0.5f, 0.5f, 0.5f, 1.0f,1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f,1.0f, -0.5f,-0.5f, 0.5f, 0.0f,0.0f,

        -0.5f, 0.5f, 0.5f, 1.0f,0.0f, -0.5f, 0.5f,-0.5f, 1.0f,1.0f,
        -0.5f,-0.5f,-0.5f, 0.0f,1.0f, -0.5f,-0.5f,-0.5f, 0.0f,1.0f,
        -0.5f,-0.5f, 0.5f, 0.0f,0.0f, -0.5f, 0.5f, 0.5f, 1.0f,0.0f,

         0.5f, 0.5f, 0.5f, 1.0f,0.0f,  0.5f, 0.5f,-0.5f, 1.0f,1.0f,
         0.5f,-0.5f,-0.5f, 0.0f,1.0f,  0.5f,-0.5f,-0.5f, 0.0f,1.0f,
         0.5f,-0.5f, 0.5f, 0.0f,0.0f,  0.5f, 0.5f, 0.5f, 1.0f,0.0f,

        -0.5f,-0.5f,-0.5f, 0.0f,1.0f,  0.5f,-0.5f,-0.5f, 1.0f,1.0f,
         0.5f,-0.5f, 0.5f, 1.0f,0.0f,  0.5f,-0.5f, 0.5f, 1.0f,0.0f,
        -0.5f,-0.5f, 0.5f, 0.0f,0.0f, -0.5f,-0.5f,-0.5f, 0.0f,1.0f,

        -0.5f, 0.5f,-0.5f, 0.0f,1.0f,  0.5f, 0.5f,-0.5f, 1.0f,1.0f,
         0.5f, 0.5f, 0.5f, 1.0f,0.0f,  0.5f, 0.5f, 0.5f, 1.0f,0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f,0.0f, -0.5f, 0.5f,-0.5f, 0.0f,1.0f,
    };
};
```

### `Shaders/instanced.glsl`

```glsl
#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

// mat4 belegt 4 Slots: location 2, 3, 4, 5
// glVertexAttribDivisor sorgt dafür dass diese sich pro Instanz ändern
layout(location = 2) in mat4 instanceModel;

uniform mat4 view;
uniform mat4 proj;

out vec2 TexCoord;

void main() {
    gl_Position = proj * view * instanceModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
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

---

## Lego Terrain Erweiterung

Für einen Lego Terrain Generator brauchst du Height-Informationen pro Instanz. Das machst du so:

### Erweiterte Instance-Daten (statt nur Matrix)

```cpp
struct BrickInstance {
    glm::mat4 model;     // Position + Rotation + Scale
    glm::vec3 color;     // Lego-Farbe
    float     padding;   // Alignment auf 16 Bytes (GPU erwartet das)
};
```

### Height Map zu Lego-Grid

```cpp
// Simplex/Perlin Noise Wert (0.0 - 1.0) → Brick-Stack-Höhe
float noiseValue = /* ... */;
int stackHeight = static_cast<int>(noiseValue * maxHeight);

for (int y = 0; y < stackHeight; y++) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y * brickHeight, z));
    instances.push_back({ model, legoColorForHeight(y) });
}
```

### Color im Shader nutzen

```glsl
layout(location = 6) in vec3 instanceColor;

out vec3 vColor;

void main() {
    vColor = instanceColor;
    // ...
}
```

---

## Dynamische Updates (DYNAMIC_DRAW)

Wenn sich die Bricks zur Laufzeit ändern (z.B. weil der Spieler die Terrain-Parameter ändert):

```cpp
// Nur den Buffer neu befüllen, nicht neu erstellen
glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
glBufferSubData(GL_ARRAY_BUFFER, 0,
    newMatrices.size() * sizeof(glm::mat4),
    newMatrices.data());
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

`glBufferSubData` ist schneller als `glBufferData`, weil kein neues Objekt angelegt wird.

---

## Häufige Fehler

1. **`instanceVBO` muss an den VAO gebunden sein** wenn du `glVertexAttribPointer` aufrufst — also zwischen `vaoPtr->Bind()` und `vaoPtr->UnBind()`.

2. **Vergiss nicht `glVertexAttribDivisor(i, 1)`** — ohne das behandelt OpenGL die Instance-Matrix als normales per-Vertex Attribut und rendert Unsinn.

3. **`glDrawArraysInstanced` nicht `glDrawArrays`** — der letzte Parameter ist die Anzahl der Instanzen.

4. **mat4 = 4 Slots** — du musst Slots 2, 3, 4, 5 alle aktivieren. Wenn du den Cube-Shader mit Normals erweiterst (location 2 = Normal), musst du die Matrix auf Slot 3-6 verschieben.
