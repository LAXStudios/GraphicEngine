# Dynamische Terrain-Generierung — Ein schrittweises Tutorial

> [!abstract] Lernziel
> Du baust ein prozedural generiertes Terrain Schritt für Schritt auf.
> Nach jedem Abschnitt hast du etwas **Sichtbares** im Fenster — kein blinder Code.
>
> Dieses Tutorial erklärt das *Warum* bevor das *Wie* kommt.
> Loesungen sind am Ende jedes Schritts eingeklappt — versuche wirklich zuerst selbst,
> bevor du aufklappst.

---

## Wo wir stehen — und wo wir hinwollen

Schau dir kurz an, was schon im Projekt existiert:

- **`TerrainMesh.cpp`** — Grid-Generierung, VAO/VBO/EBO-Upload — fertig
- **`terrain.glsl`** — Shader mit Hoehenfarben und Beleuchtung — fertig
- **Normals** — alle `(0, 1, 0)`, also **Platzhalter** — fehlt noch
- **Hoehenfunktion** — sin/cos als Platzhalter, kein echter Noise — fehlt noch

Der Plan:

```
Schritt 1  Geometrie verstehen: eigene Hoehenfunktion bauen
Schritt 2  Normals korrekt berechnen (Beleuchtung lebt!)
Schritt 3  Perlin Noise: was er ist und wie er funktioniert
Schritt 4  Frequency & Amplitude: live tweaken mit ImGui
Schritt 5  fBm: von glatt zu realistisch
Schritt 6  Height Remapping: Charakter ins Terrain
Schritt 7  Alles mit ImGui steuerbar machen
Schritt 8  Chunk-System: die Welt wird groesser
```

---

## Schritt 1 — Das Grid und die Hoehenfunktion

### Was werden wir bauen?

Ein Terrain das du selbst mit einer Funktion formst — bevor wir Noise benutzen,
verstehen wir das Prinzip: **Y ist eine Funktion von X und Z**.

### Warum ist das wichtig?

Ein Terrain ist am Ende nichts als ein flaches Grid, bei dem jeder Punkt eine Hoehe bekommt:

```
Flaches Grid:          Mit Hoehe:
  .   .   .   .         .   .   .   .
  .   .   .   .         . ^ . ^ .   .
  .   .   .   .         .   . ^ .   .
  .   .   .   .         .   .   .   .
```

Der **einzige Unterschied** zwischen einem flachen Boden und einem Berg ist:
`position.y = f(position.x, position.z)`.

Oeffne `TerrainMesh.cpp` und schau die `generateGrid`-Funktion an:

```cpp
vertex.position.y = ((sx * cz) + 1.0f) * 0.5f * 5.0f;
// das ist der aktuelle Platzhalter: sin * cos
```

### Denk selbst nach

Bevor du weiter liest: Was wuerde passieren wenn du `position.y` so setzt?

```cpp
float dx = vertex.position.x;
float dz = vertex.position.z;
vertex.position.y = sqrt(dx*dx + dz*dz) * 0.2f;
```

Zeichne die Form kurz in deinem Kopf oder auf Papier.
Dann implementiere es und schau ob du richtig lagst.

> [!question]- Aufloesung
> `sqrt(x^2 + z^2)` ist die **Distanz vom Ursprung** — das ergibt einen Kegel,
> der in der Mitte flach ist und nach aussen ansteigt. Du hast einen invertierten Berg gebaut.
>
> ```
> Draufsicht (Hoehenlinien):
>     # # # # #
>   #   o o o   #
>   #  o   .  o #    . = Mitte (niedrig)
>   #   o o o   #    # = aussen (hoch)
>     # # # # #
> ```

### Aufgabe 1.1 — Experimentiere mit Formen

Versuche diese Formen nacheinander als `position.y` zu implementieren.
Baue jede ein, compile, schau sie an, verstehe warum sie so aussieht:

| Ausdruck                                                 | Was entsteht?              |
| -------------------------------------------------------- | -------------------------- |
| `5.0f`                                                   | konstant — flache Ebene    |
| `position.x * 0.1f`                                      | schräge                    |
| `sin(position.x * 0.3f) * 3.0f`                          | wellen entlang der X-Achse |
| `sin(position.x * 0.3f) * cos(position.z * 0.3f) * 3.0f` | Symmetrische Berge Muster  |

Keine Angst vorm Experimentieren — der Code ist dein Spielplatz.

### Was du nach Schritt 1 siehst

Ein Terrain das sich durch simples Ändern einer Zeile komplett verändert.
Das ist der Kern-Gedanke: **Terrain = Geometrie + Höhenfunktion**. Der Rest ist Qualität.

---

## Schritt 2 — Normals: Beleuchtung zum Leben erwecken

### Das Problem

Öffne das Programm. Drehe die Lichtrichtung in ImGui.
Fällt etwas auf? Die **Beleuchtung ignoriert die Geometrie** völlig.

Warum? Schau in `generateGrid`:

```cpp
vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Platzhalter!
```

Jeder Vertex sagt der GPU: "Meine Oberfläche zeigt senkrecht nach oben."
Aber ein Hang zeigt zur Seite! Ein Tal zeigt schräg nach unten!

Die GPU berechnet die Beleuchtung mit der Normal — wenn die Normal falsch ist,
sieht die Beleuchtung falsch aus.

