# Orbit Camera (Blender-Style)

## Teil 1: Deine FPSCamera — was passiert da eigentlich?

Bevor wir die neue Kamera bauen, erkläre ich dir was die Kamera die du kopiert hast wirklich macht. Das ist wichtig, weil die neue Kamera auf denselben Grundprinzipien aufbaut.

### Was ist eine Kamera in OpenGL überhaupt?

OpenGL hat keine eingebaute Kamera. Es gibt nur eine **View Matrix** — eine 4×4 Matrix die die gesamte Welt so verschiebt und dreht, als ob du von einer bestimmten Position aus schaust. Die Kamera ist also nur eine Abstraktion um diese Matrix bequem zu berechnen.

```
View Matrix = "Bewege die Welt so, dass mein Standpunkt der Ursprung ist"
```

### glm::lookAt — das Herzstück

```cpp
glm::mat4 FPSCamera::GetViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
}
```

`glm::lookAt` braucht drei Vektoren:
- **eye** (`Position`): Wo bist du?
- **center** (`Position + Front`): Wo schaust du hin? (ein Punkt, nicht eine Richtung)
- **up** (`Up`): Was ist "oben" für deine Kamera?

Aus diesen drei Punkten berechnet GLM intern drei orthogonale Achsen und baut daraus eine Matrix. Du musst das nicht selbst machen — aber du musst verstehen was du da reingibst.

### Euler-Winkel: Yaw und Pitch

Die FPSCamera speichert keine "Richtung" direkt, sondern zwei **Winkel**:

```
Yaw   = Rotation um die Y-Achse (links/rechts schauen) — wie ein Kompass
Pitch = Rotation um die X-Achse (hoch/runter schauen) — wie Kopf neigen
```

```
       Pitch = 0°        Pitch = 45°       Pitch = -45°
         ↑                  ↗                  ↘
    (geradeaus)          (hoch)               (runter)
```

Aus diesen Winkeln berechnet `updateCameraVectors()` den `Front`-Vektor mit **Trigonometrie**:

```cpp
front.x = cos(Yaw) * cos(Pitch);
front.y = sin(Pitch);
front.z = sin(Yaw) * cos(Pitch);
```

**Warum das funktioniert** — stell dir eine Einheitskugel vor:
- `Pitch = 0, Yaw = 0°`: `front = (cos0·cos0, sin0, sin0·cos0) = (1, 0, 0)` → schaut nach rechts
- `Pitch = 90°`: `front = (0, 1, 0)` → schaut nach oben
- `Yaw = -90°` (der Standard): `front = (0, 0, -1)` → schaut in -Z Richtung (OpenGL Standard)

Das ist **Kugelkoordinaten** — ein Punkt auf einer Einheitskugel durch zwei Winkel beschreiben. Du wirst gleich sehen dass die Orbit-Kamera **exakt denselben Trick** benutzt.

### Right und Up aus Front berechnen

```cpp
Right = normalize(cross(Front, WorldUp));
Up    = normalize(cross(Right, Front));
```

Das **Kreuzprodukt** zweier Vektoren ergibt einen dritten Vektor der senkrecht auf beiden steht:
- `cross(Front, WorldUp)` → zeigt nach rechts
- `cross(Right, Front)` → zeigt nach oben (kamera-relativ, nicht Weltkoordinaten)

Das gibt dir ein **orthogonales Koordinatensystem** für die Kamera.

### Problem der FPSCamera

Die FPSCamera ist perfekt für Egoperspektive. Aber für einen Editor (wie Blender) ist sie unbrauchbar:
- Du läufst weg wenn du die Maus bewegst
- Es gibt keinen fixen "Fokuspunkt" um den du kreist
- Zoomen ist nur "nach vorne laufen"

---

## Teil 2: Die Blender-Kamera — ein anderes Denkmodell

### Idee: "Was schaue ich an?" statt "Wo bin ich?"

Die FPSCamera denkt: *"Ich stehe hier und schaue in diese Richtung."*

Die Orbit-Kamera denkt: *"Ich schaue auf diesen Punkt, aus dieser Entfernung, aus diesem Winkel."*

```
        Kamera
          *
         /|
        / |  ← distance (Abstand)
       /  |
      /   |
     /    ↓
    ------*------ 
         target   ← fixer Fokuspunkt
```

