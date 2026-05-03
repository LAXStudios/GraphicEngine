# Application-Architektur: 5 Verbesserungen erklärt

Dieses Tutorial geht durch fünf konkrete Schwachstellen in `Application.h` und erklärt jeweils: warum das aktuelle Muster problematisch ist, was die bessere Lösung ist, und wie sie aussieht.

---

## 1. Static-Callback-Boilerplate

### Das Problem

GLFW erwartet C-Funktionszeiger für seine Callbacks. C-Funktionszeiger können keine Member-Funktionen sein, weil Member-Funktionen immer einen impliziten `this`-Zeiger brauchen, den ein C-Funktionszeiger nicht kennt.

Die aktuelle Lösung in `Application.h` sieht so aus:

```cpp
static void keyCallbackStatic(GLFWwindow *window, int key, int scancode, int action, int mods) {
    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
        app->keyCallback(window, key, scancode, action, mods);
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // eigentliche Logik
}
```

Das Problem: Für jeden der 5 Callbacks (Key, MouseButton, CursorPos, Scroll, FramebufferSize) existiert dieses identische Static-Wrapper-Paar. Das ist 10 Methoden statt 5 — reines Boilerplate ohne eigenen Mehrwert.

### Die bessere Lösung: Template-Helper

Man kann einen einzigen generischen Wrapper schreiben, der für alle Callbacks funktioniert:

```cpp
// Einmalig definiert, außerhalb der Klasse
template<auto MemberFn, typename... Args>
static void glfwCallback(GLFWwindow *window, Args... args) {
    auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
        (app->*MemberFn)(window, args...);
}
```

Registrierung dann ohne jede Static-Methode:

```cpp
glfwSetKeyCallback(_window, glfwCallback<&Application::keyCallback>);
glfwSetCursorPosCallback(_window, glfwCallback<&Application::mouseCursorPosCallback>);
glfwSetScrollCallback(_window, glfwCallback<&Application::mouseScrollBack>);
// usw.
```

### Was hier passiert

`auto MemberFn` ist ein Non-Type Template Parameter — der Compiler kennt die konkrete Member-Funktion zur Compile-Zeit und generiert für jede Instanziierung eine eigene Static-Funktion. Zur Laufzeit ist das genauso schnell wie die manuellen Static-Wrapper, aber der Quellcode enthält kein Boilerplate mehr.

`Args...` ist ein Variadic Template — es passt sich automatisch an die Signatur des jeweiligen Callbacks an (ein Callback bekommt `double xpos, double ypos`, ein anderer `int key, int scancode, int action, int mods`).

### Warum `glfwSetWindowUserPointer` überhaupt nötig ist

GLFW-Callbacks sind reine C-Callbacks. Sie wissen nichts von C++-Objekten. Der einzige Weg, von einem solchen Callback auf das eigene `Application`-Objekt zu kommen, ist ein void-Pointer, den GLFW pro Fenster speichert:

```cpp
glfwSetWindowUserPointer(_window, this);  // Speichern
// ...
Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));  // Abrufen
```

Das ist ein klassisches C-API-Muster für "User Data" — nahezu jede C-Library mit Callbacks hat so etwas.

---

## 2. Raw Pointer vs. `std::unique_ptr`

### Das Problem

```cpp
ImGuiLayer *imGuiLayer;          // Declaration
imGuiLayer = new ImGuiLayer(...); // Init()
delete imGuiLayer;               // Destructor
```

Dieses Muster hat drei Schwachstellen:

**Schwachstelle 1 — Exception Safety:**
Wenn zwischen `new` und `delete` eine Exception geworfen wird (z.B. in `Init()`), läuft der Destruktor von `Application` trotzdem — aber `imGuiLayer` könnte ein ungültiger Pointer sein oder der `delete`-Aufruf nie erreicht werden.

**Schwachstelle 2 — Fehler durch frühen Return:**
In `Init()` gibt es mehrere `return`-Stellen bei Fehler. Wenn `new ImGuiLayer` bereits ausgeführt wurde und danach ein `return` kommt, wird `delete` nie aufgerufen.

**Schwachstelle 3 — Implizite Ownership:**
Wer sieht, dass `imGuiLayer` ein `ImGuiLayer*` ist, weiß nicht sofort: Wer besitzt diesen Pointer? Wer ist zuständig für `delete`?

### Die bessere Lösung: `std::unique_ptr`

```cpp
std::unique_ptr<ImGuiLayer> imGuiLayer;

// Init:
imGuiLayer = std::make_unique<ImGuiLayer>(_window, mainScale);

// Destruktor: nicht nötig — unique_ptr räumt automatisch auf
```

`unique_ptr` modelliert **exklusive Eigentumsrechte** (Ownership). Der Pointer kann nicht kopiert werden, nur verschoben (`std::move`). Wenn das `Application`-Objekt zerstört wird — egal ob normal oder durch Exception — wird `imGuiLayer` automatisch korrekt aufgeräumt.

### Der Unterschied zu `shared_ptr`

| | `unique_ptr` | `shared_ptr` |
|---|---|---|
| Ownership | exklusiv (1 Besitzer) | geteilt (N Besitzer) |
| Overhead | kein Laufzeit-Overhead | Referenzzähler (atomic) |
| Kopierbar | nein | ja |
| Wann benutzen | default, wenn ein Objekt einen klaren Besitzer hat | wenn mehrere Stellen denselben Pointer brauchen |