### Was ist eine Normal?

Eine Normal ist ein **Einheitsvektor** (Länge = 1) der **senkrecht** auf einer Fläche steht:

```
Flache Flaeche:    Hanglage nach rechts:
    | N               / N
    |                /
  ------          ------
```

An einem steilen Hang zeigt die Normal fast waagerecht.
An einer flachen Ebene zeigt sie fast senkrecht nach oben.

### Denk selbst nach — die Gradient-Methode

Hier die entscheidende Frage: Wie berechne ich die Normal an Punkt P,
wenn ich nur die **Höhe** der benachbarten Punkte kenne?

```
        N (noerdlicher Nachbar)
        |
W ------P------ E
        |
        S (suedlicher Nachbar)
```

Die Steigung in X-Richtung ist proportional zu `hoehe(E) - hoehe(W)`.
Die Steigung in Z-Richtung ist proportional zu `hoehe(N) - hoehe(S)`.

Wenn das Gelaende von links nach rechts steil ansteigt, dann ist `hoehe(E) - hoehe(W)` gross.
Und die Normal muss dann nach links kippen — entgegen der Steigungsrichtung.

Kannst du daraus einen Normalvektor konstruieren, bevor du weiter liest?

> [!tip]- Hint: Die Gradient-Methode
> Der Trick: Wir bauen den Normalvektor direkt aus den Steigungen:
>
> ```
> Steigung in X:  dX = hoehe(links) - hoehe(rechts)
> Steigung in Z:  dZ = hoehe(vorne) - hoehe(hinten)
>
> Normal = normalize(vec3(dX, 2.0f, dZ))
> ```
>
> Der Y-Wert `2.0f` ist ein Skalierungsfaktor:
> - Hoeher = weniger ausgepragte Kippe (Normals bleiben mehr "aufrecht")
> - Niedriger = staerkere Kippe (Beleuchtung reagiert sensibler auf Steigung)
>
> Warum ist das eine Vereinfachung? Echte Normals wuerden das Cross Product zweier
> Dreieckskanten berechnen. Die Gradient-Methode kommt aber fuer Terrain zu praktisch
> denselben Ergebnissen und ist deutlich simpler.

### Aufgabe 2.1 — Normal-Berechnung implementieren

Füge in `TerrainMesh.cpp` eine Methode hinzu:

```cpp
glm::vec3 TerrainMesh::computeNormal(int x, int z, int width, int height) {
    // Deine Aufgabe:
    // 1. Index der 4 Nachbarn berechnen (clamp an den Raendern!)
    // 2. Hoehen der Nachbarn holen
    // 3. Gradienten berechnen
    // 4. Normal konstruieren und normalisieren
}
```

**Wichtig beim Clampen:** Was passiert mit Punkten am Rand des Grids?
Die haben keine 4 Nachbarn. Nutze `glm::clamp(x, 0, width-1)` um sicher zu lesen.

> [!example]- Referenzloesung (erst selbst versuchen)
> ```cpp
> glm::vec3 TerrainMesh::computeNormal(int x, int z, int width, int height) {
>     // Hilfsfunktion: Index mit Randbehandlung
>     auto idx = [&](int px, int pz) -> int {
>         px = glm::clamp(px, 0, width  - 1);
>         pz = glm::clamp(pz, 0, height - 1);
>         return pz * width + px;
>     };
>
>     // Hoehen der 4 Nachbarn holen
>     float hL = vertices[idx(x - 1, z    )].position.y; // links  (West)
>     float hR = vertices[idx(x + 1, z    )].position.y; // rechts (East)
>     float hN = vertices[idx(x,     z - 1)].position.y; // nord   (vorne)
>     float hS = vertices[idx(x,     z + 1)].position.y; // sued   (hinten)
>
>     // Gradient -> Normal
>     return glm::normalize(glm::vec3(
>         hL - hR,   // X-Steigung (invertiert weil Normal entgegen Steigung)
>         2.0f,      // Y: Skalierungsfaktor
>         hN - hS    // Z-Steigung
>     ));
> }
> ```

### Aufgabe 2.2 — Normals nach dem Generieren setzen

Nach dem Fuellen aller Vertex-Positionen in `generateGrid` musst du eine zweite
Schleife hinzufuegen die alle Normals setzt. Erst dann, wenn alle Hoehen bekannt sind!

**Warum erst danach?** Wenn du die Normal von Punkt (5, 3) berechnen willst,
brauchst du die Hoehe von Punkt (6, 3). Wenn der noch nicht gesetzt ist, liest du Muell.

> [!tip]- Hint: Wo genau einfuegen?
> ```cpp
> // Am Ende von generateGrid, nach der Positions-Schleife:
> for (int z = 0; z < height; z++) {
>     for (int x = 0; x < width; x++) {
>         vertices[z * width + x].normal = computeNormal(x, z, width, height);
>     }
> }
> ```
>
> Du musst `computeNormal` so deklarieren dass es auf den `vertices`-Vector zugreifen kann —
> entweder als Parameter oder als Member-Variable.

### Was du nach Schritt 2 siehst

Drehe jetzt die Lichtrichtung in ImGui — die Haenge werden heller und dunkler,
Taeler liegen im Schatten, Gipfel leuchten. Beleuchtung lebt.