Die Kamera-Position wird aus drei Werten berechnet:
- `target`: der Punkt der angeschaut wird
- `azimuth`: horizontaler Winkel (links/rechts, wie Yaw)
- `elevation`: vertikaler Winkel (hoch/runter, wie Pitch)
- `distance`: Abstand vom Fokuspunkt

Das sind **Kugelkoordinaten** — genau wie `Front` in der FPSCamera, nur diesmal für die **Position** der Kamera relativ zum Target.

### Kugelkoordinaten → Kameraposition

```
position = target + offset

offset.x = distance * cos(elevation) * sin(azimuth)
offset.y = distance * sin(elevation)
offset.z = distance * cos(elevation) * cos(azimuth)
```

Und dann einfach:
```cpp
glm::lookAt(position, target, worldUp)
```

Das ist alles. Die gesamte Orbit-Kamera reduziert sich auf diese Formel.

### Blender-Controls

| Aktion | Input |
|---|---|
| Orbit (drehen) | Mittlere Maustaste gedrückt + Maus bewegen |
| Pan (verschieben) | Shift + Mittlere Maustaste + Maus bewegen |
| Zoom | Scrollrad |
| Ansicht zurücksetzen | wird automatisch per Funktion gemacht |

---

## Teil 3: Die OrbitCamera Klasse

Erstelle eine neue Datei: `src/Headers/Core/Camera/OrbitCamera.h`

```cpp
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

class OrbitCamera {
public:
    // Der Punkt den die Kamera anschaut
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);

    // Kugelkoordinaten
    float azimuth   = 0.0f;     // horizontaler Winkel in Grad
    float elevation = 30.0f;    // vertikaler Winkel in Grad (30° = leicht von oben)
    float distance  = 10.0f;    // Abstand vom Target

    // Einstellungen
    float orbitSensitivity = 0.4f;
    float panSensitivity   = 0.01f;
    float zoomSensitivity  = 0.5f;
    float minDistance      = 0.5f;
    float maxDistance      = 500.0f;

    // Gibt die View Matrix zurück (für deinen Shader)
    glm::mat4 GetViewMatrix() const {
        glm::vec3 pos = getPosition();
        return glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Kameraposition im Weltkoordinaten (nützlich für Lighting-Shader)
    glm::vec3 getPosition() const {
        // Grad → Radiant für die trig. Funktionen
        float azRad  = glm::radians(azimuth);
        float elRad  = glm::radians(elevation);

        // Kugelkoordinaten → kartesische Koordinaten
        glm::vec3 offset;
        offset.x = distance * std::cos(elRad) * std::sin(azRad);
        offset.y = distance * std::sin(elRad);
        offset.z = distance * std::cos(elRad) * std::cos(azRad);

        return target + offset;
    }

    // ── Orbit (drehen) ────────────────────────────────────────────
    // xOffset: Mausbewegung links/rechts
    // yOffset: Mausbewegung hoch/runter
    void orbit(float xOffset, float yOffset) {
        azimuth   -= xOffset * orbitSensitivity;  // Links = negative X → Azimuth sinkt
        elevation += yOffset * orbitSensitivity;  // Hoch  = positive Y → Elevation steigt

        // Elevation begrenzen: nicht über 89° (Gimbal Lock vermeiden)
        // Nicht unter -89° (kein Flip)
        if (elevation >  89.0f) elevation =  89.0f;
        if (elevation < -89.0f) elevation = -89.0f;
    }

    // ── Pan (verschieben) ─────────────────────────────────────────
    // Verschiebt das Target (und damit die gesamte Kamera) in der
    // Kamera-Ebene — so fühlt es sich an als würde das Bild "geschoben"
    void pan(float xOffset, float yOffset) {
        // Wir brauchen die Kamera-Achsen um in der richtigen Richtung zu schieben
        glm::vec3 pos     = getPosition();
        glm::vec3 forward = glm::normalize(target - pos);
        glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up      = glm::normalize(glm::cross(right, forward));

        // Skalierung mit Distanz: weit entfernt → größere Pan-Bewegung (natürlicheres Gefühl)
        float scale = distance * panSensitivity;

        target -= right * (xOffset * scale);
        target += up    * (yOffset * scale);
    }

    // ── Zoom ──────────────────────────────────────────────────────
    // yOffset: positiv = rein, negativ = raus
    void zoom(float yOffset) {
        distance -= yOffset * zoomSensitivity * (distance * 0.1f);
        // Distanz begrenzen
        if (distance < minDistance) distance = minDistance;
        if (distance > maxDistance) distance = maxDistance;
    }

    // ── Preset-Ansichten (wie Blender Numpad) ─────────────────────
    void setFrontView()  { azimuth =   0.0f; elevation = 0.0f; }
    void setBackView()   { azimuth = 180.0f; elevation = 0.0f; }
    void setRightView()  { azimuth =  90.0f; elevation = 0.0f; }
    void setLeftView()   { azimuth = 270.0f; elevation = 0.0f; }
    void setTopView()    { azimuth =   0.0f; elevation = 89.0f; }
    void setBottomView() { azimuth =   0.0f; elevation = -89.0f; }

    void focusOn(const glm::vec3& point, float newDistance = -1.0f) {
        target = point;
        if (newDistance > 0.0f) distance = newDistance;
    }
};
```

