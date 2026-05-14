# Dynamische Terrain-Generierung

Dieses Tutorial erklärt, wie man in OpenGL/C++ ein prozedural generiertes Terrain erstellt — von einem leeren Grid bis zu einem beleuchteten, chunkbasierten Terrain mit Noise-Algorithmen. Es ist als Grundlage für größere Projekte wie Lego-Terrain-Generatoren gedacht.

---

## Inhaltsverzeichnis

1. [Das Grundprinzip: Was ist ein Terrain-Mesh?](#1-das-grundprinzip)
2. [Das Grid aufbauen: Vertices und Indices](#2-das-grid-aufbauen)
3. [Heightmaps: Höhe als Daten](#3-heightmaps)
4. [Normals berechnen: Für korrekte Beleuchtung](#4-normals-berechnen)
5. [Noise-Algorithmen: Prozedurales Gelände](#5-noise-algorithmen)
6. [Oktaven und fBm: Realistisch wirkendes Gelände](#6-oktaven-und-fbm)
7. [Der Terrain-Shader: Höhenbasierte Farbe](#7-der-terrain-shader)
8. [Chunk-basiertes Terrain: Unendliche Welten](#8-chunk-basiertes-terrain)
9. [Verbindung zu Lego/Voxel-Terrain](#9-verbindung-zu-lego-terrain)

---

## 1. Das Grundprinzip

Ein Terrain ist im Kern ein **Gitter aus Dreiecken** (Triangle Grid). Jeder Punkt dieses Gitters hat eine X- und Z-Position (horizontal) sowie eine Y-Position (Höhe).

```
Draufsicht (X/Z-Ebene):
+--+--+--+
|\ |\ |\ |
| \| \| \|
+--+--+--+
|\ |\ |\ |
| \| \| \|
+--+--+--+
```

Jedes Quadrat des Gitters besteht aus zwei Dreiecken. Ein 4x4-Gitter von Punkten ergibt also ein 3x3-Gitter von Quadraten, also 18 Dreiecke.

Die **Höhe** jedes Punktes wird durch eine Funktion bestimmt — entweder aus einer Bilddatei (Heightmap) oder durch eine mathematische Funktion (Noise). Das ist der einzige Unterschied zu einer flachen Ebene.

**Zusammengefasst:**
- Terrain = flaches Grid + Höhenfunktion auf der Y-Achse
- Die Höhenfunktion kann eine Textur, Noise, oder beides sein
- OpenGL sieht am Ende nur Vertices und Dreiecke — wie immer

---

## 2. Das Grid aufbauen

### 2.1 Vertices generieren

Ein Grid der Größe `W x H` (in Punkten) wird so aufgebaut:

```cpp
// width  = Anzahl Punkte in X-Richtung
// height = Anzahl Punkte in Z-Richtung
// spacing = Abstand zwischen zwei Punkten in Weltkoordinaten

struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

std::vector<TerrainVertex> generateGrid(int width, int height, float spacing) {
    std::vector<TerrainVertex> vertices;
    vertices.reserve(width * height);

    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            TerrainVertex v;

            // Position: zentriert um den Ursprung
            v.position.x = (x - width  * 0.5f) * spacing;
            v.position.y = 0.0f; // Höhe später befüllen
            v.position.z = (z - height * 0.5f) * spacing;

            // UV: 0..1 über das gesamte Grid
            v.uv.x = (float)x / (width  - 1);
            v.uv.y = (float)z / (height - 1);

            v.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Platzhalter

            vertices.push_back(v);
        }
    }
    return vertices;
}
```

**Warum zentriert um den Ursprung?**
Wenn das Grid bei (0,0) anfängt, liegt es komplett in der positiven X/Z-Halbebene. Das macht Kamerasteuerung und Berechnungen umständlicher. Zentriert ist einfacher.

**Der Index eines Punktes bei (x, z):**
```cpp
int index = z * width + x;
```
Das ist das Standard-Schema für 2D-Arrays, die als 1D-Array gespeichert sind (Row-Major).

### 2.2 Index Buffer (EBO) generieren

Der Index Buffer sagt OpenGL, in welcher Reihenfolge die Vertices zu Dreiecken verbunden werden. Für jedes Quadrat des Gitters werden zwei Dreiecke definiert:

```
Quad bei (x, z):
  A---B
  |\ |
  | \|
  C---D

Dreieck 1: A, C, B  (oben-links, unten-links, oben-rechts)
Dreieck 2: B, C, D  (oben-rechts, unten-links, unten-rechts)
```

```cpp
std::vector<unsigned int> generateIndices(int width, int height) {
    std::vector<unsigned int> indices;
    // (width-1) * (height-1) Quads, je 2 Dreiecke, je 3 Indices
    indices.reserve((width - 1) * (height - 1) * 6);

    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            unsigned int A = (z    ) * width + (x    );
            unsigned int B = (z    ) * width + (x + 1);
            unsigned int C = (z + 1) * width + (x    );
            unsigned int D = (z + 1) * width + (x + 1);

            // Dreieck 1
            indices.push_back(A);
            indices.push_back(C);
            indices.push_back(B);

            // Dreieck 2
            indices.push_back(B);
            indices.push_back(C);
            indices.push_back(D);
        }
    }
    return indices;
}
```

**Warum Index Buffer statt direkter Vertices?**
Ohne Index Buffer müsste jeder Vertex mehrfach im VBO stehen (ein Punkt gehört zu 6 Dreiecken). Mit Index Buffer steht jeder Punkt genau einmal im Speicher — der EBO referenziert ihn mehrfach. Bei einem 256x256-Grid spart das ~85% Speicher.

### 2.3 GPU-Upload

```cpp
unsigned int VAO, VBO, EBO;

glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
glGenBuffers(1, &EBO);

glBindVertexArray(VAO);

glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER,
    vertices.size() * sizeof(TerrainVertex),
    vertices.data(),
    GL_DYNAMIC_DRAW); // DYNAMIC weil Höhendaten sich ändern können

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER,
    indices.size() * sizeof(unsigned int),
    indices.data(),
    GL_STATIC_DRAW); // Index-Struktur ändert sich nie

// Position: layout(location = 0)
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
    sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, position));

// Normal: layout(location = 1)
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
    sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, normal));

// UV: layout(location = 2)
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
    sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, uv));

glBindVertexArray(0);
```

**`GL_DYNAMIC_DRAW` vs `GL_STATIC_DRAW`:**
- `GL_STATIC_DRAW` → Daten werden einmal hochgeladen und nie geändert (Index Buffer)
- `GL_DYNAMIC_DRAW` → Daten werden regelmäßig per `glBufferSubData` aktualisiert (Vertex Buffer, wenn Höhe sich ändert)

**`offsetof(Struct, Member)`:**
Das Makro berechnet den Byte-Offset eines Members innerhalb einer Struct. Unverzichtbar wenn man Structs direkt in VBOs verwendet — OpenGL muss wissen, wo innerhalb eines Vertex-Blocks die Position, Normal usw. anfangen.

---

## 3. Heightmaps

### 3.1 Was ist eine Heightmap?

Eine Heightmap ist ein Graustufenbild, bei dem die Helligkeit eines Pixels die Höhe an dieser Stelle codiert. Schwarz = niedrig, Weiß = hoch.

```
Pixel (128, 128):  Höhe = 0.5 * maxHeight = 5.0f
Pixel (255, 255):  Höhe = 1.0 * maxHeight = 10.0f
Pixel (0,   0  ):  Höhe = 0.0 * maxHeight = 0.0f
```

Heightmaps sind einfach zu erstellen (z.B. in GAEA, WorldMachine, oder auch Photoshop) und einfach zu lesen.

### 3.2 Höhe aus einem Array lesen

Wenn die Höhendaten als `float`-Array vorliegen (Werte 0.0..1.0):

```cpp
// heights: 1D-Array, Größe width*height, Werte 0..1
float getHeight(const std::vector<float>& heights, int x, int z, int width) {
    // Clamp damit man nicht außerhalb des Arrays liest
    x = glm::clamp(x, 0, width - 1);
    // (height der Heightmap, nicht Terrainbreite)
    int maxZ = (int)(heights.size() / width) - 1;
    z = glm::clamp(z, 0, maxZ);
    return heights[z * width + x];
}

// Anwenden auf die Vertex-Positionen:
float maxTerrainHeight = 20.0f;

for (int z = 0; z < gridHeight; z++) {
    for (int x = 0; x < gridWidth; x++) {
        int i = z * gridWidth + x;
        float h = getHeight(heights, x, z, gridWidth);
        vertices[i].position.y = h * maxTerrainHeight;
    }
}
```

### 3.3 Daten aktualisieren ohne neuen Upload

Wenn sich die Höhendaten ändern (z.B. weil ein neuer Noise-Seed berechnet wurde), müssen nicht alle Daten neu hochgeladen werden. `glBufferSubData` aktualisiert nur einen Teilbereich:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferSubData(GL_ARRAY_BUFFER,
    0,                                      // Offset: ab Byte 0
    vertices.size() * sizeof(TerrainVertex), // Größe
    vertices.data());                        // neue Daten
```

Das ist deutlich schneller als `glBufferData`, weil kein neuer GPU-Speicher alloziert wird.

---

## 4. Normals berechnen

### 4.1 Warum braucht man Normals?

Normals werden vom Fragmentshader für Beleuchtung verwendet (Diffuse, Specular, etc.). Eine Normal ist ein Einheitsvektor, der senkrecht auf der Oberfläche an diesem Punkt steht.

Bei einem flachen Mesh ist jede Normal `(0, 1, 0)`. Bei einem Terrain zeigt jede Normal in die Richtung, in die die Oberfläche an diesem Punkt "schaut".

Falsche oder fehlende Normals → Beleuchtung wirkt vollkommen falsch.

### 4.2 Cross Product zweier Kanten

Für jeden Vertex berechnet man Normals über die umliegenden Vertices:

```
     N (z-1)
     |
W ---P--- E    P = aktueller Punkt
     |         N/S/E/W = Nachbarn in Grid-Koordinaten
     S (z+1)
```

```cpp
// Zwei Kanten vom aktuellen Punkt zu Nachbarn bilden
// Cross Product der Kanten = Normalvektor
glm::vec3 computeNormal(
    const std::vector<TerrainVertex>& verts,
    int x, int z, int width, int height)
{
    // Nachbar-Höhen holen (mit Clamp an den Rändern)
    auto idx = [&](int px, int pz) {
        px = glm::clamp(px, 0, width  - 1);
        pz = glm::clamp(pz, 0, height - 1);
        return pz * width + px;
    };

    float hL = verts[idx(x - 1, z)].position.y; // links  (West)
    float hR = verts[idx(x + 1, z)].position.y; // rechts (East)
    float hD = verts[idx(x, z - 1)].position.y; // vorne  (North)
    float hU = verts[idx(x, z + 1)].position.y; // hinten (South)

    // Gradient-Methode: schnell und ausreichend genau
    glm::vec3 normal = glm::normalize(glm::vec3(
        hL - hR,   // X: Steigung in X-Richtung (invertiert)
        2.0f,      // Y: konstant (Skalierungsfaktor)
        hD - hU    // Z: Steigung in Z-Richtung (invertiert)
    ));

    return normal;
}
```

**Die Gradient-Methode** ist eine Vereinfachung: Man berechnet den Höhengradienten (Steigung) in X und Z, und konstruiert daraus direkt eine Normal. Der Y-Wert `2.0f` ist ein Faktor für die Glattheit — größer = flachere Normals, kleiner = stärker ausgeprägte Normals.

Diese Methode ist schneller als echtes Cross Product mit zwei Dreieckskanten und liefert für Terrain völlig ausreichende Ergebnisse.

### 4.3 Alle Normals nach dem Höhen-Update neu berechnen

```cpp
void recomputeNormals(std::vector<TerrainVertex>& verts,
                      int width, int height) {
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            verts[z * width + x].normal =
                computeNormal(verts, x, z, width, height);
        }
    }
}
```

Das wird nach jedem Höhen-Update aufgerufen, bevor `glBufferSubData` die Daten zur GPU schickt.

---

## 5. Noise-Algorithmen

Höhendaten brauchen eine Quelle. Die wichtigste Technik für prozedurales Terrain ist **Perlin Noise** (und der modernere **Simplex Noise**).

### 5.1 Was ist Noise?

Noise ist eine Funktion `f(x, z) → float (0..1)`, die:
- **Deterministisch** ist: selbe Eingabe → selber Wert, immer
- **Kontinuierlich** ist: benachbarte Punkte haben ähnliche Werte (keine abrupten Sprünge)
- **Zufällig aussieht** (ohne wirklich zufällig zu sein)

```
Zufallswerte (kein Noise):   Perlin Noise:
0.7  0.1  0.9  0.3           0.5  0.6  0.7  0.7
0.4  0.8  0.2  0.5           0.4  0.5  0.6  0.7
0.6  0.3  0.7  0.1           0.3  0.4  0.5  0.6
0.2  0.9  0.4  0.8           0.3  0.3  0.4  0.5
```

Zufallswerte ergeben "Salz und Pfeffer" — keine nutzbare Topographie. Noise ergibt weiche Hügel und Täler.

### 5.2 Perlin Noise: Das Prinzip

Perlin Noise (Ken Perlin, 1983) funktioniert in drei Schritten:

**Schritt 1: Zufällige Gradienten-Vektoren an Gitterpunkten**

Ein unsichtbares Gitter wird mit zufälligen Einheitsvektoren belegt:
```
(0,0)→(0.7, 0.7)   (1,0)→(-0.5, 0.9)
(0,1)→(0.3, -0.9)  (1,1)→(0.8, 0.6)
```

Diese Vektoren sind abhängig vom **Seed** (Zufallszahl für die Generierung).

**Schritt 2: Für einen Anfragepunkt (x, z) — Skalarprodukte berechnen**

Für den Punkt `P = (0.3, 0.7)` (innerhalb des Gitterquadrats `(0,0)-(1,1)`):
- Berechne Vektor von jedem Gittereckpunkt zu P: `d = P - Eckpunkt`
- Berechne Skalarprodukt von Gradient-Vektor und d-Vektor an jedem Eckpunkt

**Schritt 3: Interpolation mit Smooth-Step**

Die 4 Skalarprodukte werden mit einer glatten Kurve (keine lineare Interpolation!) interpoliert. Die Kurve ist `f(t) = 6t⁵ - 15t⁴ + 10t³` (Quintic Ease).

Lineare Interpolation würde an Gitterpunkten sichtbare Kanten erzeugen. Die Quintic-Funktion hat an 0 und 1 sowohl Steigung als auch Krümmung 0 — perfekte Übergänge.

### 5.3 Eine einfache Implementierung in C++

Eine vollständige, kompakte Perlin-Noise-Implementierung:

```cpp
#include <array>
#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>

class PerlinNoise {
public:
    explicit PerlinNoise(unsigned int seed = 0) {
        // Permutationstabelle: 0..255, doppelt für Wrap-Around
        std::iota(p.begin(), p.begin() + 256, 0);
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.begin() + 256, engine);
        for (int i = 0; i < 256; i++)
            p[256 + i] = p[i];
    }

    // Gibt Wert im Bereich [-1, 1] zurück
    float noise(float x, float z) const {
        // Gitterzelle bestimmen
        int X = (int)std::floor(x) & 255;
        int Z = (int)std::floor(z) & 255;

        // Position innerhalb der Zelle (0..1)
        x -= std::floor(x);
        z -= std::floor(z);

        // Smooth-Step (Quintic Ease)
        float u = fade(x);
        float v = fade(z);

        // Hash der 4 Eckpunkte
        int aa = p[p[X    ] + Z    ];
        int ab = p[p[X    ] + Z + 1];
        int ba = p[p[X + 1] + Z    ];
        int bb = p[p[X + 1] + Z + 1];

        // Interpolieren
        return lerp(v,
            lerp(u, grad(aa, x,       z    ),
                    grad(ba, x - 1.0f, z    )),
            lerp(u, grad(ab, x,       z - 1.0f),
                    grad(bb, x - 1.0f, z - 1.0f))
        );
    }

    // Gibt Wert im Bereich [0, 1] zurück (verschoben)
    float noise01(float x, float z) const {
        return (noise(x, z) + 1.0f) * 0.5f;
    }

private:
    std::array<int, 512> p;

    static float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static float lerp(float t, float a, float b) {
        return a + t * (b - a);
    }

    static float grad(int hash, float x, float z) {
        // 4 mögliche Gradienten-Richtungen
        switch (hash & 3) {
            case 0: return  x + z;
            case 1: return -x + z;
            case 2: return  x - z;
            case 3: return -x - z;
        }
        return 0.0f;
    }
};
```

**Verwendung:**
```cpp
PerlinNoise noise(42); // Seed 42

for (int z = 0; z < gridHeight; z++) {
    for (int x = 0; x < gridWidth; x++) {
        // frequency: wie "weit auseinander" die Noisewellen sind
        float frequency = 0.05f;
        float h = noise.noise01(x * frequency, z * frequency);
        vertices[z * gridWidth + x].position.y = h * maxTerrainHeight;
    }
}
```

**Der `frequency`-Parameter:**
- `frequency = 0.01f` → sehr weite, sanfte Hügel (große Wellenlänge)
- `frequency = 0.1f` → engere, häufigere Hügel
- `frequency = 0.5f` → sehr kleinteiliges, zerklüftetes Terrain

---

## 6. Oktaven und fBm

Ein einzelner Noise-Pass sieht zu glatt aus — reales Gelände hat sowohl große Bergzüge als auch kleine Felsen und Unebenheiten. Die Lösung: **Fractional Brownian Motion (fBm)**.

### 6.1 Das Prinzip

fBm addiert mehrere Noise-Schichten (Oktaven) übereinander, jede mit höherer Frequenz und geringerer Amplitude:

```
Oktave 1: amplitude=1.0,  frequency=0.01  → Bergzüge
Oktave 2: amplitude=0.5,  frequency=0.02  → Hügel
Oktave 3: amplitude=0.25, frequency=0.04  → Felsen
Oktave 4: amplitude=0.125,frequency=0.08  → kleine Unebenheiten

Summe = realistische Topographie
```

Jede Oktave wird mit **Lacunarity** (typisch 2.0) frequenzmäßig skaliert und mit **Persistence/Gain** (typisch 0.5) amplitudenmäßig gedämpft.

```
Lacunarity = 2.0:   jede Oktave hat doppelte Frequenz
Persistence = 0.5:  jede Oktave hat halbe Amplitude
```

### 6.2 Implementierung

```cpp
float fbm(const PerlinNoise& noise, float x, float z,
          int octaves, float frequency, float persistence, float lacunarity)
{
    float value     = 0.0f;
    float amplitude = 1.0f;
    float maxValue  = 0.0f; // Zum Normalisieren auf 0..1

    for (int i = 0; i < octaves; i++) {
        value    += noise.noise01(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;

        amplitude *= persistence; // Amplitude pro Oktave halbieren
        frequency *= lacunarity;  // Frequenz pro Oktave verdoppeln
    }

    return value / maxValue; // Normalisiert auf 0..1
}
```

**Verwendung:**
```cpp
PerlinNoise noise(seed);

for (int z = 0; z < gridHeight; z++) {
    for (int x = 0; x < gridWidth; x++) {
        float h = fbm(noise,
            (float)x, (float)z,
            6,      // Anzahl Oktaven
            0.005f, // Basis-Frequenz (bestimmt Gesamtgröße der Berge)
            0.5f,   // Persistence (0..1, typisch 0.5)
            2.0f    // Lacunarity (typisch 2.0)
        );
        vertices[z * gridWidth + x].position.y = h * maxTerrainHeight;
    }
}
```

### 6.3 Parameter-Intuition

| Parameter | Niedrig | Hoch |
|---|---|---|
| `octaves` | Glatt, einfache Formen | Detailliert, komplex (teurer) |
| `frequency` | Weite Landschaft | Enge, kleinteilige Topographie |
| `persistence` | Dominanz der Grobstrukturen | Viele Details, rauer |
| `lacunarity` | Oktaven ähneln sich | Oktaven unterscheiden sich stark |
| `maxTerrainHeight` | Flaches Land | Steile Berge |

### 6.4 Höhenkurven (Height Remapping)

Rohes fBm-Terrain sieht oft zu "rund" aus. Durch eine nicht-lineare Transformation der Höhe kann man Charakteristika erzeugen:

```cpp
// Beispiel: Flache Täler, steile Berge (wie echte Erosion)
float remapHeight(float h) {
    // h liegt in [0, 1]

    // Potenz > 1 → Werte nahe 0 bleiben klein, Werte nahe 1 wachsen
    // Ergibt flache Täler und spitze Berge
    return std::pow(h, 2.5f);
}

// Oder: Meeresspiegel einführen
float remapWithSea(float h, float seaLevel = 0.4f) {
    if (h < seaLevel) return seaLevel; // alles darunter wird abgeschnitten
    return h;
}
```

---

## 7. Der Terrain-Shader

### 7.1 Vertex Shader

```glsl
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal   = mat3(transpose(inverse(model))) * aNormal;
    vUV       = aUV;

    gl_Position = proj * view * worldPos;
}
```

**`transpose(inverse(model))`:**
Normals dürfen nicht einfach mit der Model-Matrix transformiert werden, wenn die Matrix nicht-uniform skaliert (also unterschiedlich in X/Y/Z). Die Normal-Matrix ist die transponierte Inverse der Model-Matrix. Bei Terrain, das nicht skaliert wird, wäre `mat3(model)` ausreichend — aber die korrekte Variante ist robuster.

### 7.2 Fragment Shader mit Höhenfarbe

```glsl
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

uniform float maxHeight;      // Maximale Terrainhöhe
uniform vec3  lightDir;       // Richtung ZUM Licht (normalisiert)
uniform vec3  lightColor;

void main() {
    // Normalisierte Höhe: 0 (tief) bis 1 (Gipfel)
    float normalizedHeight = vWorldPos.y / maxHeight;

    // Höhenbasierte Farbe (Biom-ähnlich)
    vec3 color;
    if (normalizedHeight < 0.15) {
        // Tiefstes: Sand/Wasser
        color = mix(vec3(0.2, 0.3, 0.6), vec3(0.76, 0.70, 0.50),
                    normalizedHeight / 0.15);
    } else if (normalizedHeight < 0.45) {
        // Gras
        color = mix(vec3(0.3, 0.55, 0.2), vec3(0.25, 0.45, 0.15),
                    (normalizedHeight - 0.15) / 0.30);
    } else if (normalizedHeight < 0.75) {
        // Fels
        color = mix(vec3(0.5, 0.45, 0.4), vec3(0.4, 0.35, 0.3),
                    (normalizedHeight - 0.45) / 0.30);
    } else {
        // Schnee
        color = mix(vec3(0.85, 0.85, 0.9), vec3(1.0, 1.0, 1.0),
                    (normalizedHeight - 0.75) / 0.25);
    }

    // Diffuse Beleuchtung (Lambertian)
    vec3 N = normalize(vNormal);
    float diff = max(dot(N, normalize(lightDir)), 0.0);

    vec3 ambient  = 0.15 * lightColor * color;
    vec3 diffuse  = diff * lightColor * color;

    FragColor = vec4(ambient + diffuse, 1.0);
}
```

**`mix(a, b, t)`:**
Lineare Interpolation zwischen `a` und `b` mit Faktor `t` (0=a, 1=b). Wird hier genutzt um weiche Übergänge zwischen Biom-Farben zu erzeugen statt harter Grenzen.

---

## 8. Chunk-basiertes Terrain

Ein einzelnes Grid von 1000x1000 Punkten erzeugt 1 Million Vertices. Das ist zwar machbar, aber starr: Man kann den sichtbaren Bereich nicht einfach verschieben ohne alles neu zu generieren.

Die Lösung: Das Terrain wird in **Chunks** unterteilt. Jeder Chunk ist ein kleines Grid (z.B. 64x64 Punkte). Chunks werden bei Bedarf geladen und entladen.

### 8.1 Chunk-Koordinaten vs. Weltkoordinaten

```cpp
// Chunk-Größe in Welteinheiten
const float CHUNK_SIZE = 64.0f; // 64 Meter pro Chunk
const int   CHUNK_RES  = 64;    // 64x64 Punkte pro Chunk

// Welt → Chunk-Koordinaten
glm::ivec2 worldToChunk(glm::vec3 worldPos) {
    return glm::ivec2(
        (int)std::floor(worldPos.x / CHUNK_SIZE),
        (int)std::floor(worldPos.z / CHUNK_SIZE)
    );
}

// Chunk-Koordinaten → Welt-Offset (untere linke Ecke des Chunks)
glm::vec2 chunkToWorld(glm::ivec2 chunkCoord) {
    return glm::vec2(
        chunkCoord.x * CHUNK_SIZE,
        chunkCoord.y * CHUNK_SIZE
    );
}
```

### 8.2 Chunk-Klasse

```cpp
struct TerrainChunk {
    glm::ivec2 coord;           // Chunk-Koordinate im Grid
    unsigned int VAO, VBO, EBO;
    int indexCount;
    bool isLoaded = false;

    void generate(const PerlinNoise& noise, glm::ivec2 chunkCoord);
    void draw();
    void unload();
};

void TerrainChunk::generate(const PerlinNoise& noise, glm::ivec2 chunkCoord) {
    this->coord = chunkCoord;
    glm::vec2 worldOffset = chunkToWorld(chunkCoord);
    float spacing = CHUNK_SIZE / (CHUNK_RES - 1);

    std::vector<TerrainVertex> vertices;
    vertices.reserve(CHUNK_RES * CHUNK_RES);

    for (int z = 0; z < CHUNK_RES; z++) {
        for (int x = 0; x < CHUNK_RES; x++) {
            TerrainVertex v;

            // Weltposition dieses Vertex
            float wx = worldOffset.x + x * spacing;
            float wz = worldOffset.y + z * spacing;

            // Noise-Abfrage mit Weltkoordinaten (nicht Chunk-lokalen!)
            // Das ist wichtig: nahtlose Übergänge zwischen Chunks
            float h = fbm(noise, wx, wz, 6, 0.005f, 0.5f, 2.0f);

            v.position = glm::vec3(wx, h * 20.0f, wz);
            v.uv       = glm::vec2((float)x / (CHUNK_RES - 1),
                                   (float)z / (CHUNK_RES - 1));
            v.normal   = glm::vec3(0, 1, 0); // Platzhalter

            vertices.push_back(v);
        }
    }

    // Normals berechnen (nach dem Füllen der Positionen)
    recomputeNormals(vertices, CHUNK_RES, CHUNK_RES);

    // Indices generieren
    auto indices = generateIndices(CHUNK_RES, CHUNK_RES);
    indexCount = (int)indices.size();

    // GPU-Upload
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // ... (wie in Abschnitt 2.3)

    isLoaded = true;
}
```

**Warum Weltkoordinaten für den Noise-Aufruf?**
Wenn man Chunk-lokale Koordinaten (0..63) für den Noise-Aufruf verwendet, würden alle Chunks identisch aussehen (jeder beginnt bei 0,0). Durch die Weltkoordinate wird der Noise korrekt positioniert, und Chunk-Grenzen sind nahtlos.

### 8.3 Chunk-Manager

```cpp
class TerrainManager {
    std::unordered_map<glm::ivec2, TerrainChunk, IVec2Hash> chunks;
    PerlinNoise noise;
    int viewDistance = 4; // Chunks in jede Richtung

public:
    void update(glm::vec3 cameraPos) {
        glm::ivec2 camChunk = worldToChunk(cameraPos);

        // Lade Chunks im View-Radius
        for (int dz = -viewDistance; dz <= viewDistance; dz++) {
            for (int dx = -viewDistance; dx <= viewDistance; dx++) {
                glm::ivec2 c = camChunk + glm::ivec2(dx, dz);
                if (chunks.find(c) == chunks.end()) {
                    chunks[c].generate(noise, c);
                }
            }
        }

        // Entlade Chunks außerhalb des Radius
        for (auto it = chunks.begin(); it != chunks.end(); ) {
            glm::ivec2 diff = it->first - camChunk;
            if (std::abs(diff.x) > viewDistance + 1 ||
                std::abs(diff.y) > viewDistance + 1) {
                it->second.unload();
                it = chunks.erase(it);
            } else {
                ++it;
            }
        }
    }

    void render(ShaderProgram& shader) {
        for (auto& [coord, chunk] : chunks) {
            if (chunk.isLoaded) chunk.draw();
        }
    }
};
```

**`std::unordered_map` mit `glm::ivec2`:**
Standardmäßig hat `glm::ivec2` keine Hash-Funktion für `unordered_map`. Ein einfacher Hash:
```cpp
struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const {
        size_t h1 = std::hash<int>()(v.x);
        size_t h2 = std::hash<int>()(v.y);
        return h1 ^ (h2 << 32); // XOR mit Shift — einfach und ausreichend
    }
};
```

---

## 9. Verbindung zu Lego-Terrain

Das hier beschriebene kontinuierliche Terrain ist die Grundlage für einen Lego/Voxel-Terrain-Generator. Der Unterschied liegt in der **Diskretisierung** der Höhe.

### 9.1 Von kontinuierlich zu Lego-Stufen

Kontinuierliches Terrain: jede beliebige Y-Höhe möglich
Lego-Terrain: Höhe wird auf ganzzahlige Brick-Stufen gerundet

```cpp
const float BRICK_HEIGHT = 1.0f; // Höhe eines Lego-Bricks in Welteinheiten

float quantizeHeight(float h, float maxHeight) {
    float worldH = h * maxHeight;
    // Abrunden auf nächste Brick-Stufe
    int brickLevel = (int)std::floor(worldH / BRICK_HEIGHT);
    return brickLevel * BRICK_HEIGHT;
}
```

### 9.2 Voxel-Architektur (Ausblick)

Für echtes Lego-Terrain ersetzt man das kontinuierliche Mesh durch ein **3D-Voxel-Grid**:
- Jedes Voxel = ein potenzieller Brick (belegt oder leer)
- Die Oberfläche des Terrains wird durch die Grenze belegt/leer definiert
- Rendering: nur die sichtbaren Faces der Voxel (Culling der Innenflächen)

Das ist die Architektur von Minecraft, und der Unterschied zu dem hier beschriebenen Terrain ist groß — aber das Noise-System und das Chunking-System bleiben identisch. Die Noise-Funktion bestimmt weiterhin die Höhe, nur die Darstellung ändert sich von einem interpolierten Mesh zu einem Haufen diskreter Würfel.

### 9.3 Heightmap → Lego-Terrain (direkter Weg)

Der einfachste Weg ohne vollständige Voxel-Architektur:
Für jede Grid-Zelle `(x, z)` die Höhe berechnen, in Brick-Stufen quantisieren, und dann **einen Stapel Brick-Meshes** mit Instanced Rendering zeichnen (siehe Tutorial 01 — Instanced Rendering).

```
fBm-Noise → float h ∈ [0,1]
→ quantizeHeight(h) → int brickLevel
→ für jedes Level 0..brickLevel: eine Brick-Instanz bei (x, brickLevel, z)
→ alle Instanzen per glDrawArraysInstanced rendern
```

Das ist die direkte Verbindung zwischen diesem Tutorial und dem Instanced Rendering Tutorial.

---

## Zusammenfassung

| Konzept | Kurzbeschreibung |
|---|---|
| Terrain-Mesh | Gitter aus Dreiecken, Höhe auf Y-Achse |
| Index Buffer | Vertices einmal speichern, mehrfach referenzieren |
| GL_DYNAMIC_DRAW | VBO für häufig aktualisierte Daten |
| Heightmap | Graustufenbild als Höhendatenquelle |
| Normals | Gradient-Methode für Beleuchtung |
| Perlin Noise | Glatte, deterministische Zufallsfunktion |
| fBm / Oktaven | Mehrere Noise-Schichten für Detailtiefe |
| Höhenremap | Nicht-lineare Transformation für Terrain-Charakter |
| Chunks | Terrain in Segmente unterteilen für Streaming |
| Quantisierung | Höhe auf Stufen runden → Voxel/Lego-Logik |

Der typische Ablauf beim Generieren eines Terrain-Frames:

```
1. Seed / Parameter ändern
        ↓
2. fBm-Noise für alle Vertices berechnen
        ↓
3. Vertex-Positionen (Y) aktualisieren
        ↓
4. Normals neu berechnen
        ↓
5. VBO per glBufferSubData aktualisieren
        ↓
6. Rendern (VAO binden, glDrawElements)
```