Das ist ein riesiger Unterschied. Gute Normals sind 50% des visuellen Qualitaetssprungs.

---

## Schritt 3 — Perlin Noise: Die Physik des "zufaellig Aussehenden"

### Das Problem mit echtem Zufall

Ersetze die Hoehenfunktion kurz durch:

```cpp
vertex.position.y = static_cast<float>(rand()) / RAND_MAX * 5.0f;
```

Schau dir das Ergebnis an. **Warum sieht das wie Stacheln aus und nicht wie Gelaende?**

Echter Zufall hat keine raeumliche Kohaerenz — jeder Punkt ist unabhaengig.
Real wirkende Landschaft hat **weiche Uebergaenge**: benachbarte Punkte haben aehnliche Hoehen.

Das ist genau was **Noise** leistet: eine deterministisch-zufaellige Funktion die
*raeumlich kohaerent* ist.

### Was unterscheidet Noise von rand()?

| Eigenschaft | `rand()` | Perlin Noise |
|---|---|---|
| Benachbarte Punkte | voellig unabhaengig | aehnliche Werte |
| Selbe Eingabe = selbe Ausgabe | nein | ja, immer |
| Wirkt "natuerlich" | nein | ja |
| Skalierbar (zoom in/out) | nein | ja |

### Das Prinzip von Perlin Noise — ohne Code

Stell dir ein unsichtbares Gitter vor. An jedem **Gitterpunkt** liegt ein zufaelliger
Pfeil (Gradient-Vektor). Diese Pfeile sind vom Seed abhaengig — fuer denselben Seed
immer dieselben Pfeile.

```
  /       \
     . P .       P = der Punkt der abgefragt wird
  \       /
```

Fuer einen Abfragepunkt P zwischen 4 Gitterpunkten:
1. Berechne den Vektor von jedem Gitterpunkt **zu P**
2. Berechne das **Skalarprodukt** dieses Vektors mit dem Gradient-Pfeil des Gitterpunkts
3. **Interpoliere** die 4 Ergebnisse mit einer glatten Kurve

### Denk selbst nach: Warum keine lineare Interpolation?

Wenn du zwei Werte linear interpolierst, hat die Kurve an den Endpunkten
eine abrupte Richtungsaenderung (Knick). Ueberlege:

```
Lineare Interpolation:           Smooth-Step:
    *                               *
     \          *               ..   ..
      \       ./               .       .
       \    ./                .         *
        \./
         *
```

Bei Perlin Noise wuerden lineare Uebergaenge an jedem Gitterpunkt **sichtbare Grate**
erzeugen. Die Quintic-Ease `6t^5 - 15t^4 + 10t^3` hat an t=0 und t=1 sowohl
erste als auch zweite Ableitung = 0 — perfekt glatte Uebergaenge ohne Knicke.

### Die Permutationstabelle — das Herz von Perlin Noise

Anstatt Gradient-Vektoren explizit zu speichern, nutzt Perlin Noise einen Trick:
eine **Permutation der Zahlen 0..255** dient als Hash-Funktion.

```
Permutationstabelle: [42, 17, 233, 1, 89, ...]  <- einmalig aus Seed gemischt
                                                    <- immer 256 Zahlen
```

Fuer einen Gitterpunkt `(X, Z)` wird der Gradient-Vektor ueber `p[p[X] + Z]`
abgefragt — eine Zahl 0..255 die dann auf 4 Gradient-Richtungen gemappt wird.

Das ist elegant: keine grosse Tabelle von Vektoren, nur 256 Zahlen. Der ganze
"Zufall" steckt darin wie diese Tabelle per Seed gemischt wurde.

### Aufgabe 3.1 — Die Noise-Klasse

Erstelle eine neue Datei `PerlinNoise.h` im TerrainGenerationScene-Ordner.

Implementiere eine Klasse mit mindestens:

```cpp
class PerlinNoise {
public:
    PerlinNoise(unsigned int seed);
    float noise(float x, float z) const;   // Rueckgabe: ca. -1..1
    float noise01(float x, float z) const; // Rueckgabe: 0..1

private:
    std::array<int, 512> p; // Permutationstabelle (doppelt fuer Wrap-Around)
};
```

Beginne mit dem Grundgeruest: die Permutationstabelle mit `std::iota` und
`std::shuffle` aufbauen. Das ist der einfachste Teil.

> [!tip]- Hint: Permutationstabelle aufbauen
> ```cpp
> #include <array>
> #include <numeric>    // std::iota
> #include <algorithm>  // std::shuffle
> #include <random>     // std::default_random_engine
>
> PerlinNoise::PerlinNoise(unsigned int seed) {
>     // Tabelle mit 0, 1, 2, ..., 255 fuellen
>     std::iota(p.begin(), p.begin() + 256, 0);
>
>     // Mit dem Seed mischen
>     std::default_random_engine engine(seed);
>     std::shuffle(p.begin(), p.begin() + 256, engine);
>
>     // Doppeln fuer einfachen Wrap-Around (kein Modulo noetig)
>     for (int i = 0; i < 256; i++)
>         p[256 + i] = p[i];
> }
> ```
>
> Die Tabelle wird **einmal im Konstruktor** aufgebaut. Danach ist der Noise
> vollstaendig deterministisch — kein Zufall mehr, nur Arithmetik.