---

## Teil 4: Input-Handling in GLFW

Die Blender-Kamera braucht Zugriff auf:
- Mittlere Maustaste (gedrückt halten)
- Shift-Taste (für Pan)
- Scrollrad
- Maus-Delta (wie weit sie sich bewegt hat)

### Zustand tracken

```cpp
// In der Scene als Member-Variablen:
bool  isOrbiting    = false;  // MMB gehalten?
bool  isPanning     = false;  // Shift + MMB gehalten?
float lastMouseX    = 0.0f;
float lastMouseY    = 0.0f;
bool  firstMouse    = true;
```

### Mouse Button Callback

GLFW hat einen separaten Callback für Maustasten. In deiner Scene überschreibe:

```cpp
// Diese Methode gibt es noch nicht in Scene.h — du musst sie hinzufügen!
// Oder du checkst in HandleMouseInput ob MMB gedrückt ist.
```

Da deine `Scene`-Basisklasse noch keinen Mouse-Button-Callback hat, gibt es zwei Möglichkeiten:

**Option A**: Den Status jedes Frame in `HandleInput(GLFWwindow*)` abfragen:
```cpp
void HandleInput(GLFWwindow* window) override {
    bool mmb   = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
              || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    isOrbiting = mmb && !shift;
    isPanning  = mmb &&  shift;
}
```

**Option B**: Scene.h um einen Mouse-Button-Callback erweitern (sauberer):

```cpp
// In Scene.h hinzufügen:
virtual void HandleMouseButton(GLFWwindow* window, int button, int action, int mods) {}
```

Für dieses Tutorial nutze ich Option A — keine Änderungen an der Engine nötig.

### Maus-Delta in HandleMouseInput

```cpp
void HandleMouseInput(GLFWwindow* window, double xpos, double ypos) override {
    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);

    if (firstMouse) {
        lastMouseX = x;
        lastMouseY = y;
        firstMouse = false;
        return;  // Erstes Frame: kein Delta berechnen
    }

    float dx = x - lastMouseX;
    float dy = lastMouseY - y;  // Invertiert: Maus hoch = positives Y
    lastMouseX = x;
    lastMouseY = y;

    if (isOrbiting) camera.orbit(dx, dy);
    if (isPanning)  camera.pan(dx, dy);
}
```

### Scroll für Zoom

Das Scrollrad kommt über den GLFW Scroll-Callback. In deiner `Application.h` ist dafür vermutlich noch kein Routing. Du kannst es direkt in der `HandleInput` Schleife simulieren:

```cpp
void HandleInput(GLFWwindow* window) override {
    // ... MMB check oben ...

    // Scroll via Tasten simulieren (falls kein Scroll-Callback vorhanden)
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)  // + Taste
        camera.zoom(1.0f * deltaTime * 10.0f);
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        camera.zoom(-1.0f * deltaTime * 10.0f);
}
```

Oder du fügst einen echten Scroll-Callback in `Application.h` hinzu (siehe Abschnitt unten).

---

## Teil 5: Scroll-Callback in der Engine einbauen

Um echtes Scrollrad-Zooming zu haben, muss `Application.h` den Scroll-Callback weiterleiten.

### Schritt 1: `Scene.h` erweitern

In `Scene.h` eine neue virtuelle Methode hinzufügen:

