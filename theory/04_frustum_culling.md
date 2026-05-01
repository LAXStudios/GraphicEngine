# Frustum Culling

## Was ist Frustum Culling?

Der **View Frustum** ist der Sichtbereich deiner Kamera — ein abgeschnittenes Pyramid-Volumen, das genau das enthält, was die Kamera sehen kann. Alles außerhalb dieses Bereichs landet nie auf dem Bildschirm.

Das Problem: Ohne Culling sendet deine CPU trotzdem Draw Calls für alle Objekte, auch die hinter der Kamera oder weit außerhalb des Sichtfeldes. Die GPU verwirft diese dann im Clipping-Schritt zwar, aber **die CPU-Arbeit (Draw Call aufstellen, Matrizen setzen) ist bereits verschwendet**.

Bei einem Lego-Terrain mit 100×100 Bricks siehst du vielleicht nur 30% der Bricks gleichzeitig. Ohne Culling: 10.000 Draw Calls. Mit Culling: ~3.000 Draw Calls. **3x weniger CPU-Arbeit.**

```
Frustum = [Near Plane] -------- [Far Plane]
              klein                 groß
          (Kamera hier)
```

### Sechs Planes

Das Frustum besteht aus genau **6 Ebenen**:
- **Near** (vorne, nah)
- **Far** (hinten, weit)
- **Left** / **Right** (Seiten)
- **Top** / **Bottom** (oben/unten)

Ein Objekt ist sichtbar, wenn es **auf der positiven Seite aller 6 Planes** liegt (oder zumindest eine Ecke davon).

---

## Frustum Planes aus der View-Projection-Matrix extrahieren

Das Elegante an dieser Methode: Du brauchst keine Geometrie. Du extrahierst die Planes direkt aus der `proj * view` Matrix.

Diese Methode geht auf **Gil Gribb & Klaus Hartmann** zurück und funktioniert so: In Clip-Space ist ein Punkt sichtbar wenn:
```
-w <= x <= w
-w <= y <= w
 0 <= z <= w   (OpenGL: -w <= z <= w)
```

Die 6 Planes ergeben sich aus den Zeilen der Clip-Matrix:

```
Links:   row3 + row0
Rechts:  row3 - row0
Unten:   row3 + row1
Oben:    row3 - row1
Near:    row3 + row2
Far:     row3 - row2
```

### Plane-Gleichung

Eine Ebene wird durch `(normal, distance)` beschrieben: `dot(normal, point) + distance = 0`.
Ein Punkt liegt **vor** der Ebene wenn: `dot(normal, point) + distance > 0`.

---

## Implementierung: Frustum-Klasse