> [!tip]- Hint: Die noise()-Funktion Schritt fuer Schritt
> ```cpp
> float PerlinNoise::noise(float x, float z) const {
>     // 1. Welche Gitterzelle? (&255 = Modulo 256, schnell)
>     int X = (int)std::floor(x) & 255;
>     int Z = (int)std::floor(z) & 255;
>
>     // 2. Position innerhalb der Zelle (0..1)
>     x -= std::floor(x);
>     z -= std::floor(z);
>
>     // 3. Smooth-Step (Quintic Ease)
>     float u = fade(x);
>     float v = fade(z);
>
>     // 4. Hash der 4 Eckpunkte
>     int aa = p[p[X  ] + Z  ];  // links,  unten
>     int ba = p[p[X+1] + Z  ];  // rechts, unten
>     int ab = p[p[X  ] + Z+1];  // links,  oben
>     int bb = p[p[X+1] + Z+1];  // rechts, oben
>
>     // 5. Erst in X interpolieren, dann in Z
>     return lerp(v,
>         lerp(u, grad(aa, x,     z    ),
>                 grad(ba, x-1.f, z    )),
>         lerp(u, grad(ab, x,     z-1.f),
>                 grad(bb, x-1.f, z-1.f))
>     );
> }
> ```

> [!tip]- Hint: fade(), lerp(), grad()
> ```cpp
> // Quintic Ease Curve: 6t^5 - 15t^4 + 10t^3
> static float fade(float t) {
>     return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
> }
>
> // Lineare Interpolation
> static float lerp(float t, float a, float b) {
>     return a + t * (b - a);
> }
>
> // Pseudo-Gradient: 4 moegliche Richtungen aus dem Hash
> static float grad(int hash, float x, float z) {
>     switch (hash & 3) {
>         case 0: return  x + z;
>         case 1: return -x + z;
>         case 2: return  x - z;
>         case 3: return -x - z;
>     }
>     return 0.f;
> }
> ```
>
> `grad` mappt einen Hash-Wert (0..255) auf eine von 4 Gradient-Richtungen
> und berechnet das Skalarprodukt mit dem Offset-Vektor (x, z).
> Das ist die vereinfachte 2D-Version — 3D-Perlin Noise nutzt 12 Richtungen.

### Aufgabe 3.2 — Noise als Hoehenfunktion einsetzen

Ersetze in `generateGrid` die sin/cos-Funktion durch deinen Noise.
Erstelle das `PerlinNoise`-Objekt im Konstruktor oder uebergib es als Parameter:

```cpp
PerlinNoise noise(42); // Seed 42

// In der Schleife:
float frequency = 0.05f;
float h = noise.noise01(x * frequency, z * frequency);
vertex.position.y = h * maxHeight;
```
%%
```git
- float h = noise.noise01(...);
=================
+ float h = noise.noise(...);
```
%%
### Was du nach Schritt 3 siehst

Dein erstes Noise-Terrain. Es wirkt schon deutlich natuerlicher als sin/cos.
Aendere den Seed — komplett andere Landschaft. Aendere die Frequenz — komplett andere Skala.

> [!note] Schluesselerkenntnis
> Seed + Frequenz = die ganze Identitaet des Terrains.
> Das ist der Kern von prozeduraler Generierung.

---

## Schritt 4 — Frequency & Amplitude: Parameter-Intuition

### Warum Parameter verstehen, bevor man sie nutzt?

Bevor wir mehrere Noise-Schichten übereinander legen, müssen wir verstehen
was `frequency` eigentlich *bedeutet* und wie sie das Terrain verändert.

### Denk selbst nach — Frequenz-Intuition

Stell dir vor du fährst mit dem Auto über eine Strasse mit Wellen.
- **Niedrige Frequenz:** die Wellen sind weit auseinander — du fährst langsam auf und ab
- **Hohe Frequenz:** die Wellen sind eng — du hoppelst schnell

Was passiert wenn du `frequency` in deinem Code von `0.01f` auf `0.5f` erhöhst?
Stelle eine Hypothese auf, dann probiere es aus.

> [!question]- Aufloesung
> Bei **hoher Frequenz** werden die Noise-Koordinaten schneller veraendert,
> also "reist" du schneller durch das Noise-Feld. Das Ergebnis: viele kleine Huegel.
>
> Bei **niedriger Frequenz** beruehrst du nur einen kleinen Teil des Noise-Felds —
> grosse, sanfte Berge.
>
> Die Frequenz ist der "Zoom-Faktor" in das Noise-Feld.
>
> ```
> Niedrige Frequenz:          Hohe Frequenz:
>   /---\   /---\             /\/\/\/\/\
>  /     \_/     \            /          \
> ```

### Aufgabe 4.1 — Visualisierung bauen

Füge in `TerrainGenerationScene.h` unter `ImGuiLayer()` Slider für
Frequenz und Amplitude hinzu. Wenn du den Slider bewegst, soll das Terrain
**sofort** neu generiert werden.

Das erfordert:
1. Frequenz und Amplitude als Member-Variablen in der Scene
2. Ein Terrain-Rebuild wenn sie sich ändern
3. Die Noise-Berechnung parametrisierbar machen

