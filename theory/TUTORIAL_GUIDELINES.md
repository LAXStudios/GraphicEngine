# Tutorial-Richtlinien

Dieses Dokument beschreibt den Stil und die Prinzipien der Tutorials in diesem Ordner.
Es richtet sich an KIs die neue Tutorials schreiben oder bestehende erweitern.

---

## Grundprinzip

Der Lernende soll **durch eigenes Denken** zum Ziel kommen, nicht durch Abschreiben.
Jede Erklaerung hat zwei Ebenen:

1. **Das Warum** — Warum brauchen wir das ueberhaupt? Was ist das Problem?
2. **Das Wie** — Erst danach kommt die Loesung oder Technik.

Loesungen werden nie einfach hingeworfen. Sie sind entweder eingeklappt (Obsidian-Callout)
oder kommen erst nach gezielten Denkaufgaben.

---

## Sprache und Formatierung

- **Sprache:** Deutsch
- **Keine Emojis** — weder in Ueberschriften noch im Fliesstext
- **ASCII-Art** fuer Diagramme, keine Unicode-Sonderzeichen wie Pfeile oder Symbole
- **Callouts statt HTML:** Obsidian-Callouts verwenden, keine `<details>`/`<summary>`-Tags

### Obsidian-Callout-Typen

| Callout | Verwendung |
|---|---|
| `> [!abstract]` | Lernziel ganz oben im Tutorial — immer offen |
| `> [!note]` | Wichtige Erklaerung die man lesen soll — immer offen |
| `> [!question]-` | Aufloesung einer "Denk selbst nach"-Frage — eingeklappt |
| `> [!tip]-` | Gestuftter Hint wenn man nicht weiterkommt — eingeklappt |
| `> [!example]-` | Vollstaendige Referenzloesung — eingeklappt |

Das `-` nach dem Typ macht den Callout **eingeklappt** beim Oeffnen in Obsidian.
Callouts ohne `-` sind standardmaessig offen.

Syntax:
```
> [!tip]- Titel des Hints
> Inhalt der ersten Zeile
>
> Weitere Zeilen mit vorangestelltem >
> ```cpp
> // Code-Beispiele innerhalb eines Callouts
> ```
```

---

## Struktur eines Tutorials

### Anfang

Jedes Tutorial beginnt mit:

1. Einem `[!abstract]`-Callout mit dem Lernziel
2. Einem "Wo wir stehen"-Abschnitt: Was ist schon im Projekt vorhanden, was fehlt noch
3. Einem Ueberblick aller Schritte als einfacher Textblock (kein Inhaltsverzeichnis mit Links)

### Struktur jedes Schritts

Jeder Schritt folgt diesem Muster:

```
### Was werden wir bauen?
Ein Satz: das konkrete sichtbare Ziel dieses Schritts.

### Warum ist das wichtig?
Das Problem erklaeren das dieser Schritt loest.
Erst hier versteht der Lernende warum er weiterliest.

### Denk selbst nach
Eine konkrete Frage oder ein kleines Experiment.
Der Lernende soll ERST selbst denken, dann den Callout aufklappen.

> [!question]- Aufloesung
> Die Antwort auf die obige Frage.

### Aufgabe X.Y — Kurzer Titel
Was konkret implementiert werden soll.
Keine vollstaendige Loesung — nur die Signatur oder das Geruest.

> [!tip]- Hint (erst selbst versuchen)
> Ein einzelner gezielter Hinweis. Nicht die Loesung.

> [!example]- Referenzloesung
> Die vollstaendige Loesung. Nur aufklappen wenn man wirklich feststeckt.

### Was du nach Schritt N siehst
Ein konkretes sichtbares Ergebnis beschreiben.
Das ist das Erfolgserlebnis — jeder Schritt endet damit.
```

### Ende

Jedes Tutorial endet mit:

1. Einer Zusammenfassungstabelle (Konzept / Warum es wichtig ist)
2. Einem typischen Workflow als Schritt-fuer-Schritt-Textblock
3. "Weiter von hier" mit naechsten moeglichen Schritten, kurz beschrieben

---

## Hints stufen, nicht auf einmal geben

Wenn ein Thema schwierig ist, werden Hints **gestaffelt** angeboten — nicht alles auf einmal:

```
> [!tip]- Hint 1: Wo anfangen
> Nur der erste Schritt.

> [!tip]- Hint 2: Wenn Hint 1 sitzt
> Der naechste Schritt. Erst oeffnen wenn Hint 1 verstanden.

> [!example]- Referenzloesung
> Erst aufklappen wenn beide Hints nicht gereicht haben.
```

Der Lernende soll zwischen den Hints jeweils selbst versuchen.

---

## Denkaufgaben formulieren

Eine gute "Denk selbst nach"-Frage:

- Ist **konkret** und **beantwortbar** ohne zusaetzliches Wissen
- Laesst sich durch ein Mini-Experiment im Code pruefen
- Hat eine klare richtige Antwort (keine Meinungsfrage)
- Ist nah genug an der Aufgabe dass die Antwort direkt weiterhilft

Schlechtes Beispiel:
> "Was weisst du ueber Vektoren?"

Gutes Beispiel:
> "Was gibt `z * width + x` zurueck wenn `x=0` und `z=0` sind? Und wenn `x=1, z=0`?
> Stell dir ein Grid mit `width=3` vor und schreibe die Indizes auf."

---

## Was vermieden werden soll

- **Loesungen direkt im Fliesstext** — immer in eingeklappte Callouts
- **Zu viele Konzepte auf einmal** — ein Schritt, ein Konzept
- **Schritt ohne sichtbares Ergebnis** — jeder Schritt muss zu etwas Laufendem fuehren
- **"Hier ist der vollstaendige Code"** ohne vorherige Denkaufgabe
- **Erklaerung ohne Motivation** — nie "Hier ist Technik X" ohne "weil Y das Problem ist"
- **Emojis**

---

## Interaktive Unterstuetzung (Chat)

Wenn der Lernende beim Umsetzen eines Tutorials feststeckt und eine Frage stellt,
gilt fuer die Antwort:

- **Eine Frage zurueck stellen** statt direkt die Loesung geben
- **Den Lernenden zum richtigen Ort fuehren** ("Schau dir Zeile X an — was passiert dort?")
- **Konkrete kleine Beispiele** geben statt abstrakte Erklaerungen
- **Nur einen Schritt auf einmal** — nicht alle Probleme auf einmal erklaeren
- Wenn Code gezeigt wird: das Problem benennen (z.B. "out of bounds"), aber die Ursache
  selbst finden lassen
- Erst wenn der Lernende explizit nach der Loesung fragt, diese vollstaendig zeigen

---

## Bezug zum Projekt

Tutorials referenzieren immer die **echten Dateien im Projekt** (`TerrainMesh.cpp`,
`terrain.glsl`, etc.) statt abstrakte Beispieldateien.

Wenn ein Tutorial auf einem vorherigen aufbaut, wird das am Anfang explizit erwaehnt
und verlinkt — aber der Lernende soll nicht gezwungen werden alle anderen Tutorials
zuerst zu lesen.

---

## Checkliste vor dem Veroeffentlichen

- [ ] Jeder Schritt hat ein sichtbares Ergebnis am Ende
- [ ] Keine Loesung steht ungeschuetzt im Fliesstext
- [ ] Alle Callouts sind korrekt formatiert (mit `>` in jeder Zeile)
- [ ] Keine Emojis im gesamten Dokument
- [ ] ASCII-Art statt Unicode-Symbole in Diagrammen
- [ ] Jede Denkaufgabe hat eine Aufloesung im `[!question]-`-Callout
- [ ] Hints sind gestaffelt (erst kleiner Hint, dann groesserer, dann Loesung)
- [ ] Das Tutorial referenziert echte Projektdateien