```cpp
virtual void HandleScrollInput(GLFWwindow* window, double xoffset, double yoffset) {}
```

### Schritt 2: Callback in `Application.h` registrieren

In der `Application`-Klasse, wo du `glfwSetKeyCallback` und `glfwSetCursorPosCallback` registrierst, füge hinzu:

```cpp
glfwSetScrollCallback(window, [](GLFWwindow* win, double xoff, double yoff) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(win));
    if (app->currentScene)
        app->currentScene->HandleScrollInput(win, xoff, yoff);
});
```

### Schritt 3: In der Scene nutzen

```cpp
void HandleScrollInput(GLFWwindow* window, double xoffset, double yoffset) override {
    camera.zoom(static_cast<float>(yoffset));
}
```

---

## Teil 6: Vollständige Scene-Implementierung

Erstelle: `src/Main/Scenes/OrbitCameraScene/OrbitCameraScene.h`

```cpp
#pragma once
#include "../../../Headers/Core/Common/Common.h"
#include "Headers/Core/Camera/OrbitCamera.h"
#include "Headers/Core/TextureManager/TextureManager.h"
#include <GL/gl.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vector>

class OrbitCameraScene : public Scene {
private:
    ShaderProgram* shaderPtr  = nullptr;
    VertexArray*   cubeVAO    = nullptr;
    VertexBuffer*  cubeVBO    = nullptr;
    VertexArray*   gridVAO    = nullptr;
    VertexBuffer*  gridVBO    = nullptr;

    unsigned int texture;

    OrbitCamera camera;

    float deltaTime   = 0.0f;
    float lastMouseX  = 0.0f;
    float lastMouseY  = 0.0f;
    bool  firstMouse  = true;
    float aspectRatio = 1280.0f / 800.0f;
    int   gridVertexCount = 0;

    // Input-Zustand
    bool isOrbiting = false;
    bool isPanning  = false;

public:
    OrbitCameraScene(const std::string& name) : Scene(name) {
        // Startkonfiguration: leicht von oben, schaut auf den Ursprung
        camera.target    = glm::vec3(0.0f, 0.0f, 0.0f);
        camera.azimuth   = 45.0f;
        camera.elevation = 30.0f;
        camera.distance  = 15.0f;
    }

    void InitScene(GLFWwindow* window) override {
        shaderPtr = new ShaderProgram(
            programPath("Main/Scenes/OrbitCameraScene/Shaders/shader.glsl"));

        setupCubeGeometry();
        setupGrid(20, 1.0f);   // 20x20 Grid mit 1 Einheit Abstand

        texture = TextureManager::Get().LoadTexture(
            programPath("Main/Scenes/CatCubes3DScene/Assets/soka_blue_cutie.png"));

        shaderPtr->Bind();
        shaderPtr->setUniform1i("texture0", 0);

        glEnable(GL_DEPTH_TEST);

        // Cursor sichtbar lassen — wir brauchen die Maus für die Kamera
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void Update(float dt) override {
        deltaTime = dt;
    }

    void HandleInput(GLFWwindow* window) override {
        // Mittlere Maustaste Status
        bool mmb   = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
        bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS
                  || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

        isOrbiting = mmb && !shift;
        isPanning  = mmb &&  shift;

        // Scroll-Zoom via Tasten (als Fallback)
        if (glfwGetKey(window, GLFW_KEY_KP_ADD)      == GLFW_PRESS) camera.zoom( 5.0f * deltaTime);
        if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) camera.zoom(-5.0f * deltaTime);

        // Preset-Ansichten (Numpad wie Blender)
        // Hinweis: Diese sind als Key-Callback besser, aber als Demo reicht das
    }

    void HandleInput(GLFWwindow* window, int key, int scancode, int action, int mods) override {
        if (action != GLFW_PRESS) return;

        // Blender Numpad-Ansichten
        if (key == GLFW_KEY_KP_1) camera.setFrontView();
        if (key == GLFW_KEY_KP_3) camera.setRightView();
        if (key == GLFW_KEY_KP_7) camera.setTopView();
        if (key == GLFW_KEY_KP_9) {  // "gegenüber" der aktuellen Ansicht
            camera.azimuth   = camera.azimuth + 180.0f;
            camera.elevation = -camera.elevation;
        }

        // R: Ansicht zurücksetzen
        if (key == GLFW_KEY_R) {
            camera.target    = glm::vec3(0.0f);
            camera.azimuth   = 45.0f;
            camera.elevation = 30.0f;
            camera.distance  = 15.0f;
        }
    }

    void HandleMouseInput(GLFWwindow* window, double xpos, double ypos) override {
        float x = static_cast<float>(xpos);
        float y = static_cast<float>(ypos);

        if (firstMouse) {
            lastMouseX = x;
            lastMouseY = y;
            firstMouse = false;
            return;
        }

        float dx =  x - lastMouseX;           // Maus nach rechts  = positiv
        float dy =  lastMouseY - y;           // Maus nach oben    = positiv (invertiert)
        lastMouseX = x;
        lastMouseY = y;

        if (isOrbiting) camera.orbit(dx, dy);
        if (isPanning)  camera.pan(dx, dy);
    }

    // Scroll-Callback (funktioniert wenn Application.h ihn weiterleitet)
    void HandleScrollInput(GLFWwindow* window, double xoffset, double yoffset) override {
        camera.zoom(static_cast<float>(yoffset));
    }

    void OnResize(float ar) override { aspectRatio = ar; }

    void Render() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

        shaderPtr->Bind();
        shaderPtr->setUniformMatrix4fv("view", view);
        shaderPtr->setUniformMatrix4fv("proj", proj);

        bindTexture(texture, 0);

        // Mehrere Würfel rendern
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f),
                    glm::vec3(x * 2.0f, 0.5f, z * 2.0f));
                shaderPtr->setUniformMatrix4fv("model", model);
                cubeVAO->Bind();
                glDrawArrays(GL_TRIANGLES, 0, 36);
                cubeVAO->UnBind();
            }
        }

        // Grid (Boden-Raster)
        glm::mat4 model = glm::mat4(1.0f);
        shaderPtr->setUniformMatrix4fv("model", model);
        gridVAO->Bind();
        glDrawArrays(GL_LINES, 0, gridVertexCount);
        gridVAO->UnBind();
    }

    void ImGuiLayer() override {
        ImGui::Begin("Orbit Camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        glm::vec3 pos = camera.getPosition();
        ImGui::Text("Kamera Position:  %.2f  %.2f  %.2f", pos.x, pos.y, pos.z);
        ImGui::Text("Target:           %.2f  %.2f  %.2f", camera.target.x, camera.target.y, camera.target.z);
        ImGui::Text("Azimuth:  %.1f°", camera.azimuth);
        ImGui::Text("Elevation: %.1f°", camera.elevation);
        ImGui::Text("Distance:  %.2f", camera.distance);

        ImGui::Separator();

        ImGui::SliderFloat("Orbit Sensitivity", &camera.orbitSensitivity, 0.1f, 2.0f);
        ImGui::SliderFloat("Pan Sensitivity",   &camera.panSensitivity,   0.001f, 0.05f);
        ImGui::SliderFloat("Zoom Sensitivity",  &camera.zoomSensitivity,  0.1f, 2.0f);

        ImGui::Separator();

        if (ImGui::Button("Front (Num 1)"))  camera.setFrontView();
        ImGui::SameLine();
        if (ImGui::Button("Right (Num 3)"))  camera.setRightView();
        ImGui::SameLine();
        if (ImGui::Button("Top (Num 7)"))    camera.setTopView();

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1,1,0,1), "MMB:       Orbit");
        ImGui::TextColored(ImVec4(1,1,0,1), "Shift+MMB: Pan");
        ImGui::TextColored(ImVec4(1,1,0,1), "Scroll:    Zoom");
        ImGui::TextColored(ImVec4(1,1,0,1), "R:         Reset");
        ImGui::End();
    }

    ~OrbitCameraScene() {
        delete shaderPtr;
        delete cubeVAO; delete cubeVBO;
        delete gridVAO; delete gridVBO;
    }

private:
    void setupCubeGeometry() {
        cubeVAO = new VertexArray();
        cubeVBO = new VertexBuffer(cubeVerts.data(),
            static_cast<GLuint>(cubeVerts.size() * sizeof(float)));
        cubeVAO->Bind(); cubeVBO->Bind();
        VertexBufferLayout layout;
        layout.AddElement<float>(3);
        layout.AddElement<float>(2);
        cubeVAO->AddBuffer(*cubeVBO, layout);
        cubeVAO->UnBind(); cubeVBO->UnBind();
    }

    void setupGrid(int halfSize, float step) {
        // Erstellt ein Gitter aus Linien für den Boden
        std::vector<float> verts;
        float limit = halfSize * step;

        for (int i = -halfSize; i <= halfSize; i++) {
            float pos = i * step;
            // Linie parallel zur X-Achse
            verts.insert(verts.end(), { -limit, 0.0f, pos,   0.0f, 0.0f });
            verts.insert(verts.end(), {  limit, 0.0f, pos,   1.0f, 0.0f });
            // Linie parallel zur Z-Achse
            verts.insert(verts.end(), { pos, 0.0f, -limit,   0.0f, 0.0f });
            verts.insert(verts.end(), { pos, 0.0f,  limit,   0.0f, 1.0f });
        }

        gridVertexCount = static_cast<int>(verts.size() / 5);

        gridVAO = new VertexArray();
        gridVBO = new VertexBuffer(verts.data(),
            static_cast<GLuint>(verts.size() * sizeof(float)));
        gridVAO->Bind(); gridVBO->Bind();
        VertexBufferLayout layout;
        layout.AddElement<float>(3);
        layout.AddElement<float>(2);
        gridVAO->AddBuffer(*gridVBO, layout);
        gridVAO->UnBind(); gridVBO->UnBind();
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

## Teil 7: Shader

### `Shaders/shader.glsl`

Identisch mit dem Standard-Shader den du bereits in anderen Scenes nutzt:

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

---

## Teil 8: Scene in `ScenesCollection.h` registrieren

```cpp
#include "OrbitCameraScene/OrbitCameraScene.h"