> [!tip]- Hint: Wie die Parameter in den Mesh kommen
> Eine saubere Loesung: `TerrainMesh` bekommt eine `regenerate(params)`-Methode die:
> - die Vertex-Positionen neu berechnet
> - die Normals neu berechnet
> - den VBO mit `glBufferSubData` aktualisiert (ohne neuen GPU-Speicher)
>
> ```cpp
> // Schneller als glBufferData — kein neuer Speicher:
> glBindBuffer(GL_ARRAY_BUFFER, VBO);
> glBufferSubData(GL_ARRAY_BUFFER, 0,
>     vertices.size() * sizeof(TerrainVertex),
>     vertices.data());
> ```
>
> `glBufferSubData` ueberschreibt nur den Inhalt, nicht den Buffer selbst.
> Das ist der richtige Weg fuer haeufig wechselnde Daten.

### Was du nach Schritt 4 siehst

Du kannst Frequenz und Amplitude live tweaken und das Terrain veraendert sich in Echtzeit.
Du lernst die Parameter durch direktes Spueren kennen — besser als jede Erklaerung.

---

## Schritt 5 — fBm: Von glatt zu realistisch

### Das Problem mit einem einzelnen Noise-Pass

Schau dir echte Satellitenbilder von Bergen an (Google Maps, Satellite-View).
Was faellt auf? Es gibt gleichzeitig:
- **Grosse Bergzuege** (Skala: 100 km)
- **Einzelne Berge** (Skala: 10 km)
- **Felsen und Kuppen** (Skala: 1 km)
- **Kleine Unebenheiten** (Skala: 100 m)

Ein einzelner Noise-Pass hat **eine** Skala. Er kann entweder grosse Bergzuege oder
kleine Felsen zeigen — nicht beides gleichzeitig.

### Die Loesung: Mehrere Schichten uebereinander

**Fractional Brownian Motion (fBm)** ist das mathematische Konzept dahinter.
Die Idee: addiere mehrere Noise-Schichten (Oktaven) mit abnehmender Amplitude
und zunehmender Frequenz:

```
Oktave 1: freq=0.005, amp=1.0    riesige Bergzuege
Oktave 2: freq=0.010, amp=0.5    mittlere Huegel
Oktave 3: freq=0.020, amp=0.25   kleine Felsen
Oktave 4: freq=0.040, amp=0.125  winzige Unebenheiten

Summe: Mehrere Skalen gleichzeitig
```

### Denk selbst nach — der Zusammenhang zwischen Oktaven

Warum wird die Amplitude mit jeder Oktave **kleiner**, nicht groesser?

Stell dir vor du zeichnest eine Landkarte: Die grossen Strukturen (Kontinente)
bestimmen das Bild, die kleinen Details (Fluesse, Strassen) ergaenzen.
Wenn Fluesse genauso "laut" waeren wie Kontinente, waere die Karte unleserlich.

Die hohen Oktaven fuegen **Detailrauschen** hinzu, sie sollen die Grundform nicht uebertoenen.

Die zwei Schlusselparameter:
- **Lacunarity** (typisch 2.0): wie viel schneller wird jede Oktave? (Frequenz-Multiplikator)
- **Persistence** (typisch 0.5): wie viel leiser wird jede Oktave? (Amplituden-Multiplikator)

### Aufgabe 5.1 — fBm implementieren

Implementiere die Funktion:

```cpp
float fbm(const PerlinNoise& noise, float x, float z,
          int octaves, float baseFrequency,
          float persistence, float lacunarity);
```

Denke dabei:
- Wie normalisierst du den Ausgabewert auf 0..1? (Tipp: akkumuliere den maximalen moeglichen Wert)
- Was passiert wenn `persistence = 1.0` statt 0.5?
- Was passiert wenn `lacunarity = 1.0` statt 2.0?

> [!example]- Referenzloesung
> ```cpp
> float fbm(const PerlinNoise& noise, float x, float z,
>           int octaves, float baseFreq, float persistence, float lacunarity)
> {
>     float value     = 0.0f;
>     float amplitude = 1.0f;
>     float frequency = baseFreq;
>     float maxValue  = 0.0f;   // fuer Normalisierung
>
>     for (int i = 0; i < octaves; i++) {
>         value    += noise.noise01(x * frequency, z * frequency) * amplitude;
>         maxValue += amplitude;
>
>         amplitude *= persistence;  // jede Oktave leiser
>         frequency *= lacunarity;   // jede Oktave feiner
>     }
>
>     return value / maxValue;  // -> 0..1
> }
> ```
>
> **Warum `value / maxValue`?**
> Wenn alle Oktaven ihren Maximalwert 1.0 liefern wuerden:
> - Oktave 1: `1.0 * 1.0 = 1.0`
> - Oktave 2: `1.0 * 0.5 = 0.5`
> - Oktave 3: `1.0 * 0.25 = 0.25`
> - Summe: `1.75`
>
> `maxValue` akkumuliert genau diese Summe. Durch Division bekommst du immer 0..1,
> egal wie viele Oktaven du nimmst.

### Aufgabe 5.2 — Parameter vergleichen

Experimentiere systematisch. Aendere **einen Parameter** und beobachte den Effekt:

| Experiment | Parameter-Aenderung | Was erwartest du? |
|---|---|---|
| Weniger Oktaven | `octaves: 6 -> 2` | ? |
| Mehr Persistence | `persistence: 0.5 -> 0.8` | ? |
| Weniger Lacunarity | `lacunarity: 2.0 -> 1.5` | ? |
| Mehr Frequenz | `baseFreq: 0.005 -> 0.02` | ? |