**Regel:** Default ist `unique_ptr`. `shared_ptr` nur, wenn du tatsächlich geteilten Besitz brauchst.

---

## 3. Unbenutzte Variablen

### Das Problem

```cpp
const char *glslVersion = "#version 130";
// ... wird nie wieder verwendet
```

Diese Variable in `Init()` wird gesetzt aber nie benutzt. Ursprünglich sollte sie an ImGui übergeben werden:

```cpp
ImGui_ImplOpenGL3_Init(glslVersion);
```

Falls `ImGuiLayer` das intern übernimmt, ist die Variable in `Application` überflüssig. Unbenutzte Variablen sind problematisch weil:

1. Compiler-Warnings werden lauter (je nach Warning-Level `-Wunused-variable`)
2. Lesende denken, die Variable hat eine Funktion, die sie nicht hat
3. Es entsteht die Frage: "Wurde das vergessen? Kommt das noch?"

### Die bessere Lösung

Entweder nutzen oder löschen. Kein "vielleicht später"-Code im Hauptpfad.

---

## 4. Fehlerbehandlung in `Init()`

### Das Problem

```cpp
void Init() {
    if (!glfwInit()) {
        printf("Error: GLFW Init. %d", glfwGetError(NULL));
        return;  // <-- stiller Fehler
    }

    // ...

    _window = glfwCreateWindow(...);
    if (_window == nullptr) {
        printf("Error: Window is nullptr.");
        return;  // <-- stiller Fehler
    }

    // ... weitere Initialisierung mit _window
}

void Run() {
    // _window wird hier verwendet — aber was wenn Init() fehlschlug?
    while (!glfwWindowShouldClose(_window)) { ... }  // nullptr-Deref → Crash
}
```

`Init()` gibt `void` zurück. Der Aufrufer hat keine Möglichkeit zu wissen, ob die Initialisierung erfolgreich war. Bei einem Fehler läuft `Run()` trotzdem — mit einem `nullptr`-Fenster — und crasht.

### Lösung A — Return bool

```cpp
bool Init() {
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW init failed\n");
        return false;
    }
    // ...
    return true;
}

// main.cpp:
if (!app.Init()) return -1;
app.Run();
```

Einfach, explizit, der Aufrufer muss den Rückgabewert prüfen.

### Lösung B — Exception werfen

```cpp
void Init() {
    if (!glfwInit())
        throw std::runtime_error("GLFW init failed");
    // ...
}

// main.cpp:
try {
    app.Init();
    app.Run();
} catch (const std::runtime_error &e) {
    std::fprintf(stderr, "Fatal: %s\n", e.what());
    return -1;
}
```

Exceptions eignen sich gut für "das sollte nie passieren, und wenn doch, ist alles vorbei". Der Vorteil: Kein Fehler kann lautlos ignoriert werden — eine unbehandelte Exception terminiert das Programm.

### Was man nicht tun sollte

`assert(_window != nullptr)` ist kein Fehlerhandling — es crasht in Debug, tut in Release gar nichts. Und `return` ohne Signal nach außen ist noch schlimmer, weil der Fehler verschwindet.

---

## 5. Member-Variablen vs. lokale Variablen

### Das Problem

```cpp
class Application {
    int frameCount;     // nur in Run() benutzt
    double lastTime;    // nur in Run() benutzt
    double fps;         // nur in Run() benutzt
    // ...
};
```

Diese drei Variablen werden ausschließlich innerhalb der `Run()`-Schleife verwendet. Trotzdem leben sie als Member-Variablen für die gesamte Lebensdauer des `Application`-Objekts — also auch während `Init()`, im Destruktor, und in allen anderen Methoden, wo sie nichts zu suchen haben.

### Was Member-Variablen bedeuten

Eine Member-Variable ist ein Versprechen: "Dieser Zustand gehört zum Objekt und ist für andere Methoden relevant." Bei `frameCount`, `lastTime` und `fps` ist das eine Lüge — sie sind rein interner Zustand von `Run()`.

### Die bessere Lösung: lokale Variablen

```cpp
void Run() {
    int frameCount = 0;
    double lastTime = glfwGetTime();
    double fps = 0.0;

    // Rest der Schleife unverändert
}
```

Das hat mehrere Vorteile:

**Initialisierung ist explizit:**
Member-Variablen wie `int frameCount;` sind ohne expliziten Initializer undefiniert (in C++ sind nicht-statische Member ohne Initializer nicht null-initialisiert). Als lokale Variable kann man sofort den richtigen Startwert setzen.

**Scope dokumentiert Absicht:**
Wer `Run()` liest, sieht sofort: diese Variablen existieren nur hier. Kein Suchen ob `frameCount` irgendwo sonst geschrieben oder gelesen wird.

**Klassen-Interface wird schlanker:**
Je weniger Member-Variablen eine Klasse hat, desto einfacher ist es, ihren Zustand zu verstehen. "Was kann ein `Application`-Objekt sein?" sollte keine FPS-Counter enthalten.

### Faustregel

> Eine Variable sollte so lokal wie möglich leben. Nur wenn sie über mehrere Methoden oder Aufrufe hinweg persistent sein muss, gehört sie als Member in die Klasse.