```cpp
// Frustum.h  (neue Datei in src/Headers/Core/)
#pragma once
#include <glm/glm.hpp>
#include <array>

struct Plane {
    glm::vec3 normal   = { 0.f, 1.f, 0.f };
    float     distance = 0.f;

    // Vorzeichenbehafteter Abstand eines Punktes zur Plane
    float distanceTo(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

struct AABB {
    glm::vec3 center;
    glm::vec3 extents; // halbe Größe (halfSize)

    // Erstellt eine AABB aus min/max Punkten
    static AABB fromMinMax(const glm::vec3& min, const glm::vec3& max) {
        return { (min + max) * 0.5f, (max - min) * 0.5f };
    }

    // Erstellt eine AABB für einen Einheits-Würfel mit gegebener Transformation
    static AABB fromTransform(const glm::vec3& pos, const glm::vec3& scale = glm::vec3(1.f)) {
        return { pos, scale * 0.5f };
    }
};

class Frustum {
public:
    std::array<Plane, 6> planes;

    // Extrahiert die 6 Frustum-Planes aus der kombinierten ViewProjection-Matrix
    // Methode nach Gribb & Hartmann
    void extractPlanes(const glm::mat4& vp) {
        // vp[col][row] in GLM (column-major!)
        // Plane = row3 ± rowN der transponierten Matrix

        // Links:  row3 + row0
        planes[0].normal   = glm::vec3(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0]);
        planes[0].distance = vp[3][3] + vp[3][0];

        // Rechts: row3 - row0
        planes[1].normal   = glm::vec3(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0]);
        planes[1].distance = vp[3][3] - vp[3][0];

        // Unten:  row3 + row1
        planes[2].normal   = glm::vec3(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1]);
        planes[2].distance = vp[3][3] + vp[3][1];

        // Oben:   row3 - row1
        planes[3].normal   = glm::vec3(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1]);
        planes[3].distance = vp[3][3] - vp[3][1];

        // Near:   row3 + row2
        planes[4].normal   = glm::vec3(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2]);
        planes[4].distance = vp[3][3] + vp[3][2];

        // Far:    row3 - row2
        planes[5].normal   = glm::vec3(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2]);
        planes[5].distance = vp[3][3] - vp[3][2];

        // Planes normalisieren (für korrekte Distanz-Berechnung)
        for (auto& plane : planes) {
            float len = glm::length(plane.normal);
            plane.normal   /= len;
            plane.distance /= len;
        }
    }

    // Prüft ob eine AABB im Frustum liegt (oder es schneidet)
    bool isAABBVisible(const AABB& aabb) const {
        for (const auto& plane : planes) {
            // "Positive half-space extent" der AABB in Richtung der Plane-Normal
            // = größte mögliche Projektion der AABB auf die Normal
            float r = glm::dot(aabb.extents, glm::abs(plane.normal));

            // Wenn das nächste Extrem der AABB hinter der Plane liegt → unsichtbar
            if (plane.distanceTo(aabb.center) < -r)
                return false;
        }
        return true;
    }

    // Vereinfachter Sphere-Test (schneller, aber etwas ungenauer)
    bool isSphereVisible(const glm::vec3& center, float radius) const {
        for (const auto& plane : planes) {
            if (plane.distanceTo(center) < -radius)
                return false;
        }
        return true;
    }
};
```

---

## Integration in eine Scene

### `FrustumCullingScene.h`