Schreibe deine Erwartungen auf, dann pruefe sie.

### Was du nach Schritt 5 siehst

Terrain das wie echtes Gelaende aussieht — mit Bergzuegen, Huegeln, und kleinen Details.
Das ist der Qualitaetssprung von "offensichtlich prozedural" zu "koennte real sein".

---

## Schritt 6 — Height Remapping: Charakter ins Terrain

### Das Problem mit rohem fBm

Perlin-fBm verteilt die Hoehenwertes annaehernd gleichmaessig zwischen 0 und 1.
Das ergibt Terrain mit symmetrischen Huegeln — aber echte Landschaft hat meistens:
- Viel **flaches Tiefland** (Taeler, Ebenen)
- Wenig **extrem hohe** Gipfel

### Nicht-lineare Transformation der Hoehe

Die Idee: bevor wir den Noise-Wert als Hoehe verwenden, transformieren wir ihn.

```
h (Noise)  ->  f(h)  ->  position.y
```

Eine einfache Transformation: Potenz-Funktion.

```
h^1.0  gleichmaessig (das was fBm liefert)
h^2.0  alles wird kleiner, Gipfel schrumpfen staerker als Taeler
h^0.5  alles wird groesser, Gipfel wachsen staerker als Taeler
```

### Denk selbst nach — Kurvenformen

Was macht `pow(h, 3.0f)` mit Werten nahe 0 und nahe 1?

```
h = 0.1:  pow(0.1, 3.0) = 0.001
h = 0.5:  pow(0.5, 3.0) = 0.125
h = 0.9:  pow(0.9, 3.0) = 0.729
```

Welches Terrain entsteht dadurch — bevor du weiter liest?

> [!question]- Aufloesung
> Niedrige Werte werden stark reduziert — breite, flache Taeler.
> Hohe Werte bleiben relativ gross — spitze, markante Gipfel.
>
> Das ist genau wie echte Erosion wirkt: Wasser schleift Taeler flach,
> aber Berggipfel sind aus hartem Gestein und ragen auf.
>
> ```
> Ohne Remap:  n  n  n  n   (symmetrische Huegel)
> Mit pow(h,3): .  .  A  .   (flache Taeler, einzelne Spitzen)
> ```

### Aufgabe 6.1 — Experimentierstrecke

Implementiere folgende Transformationen als kleine Funktionen und pruefe das Ergebnis:

```cpp
// Transformation 1: Spitze Berge, flache Taeler
float remap1(float h) { return std::pow(h, 2.5f); }

// Transformation 2: Meeresspiegel einfuehren
float remap2(float h) {
    float seaLevel = 0.35f;
    return h < seaLevel ? seaLevel : h;
}

// Transformation 3: Kombiniert — deine Aufgabe:
// Wie kombinierst du Meeresspiegel UND spitze Berge?
float remap3(float h) {
    // ???
}
```

### Aufgabe 6.2 — ImGui-Kontrolle fuer den Remap

Fuege einen Slider fuer den Potenz-Exponent hinzu (`1.0` bis `4.0`).
Beobachte wie sich der Charakter des Terrains aendert wenn du in Echtzeit tweakst.

### Was du nach Schritt 6 siehst

Ein Terrain das einen **Charakter** hat — nicht mehr nur generisches Noise-Gelaende.
Durch Remap kannst du zwischen "Islands mit Ozeanen", "kontinentale Hochebenen"
und "alpines Hochgebirge" wechseln.

---

## Schritt 7 — Alles mit ImGui kontrollieren

### Warum ImGui-Integration wichtig ist

Du hast jetzt viele Parameter: Seed, `octaves`, `baseFrequency`, `persistence`,
`lacunarity`, `maxHeight`, Remap-Exponent, Meeresspiegel.

Ohne Live-Kontrolle musst du fuer jede Aenderung neu kompilieren.
Mit ImGui-Slidern lernst du die Parameter in 5 Minuten besser kennen
als durch 2 Stunden Code-Lesen.

### Aufgabe 7.1 — Vollstaendiges Parameter-Panel

Ziel: Ein ImGui-Fenster mit allen Terrain-Parametern.
Bei jeder Aenderung wird das Terrain **sofort** neu generiert.

Ueberlege zuerst: Wo in der Architektur macht das Sinn?
- **Option A:** ImGui-Aenderung -> Mesh direkt regenerieren
- **Option B:** ImGui-Aenderung -> Flag setzen -> naechster `Update()`-Frame regeneriert

Welche ist sauberer und warum?

> [!tip]- Diskussion: Option A vs. B
> **Option B ist sauberer.**
>
> Option A hat das Problem dass `ImGuiLayer()` jetzt Seiteneffekte auf den Mesh hat.
> Das Rendering-System kennt ploetzlich Mesh-Details — das verletzt die Trennung der Verantwortlichkeiten.
>
> Option B: `ImGuiLayer()` aendert nur Variablen. `Update()` entscheidet was neu generiert wird.
>
> ```cpp
> // In der Scene-Klasse:
> bool terrainDirty = false;
>
> // In ImGuiLayer():
> if (ImGui::SliderInt("Octaves", &octaves, 1, 8)) terrainDirty = true;
>
> // In Update():
> if (terrainDirty) {
>     terrainMesh->regenerate(octaves, baseFreq, persistence,
>                             lacunarity, maxHeight, remapExp, seaLevel, seed);
>     terrainDirty = false;
> }
> ```