// In der Registrierungsfunktion:
manager.RegisterScene(std::make_unique<OrbitCameraScene>("Orbit Camera"));
```

---

## Vergleich: FPSCamera vs OrbitCamera

| | FPSCamera | OrbitCamera |
|---|---|---|
| **Denkmodell** | "Ich stehe hier, schaue dahin" | "Ich schaue das an, aus diesem Abstand" |
| **Gespeichert** | Position + Yaw/Pitch | Target + Azimuth/Elevation/Distance |
| **Bewegung** | WASD = Position ändern | MMB drag = Winkel ändern |
| **View Matrix** | `lookAt(pos, pos+front, up)` | `lookAt(calcPos(), target, up)` |
| **Geeignet für** | Egoperspektive, FPS | Editor, 3D-Viewer, Terrain-Editor |
| **Kernformel** | Euler → Richtungsvektor | Kugelkoordinaten → Position |

Beide Kameras nutzen `glm::lookAt` — der Unterschied liegt nur darin, **welche Werte** du da reinschickst.

---

## Häufige Probleme

### Kamera flippt bei Elevation 90°

Das passiert wenn `elevation >= 90°`. Dann wird `cos(elevation) = 0`, und der `up`-Vektor und der `forward`-Vektor sind parallel → `lookAt` kann kein Koordinatensystem mehr berechnen.

**Fix**: `elevation` auf `[-89°, +89°]` begrenzen (ist bereits in `orbit()` eingebaut).

### Pan fühlt sich falsch an wenn Elevation hoch ist

Bei steiler Draufsicht (elevation ~89°) stimmt die Pan-Richtung nicht mehr. Das liegt daran, dass `cross(forward, worldUp)` degeneriert wenn `forward ≈ worldUp`.

**Fix für fortgeschrittene**: Beim Pan die Kamera-`right` und `up` Vektoren aus der View Matrix extrahieren statt neu zu berechnen:

```cpp
glm::mat4 view = GetViewMatrix();
// Spalten der View Matrix sind die Kamera-Achsen (invertiert)
glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
glm::vec3 up    = glm::vec3(view[0][1], view[1][1], view[2][1]);
target -= right * (xOffset * scale);
target += up    * (yOffset * scale);
```

### Scroll funktioniert nicht

Deine `Application.h` muss `glfwSetScrollCallback` registrieren und die aktive Scene aufrufen. Wenn du den Callback noch nicht eingebaut hast, funktioniert `HandleScrollInput` nicht — nutze die `+`/`-` Tasten als Fallback.