```cpp
#pragma once
#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/TextureManager/TextureManager.h"
// Frustum.h liegt in Headers/Core/Frustum/Frustum.h
// (oder du kopierst die Klassen direkt in diese Datei)
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vector>
#include <random>

class FrustumCullingScene : public Scene {
private:
    ShaderProgram* shaderPtr  = nullptr;
    VertexArray*   vaoPtr     = nullptr;
    VertexBuffer*  vboPtr     = nullptr;

    unsigned int texture;

    FPSCamera camera{ glm::vec3(0.0f, 5.0f, 0.0f) };
    float deltaTime    = 0.0f;
    float lastX        = 640.0f;
    float lastY        = 400.0f;
    bool  firstMouse   = true;
    bool  cursorHidden = true;
    float aspectRatio  = 1280.0f / 800.0f;

    // Weltdaten
    struct ObjectData {
        glm::vec3 position;
        glm::vec3 scale;
        float     rotY;
    };
    std::vector<ObjectData> objects;

    Frustum  frustum;
    bool     cullingEnabled = true;
    int      lastVisibleCount = 0;
    int      totalCount       = 0;

public:
    FrustumCullingScene(const std::string& name) : Scene(name) {}

    void InitScene(GLFWwindow* window) override {
        shaderPtr = new ShaderProgram(
            programPath("Main/Scenes/FrustumCullingScene/Shaders/shader.glsl"));

        vaoPtr = new VertexArray();
        vboPtr = new VertexBuffer(cubeVerts.data(),
            static_cast<GLuint>(cubeVerts.size() * sizeof(float)));
        vaoPtr->Bind(); vboPtr->Bind();
        VertexBufferLayout layout;
        layout.AddElement<float>(3);
        layout.AddElement<float>(2);
        vaoPtr->AddBuffer(*vboPtr, layout);
        vaoPtr->UnBind(); vboPtr->UnBind();

        texture = TextureManager::Get().LoadTexture(
            programPath("Main/Scenes/CatCubes3DScene/Assets/soka_blue_cutie.png"));

        shaderPtr->Bind();
        shaderPtr->setUniform1i("texture0", 0);

        generateWorld(200);   // 200 zufällig platzierte Objekte

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
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
            cullingEnabled = !cullingEnabled;
    }

    void OnResize(float ar) override { aspectRatio = ar; }

    void Render() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 200.0f);

        // Frustum jedes Frame neu berechnen
        frustum.extractPlanes(proj * view);

        shaderPtr->Bind();
        shaderPtr->setUniformMatrix4fv("view", view);
        shaderPtr->setUniformMatrix4fv("proj", proj);
        bindTexture(texture, 0);

        int visibleCount = 0;

        vaoPtr->Bind();
        for (const auto& obj : objects) {
            if (cullingEnabled) {
                // AABB aus Position und Scale berechnen
                AABB aabb = AABB::fromTransform(obj.position, obj.scale);

                // Frustum-Test: unsichtbare Objekte überspringen
                if (!frustum.isAABBVisible(aabb))
                    continue;
            }

            visibleCount++;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, obj.rotY, glm::vec3(0.f, 1.f, 0.f));
            model = glm::scale(model, obj.scale);
            shaderPtr->setUniformMatrix4fv("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        vaoPtr->UnBind();

        lastVisibleCount = visibleCount;
        totalCount       = static_cast<int>(objects.size());
    }

    void ImGuiLayer() override {
        ImGui::Begin("Frustum Culling", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Gesamt:    %d Objekte", totalCount);
        ImGui::Text("Sichtbar:  %d Objekte", lastVisibleCount);
        ImGui::Text("Gecullt:   %d Objekte", totalCount - lastVisibleCount);

        float ratio = totalCount > 0
            ? 100.f * (1.f - (float)lastVisibleCount / totalCount)
            : 0.f;
        ImGui::Text("Einsparung: %.1f%%", ratio);

        ImGui::Separator();
        ImGui::Checkbox("Culling aktiv (C)", &cullingEnabled);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1,1,0,1), "Drehe die Kamera weg von Objekten\num den Effekt zu sehen!");
        ImGui::End();
    }

    ~FrustumCullingScene() {
        delete shaderPtr;
        delete vaoPtr;
        delete vboPtr;
    }

private:
    void generateWorld(int count) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> posDist(-50.f, 50.f);
        std::uniform_real_distribution<float> scaleDist(0.5f, 3.0f);
        std::uniform_real_distribution<float> rotDist(0.f, glm::two_pi<float>());

        objects.reserve(count);
        for (int i = 0; i < count; i++) {
            float s = scaleDist(rng);
            objects.push_back({
                glm::vec3(posDist(rng), s * 0.5f, posDist(rng)),
                glm::vec3(s),
                rotDist(rng)
            });
        }
        totalCount = count;
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

## Frustum Culling + Instanced Rendering kombinieren

Für das Lego-Terrain willst du beides: Instancing UND Culling. Das geht so:

```cpp
void Render() override {
    glm::mat4 vp = proj * view;
    frustum.extractPlanes(vp);

    // Nur sichtbare Instanz-Matrices in einen temporären Buffer
    std::vector<glm::mat4> visibleMatrices;
    visibleMatrices.reserve(allMatrices.size());

    for (size_t i = 0; i < allMatrices.size(); i++) {
        AABB aabb = brickAABBs[i];  // vorberechnet, nicht jedes Frame neu
        if (frustum.isAABBVisible(aabb))
            visibleMatrices.push_back(allMatrices[i]);
    }

    // Instance Buffer mit sichtbaren Instanzen aktualisieren
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        visibleMatrices.size() * sizeof(glm::mat4),
        visibleMatrices.data());

    // Ein Draw Call, nur sichtbare Instanzen
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36,
        static_cast<GLsizei>(visibleMatrices.size()));
}
```

**Wichtig**: Die AABB-Tests passieren auf der CPU, aber sie sind billig (ein paar Multiplikationen). Das `glBufferSubData` aktualisiert nur den sichtbaren Teil des Buffers.

---

## Chunk-basiertes Culling (für große Terrains)

Für ein 1000×1000 Brick-Terrain willst du nicht 1.000.000 AABB-Tests pro Frame machen. Die Lösung: **Chunks**.

```
Terrain (1000×1000 Bricks)
├── Chunk [0,0]  → 32×32 Bricks  → eine AABB
├── Chunk [1,0]  → 32×32 Bricks  → eine AABB
├── ...
└── Chunk [31,31]→ 32×32 Bricks  → eine AABB