### Aufgabe 7.2 — Seed-Randomizer-Button

Fuege einen Button hinzu der einen zufaelligen Seed generiert und das Terrain
sofort neu generiert. Das gibt dir schnelles Durchklicken durch verschiedene Landscapes.

```cpp
if (ImGui::Button("New Seed")) {
    seed = static_cast<unsigned int>(rand());
    terrainDirty = true;
}
```

### Was du nach Schritt 7 siehst

Dein eigenes interaktives Terrain-Labor. Klicke auf "New Seed" und erkunde
neue Welten. Das ist der Kern von prozeduraler Generierung in Spielen.

---

## Schritt 8 — Chunks: Die Welt wird groesser

### Warum nicht einfach das Grid vergroessern?

Versuche `TerrainMesh(500, 500, 2.5f)` — 250.000 Vertices.
Und `TerrainMesh(2000, 2000, 2.5f)` — 4 Millionen Vertices.

Das Problem: Alles auf einmal im GPU-Speicher. Und alles wird gerendert,
auch was hinter dem Spieler liegt.

Die Loesung: Das Terrain wird in **Chunks** aufgeteilt.
- Jeder Chunk = ein kleines Grid (z.B. 64x64 Punkte)
- Nur Chunks in der Naehe der Kamera werden geladen und gerendert
- Chunks die zu weit weg sind werden entladen

### Das Schluesselproblem: Nahtlose Uebergaenge

Wenn jeder Chunk seine **lokalen** Koordinaten (0..63) als Noise-Input nutzt,
sehen alle Chunks identisch aus:

```
Chunk (0,0) nutzt Noise(0..63, 0..63)
Chunk (1,0) nutzt Noise(0..63, 0..63)  <- FALSCH: identisch!
```

Die Loesung: **Weltkoordinaten** als Noise-Input.

```
Chunk (0,0): Weltpos x=0..63    -> Noise(0..63,   0..63)
Chunk (1,0): Weltpos x=64..127  -> Noise(64..127, 0..63)  <- anderer Bereich
```

An der Chunk-Grenze (x=63 bei Chunk 0, x=64 bei Chunk 1) sind die Noise-Werte
nahezu identisch — weil Perlin Noise kontinuierlich ist. Nahtloser Uebergang.

### Denk selbst nach — Chunk-Architektur

Bevor du die Loesung liest: Wie wuerdest du Chunks organisieren?

Fragen die du beantworten musst:
1. Wie identifizierst du welcher Chunk zu welcher Weltposition gehoert?
2. Wo speicherst du die aktiven Chunks?
3. Wie entscheidest du welche Chunks zu laden/entladen sind?
4. Was passiert wenn die Kamera an der Chunk-Grenze steht?

Skizziere eine Architektur (nur Klassen und ihre Beziehungen), dann weiterlesen.

### Chunk-Koordinaten

Jeder Chunk bekommt eine **Chunk-Koordinate** (integer) statt einer Weltkoordinate.

```cpp
const float CHUNK_WORLD_SIZE = (CHUNK_RES - 1) * spacing;

// Weltposition -> Chunk-Koordinate
glm::ivec2 worldToChunk(glm::vec2 worldPos) {
    return glm::ivec2(
        (int)std::floor(worldPos.x / CHUNK_WORLD_SIZE),
        (int)std::floor(worldPos.y / CHUNK_WORLD_SIZE)
    );
}

// Chunk-Koordinate -> Weltoffset (untere linke Ecke)
glm::vec2 chunkOrigin(glm::ivec2 chunkCoord) {
    return glm::vec2(chunkCoord) * CHUNK_WORLD_SIZE;
}
```

> [!note] Warum `(CHUNK_RES - 1) * spacing` und nicht `CHUNK_RES * spacing`?
> Der letzte Punkt von Chunk (0,0) und der erste Punkt von Chunk (1,0) liegen
> auf **derselben Weltposition** — ein Punkt Ueberlappung damit die Chunks
> nahtlos aneinandergrenzen. Ein Chunk mit 64 Punkten ist also 63 Abstaende breit.

### Aufgabe 8.1 — Chunk-Klasse

Erstelle eine `TerrainChunk`-Klasse:

```cpp
class TerrainChunk {
    glm::ivec2 coord;
    TerrainMesh mesh;
    bool isLoaded = false;

public:
    void generate(glm::ivec2 coord, const TerrainParams& params);
    void draw(ShaderProgram& shader);
    void unload();
};
```

Das **Wichtigste** bei `generate`: die Weltposition fuer den Noise-Aufruf berechnen:

```cpp
glm::vec2 origin = chunkOrigin(coord);

// In der Vertex-Schleife:
float wx = origin.x + x * spacing;  // <- Weltkoordinate, nicht lokales x!
float wz = origin.y + z * spacing;

float h = fbm(noise, wx * baseFreq, wz * baseFreq, ...);
```

### Aufgabe 8.2 — Chunk-Manager

Ein `TerrainManager` verwaltet die aktiven Chunks:

```cpp
class TerrainManager {
    std::unordered_map<glm::ivec2, TerrainChunk, IVec2Hash> chunks;
    int viewDistance = 3; // Chunks in jede Richtung

public:
    void update(glm::vec3 cameraPos, const TerrainParams& params);
    void render(ShaderProgram& shader);
};
```

Die `update`-Funktion:
1. Berechnet die aktuelle Kamera-Chunk-Koordinate
2. Iteriert ueber alle Chunks im `viewDistance`-Radius
3. Laedt Chunks die noch nicht existieren
4. Entlaedt Chunks die zu weit weg sind

> [!tip]- Hint: Chunks entladen ohne Iterator-Invalidierung
> Beim Entladen aus einer `unordered_map` waehrend der Iteration muss man aufpassen:
> `erase()` invalidiert den Iterator. Die sichere Methode:
>
> ```cpp
> for (auto it = chunks.begin(); it != chunks.end(); ) {
>     glm::ivec2 diff = it->first - camChunk;
>     bool tooFar = std::abs(diff.x) > viewDistance + 1 ||
>                   std::abs(diff.y) > viewDistance + 1;
>
>     if (tooFar) {
>         it->second.unload();      // GPU-Ressourcen freigeben
>         it = chunks.erase(it);   // gibt naechsten gueltigen Iterator zurueck
>     } else {
>         ++it;
>     }
> }
> ```
>
> `erase()` gibt den Iterator auf das **naechste Element** zurueck — deshalb
> kein `++it` wenn wir erased haben.

### Aufgabe 8.3 — Hash fuer glm::ivec2

`std::unordered_map` braucht eine Hash-Funktion fuer den Key-Typ.
`glm::ivec2` hat standardmaessig keine. Implementiere:

```cpp
struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const {
        // ???
    }
};
```

> [!tip]- Hint: Einfacher, guter Hash
> ```cpp
> struct IVec2Hash {
>     size_t operator()(const glm::ivec2& v) const {
>         size_t h1 = std::hash<int>()(v.x);
>         size_t h2 = std::hash<int>()(v.y);
>         return h1 ^ (h2 * 2654435761u); // Goldener Schnitt-Multiplikator
>     }
> };
> ```
>
> Der Multiplikator `2654435761` ist der Goldene Schnitt skaliert auf `uint32`,
> und verteilt die Werte gut ueber den Hash-Raum — vermeidet Kollisionen
> bei den kleinen positiven und negativen Ganzzahlen typischer Chunk-Koordinaten.

### Was du nach Schritt 8 siehst

Du kannst jetzt durch die Welt fliegen. Neue Chunks erscheinen am Horizont,
alte verschwinden hinter dir.

Achte auf die Chunk-Grenzen — wenn das Chunking korrekt implementiert ist,
sind sie **unsichtbar**. Wenn du Kanten siehst, liegt das an falschen Weltkoordinaten
im Noise-Aufruf.

---

## Zusammenfassung — Was du gelernt hast

| Konzept | Warum es wichtig ist |
|---|---|
| `Y = f(X, Z)` | Das Grundprinzip: Terrain ist Geometrie plus Funktion |
| Normals aus Gradienten | Beleuchtung braucht korrekte Flaechenrichtungen |
| `glBufferSubData` | VBOs aktualisieren ohne neuen GPU-Speicher zu allozieren |
| Perlin Noise | Raeumlich kohaerenter Zufall — das Gegenteil von `rand()` |
| Permutationstabelle | Wie Noise deterministisch aus einem Seed erzeugt wird |
| Fade-Funktion | Smooth-Step statt linearer Interpolation — keine Grate |
| fBm / Oktaven | Mehrere Scales gleichzeitig — Realismus durch Selbst-Aehnlichkeit |
| Height Remapping | Nicht-linearer Charakter: Taeler vs. Bergspitzen |
| Chunk-Koordinaten | Weltkoordinaten vs. lokale Koordinaten |
| Nahtlose Uebergaenge | Weltkoordinate als Noise-Input, nicht Chunk-lokale Position |

### Der typische Workflow beim Terrain-Tweaking

```
1. "New Seed" Button druecken
         |
2. Octaves und Frequency grob einstellen (Gesamtbild)
         |
3. Persistence tweaken (Rauheit)
         |
4. Height Remap anpassen (Charakter: Inseln? Berge? Ebenen?)
         |
5. Meeresspiegel einstellen
         |
6. Fertig: eine einzigartige Welt
```

### Weiter von hier

Wenn du alles aus diesem Tutorial implementiert hast, hier natuerliche naechste Schritte:

- **Erosion simulieren:** Wasser schleift Taeler, traegt Material ab — realistischere Topographie
- **Biome:** zweiter Noise-Wert fuer Temperatur und Feuchtigkeit bestimmt Vegetation und Farbe
- **LOD (Level of Detail):** nahe Chunks haben mehr Vertices als weite Chunks
- **Lego/Voxel-Terrain:** Hoehe auf ganzzahlige Brick-Stufen runden, dann mit Instanced Rendering rendern

```
Lego-Verbindung:
fBm-Noise -> float h (0..1)
-> brickLevel = (int)(h * maxBricks)
-> fuer Level 0..brickLevel: eine Brick-Instanz bei (x, level, z)
-> alle Instanzen per glDrawArraysInstanced rendern (Tutorial 01)
```