Frustum-Test: 32×32 = 1024 Chunk-Tests statt 1.000.000 Brick-Tests
```

```cpp
struct Chunk {
    AABB             aabb;
    std::vector<glm::mat4> instanceMatrices;
    GLuint           instanceVBO = 0;
    int              instanceCount = 0;
};

// Render-Loop
for (auto& chunk : chunks) {
    if (!frustum.isAABBVisible(chunk.aabb))
        continue;

    glBindBuffer(GL_ARRAY_BUFFER, chunk.instanceVBO);
    // ... bind VAO, draw instanced
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, chunk.instanceCount);
}
```

Jetzt hast du O(Chunks) statt O(Bricks) Tests — aus 1 Million werden 1024.

---

## Level of Detail (LOD) — Bonus

Weit entfernte Chunks müssen nicht hochauflösend sein. Du kannst verschiedene Mesh-Versionen vorhalten:

```cpp
enum class LODLevel { High, Medium, Low };

LODLevel getLOD(const glm::vec3& chunkCenter, const glm::vec3& cameraPos) {
    float dist = glm::distance(chunkCenter, cameraPos);
    if (dist < 30.f)  return LODLevel::High;
    if (dist < 80.f)  return LODLevel::Medium;
    return LODLevel::Low;
}
```

Für Lego-Bricks:
- **High**: vollständiger Brick mit Stud (Noppe) oben
- **Medium**: vereinfachter Brick ohne Stud-Detail
- **Low**: einfacher Box-Mesh (kaum zu unterscheiden aus der Entfernung)

---

## Performance-Vergleich: die drei Techniken kombiniert

| Technik | Draw Calls | GPU Work | CPU Work |
|---|---|---|---|
| Naive Schleife | 100.000 | hoch | sehr hoch |
| + Frustum Culling | ~30.000 | hoch | mittel |
| + Instancing | ~100 (pro Chunk) | mittel | niedrig |
| + Instancing + Culling | ~30 (sichtbare Chunks) | niedrig | sehr niedrig |

Das ist der Stack den du für das Lego-Terrain brauchst.

---

## Häufige Fehler

1. **GLM Column-Major vs Row-Major**: Der Plane-Extraktion-Code oben nutzt `vp[col][row]` — GLM ist column-major. Wenn du `vp[row][col]` schreibst, bekommst du falsche Planes.

2. **Nicht normalisieren**: Ohne `plane.normal /= len` ist `distanceTo()` kein echter Abstand in Welteinheiten — das macht den Sphere-Radius-Test unbrauchbar.

3. **AABB zu klein**: Wenn du die `extents` zu konservativ berechest (zu klein), werden Objekte an den Frustum-Rändern weggecullt obwohl sie noch teilweise sichtbar sind. Lieber etwas zu groß als zu klein — ein nicht-sichtbares Objekt zu rendern ist besser als ein sichtbares nicht zu rendern.

4. **Frustum jedes Frame neu berechnen**: Die `extractPlanes()` Methode muss **nach** der View-Matrix-Aktualisierung aufgerufen werden — also in `Render()`, nicht in `Update()`.
