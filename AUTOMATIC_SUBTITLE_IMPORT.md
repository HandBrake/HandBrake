# Automatyczny Import Napisów

## Opis funkcjonalności

HandBrake WPF teraz obsługuje **automatyczny import napisów** podczas dodawania filmów do konwersji. Napisów nie trzeba już dodawać ręcznie — aplikacja automatycznie wykrywa i importuje pasujące pliki napisów.

## Jak to działa

### 1. **Import napisów ze wspólnym zaznaczeniem**
Gdy przeciągasz lub wybierasz film razem z plikami napisów (`.srt`, `.ssa`, `.ass`):
- Aplikacja separuje pliki wideo od plików napisów
- Skanuje wideo
- Po zakończeniu skanowania automatycznie importuje zaznaczone napisy do zakładki Subtitles

**Obsługiwane formaty:** `.srt`, `.ssa`, `.ass`

### 2. **Automatyczne wyszukiwanie napisów (Sidecar)**
Gdy dodajesz film, HandBrake automatycznie wyszukuje napisy w tym samym katalogu o tej samej nazwie:

```
Katalog:
├── film.mkv
├── film.srt      ← automatycznie importowany
├── film.ssa      ← automatycznie importowany
└── film.ass      ← automatycznie importowany
```

Jeśli takie pliki znajdują się w katalogu źródła, są **automatycznie importowane** bez dodatkowych działań użytkownika.

## Przypadki użycia

### Przypadek 1: Przeciągnąć film i napisy razem
```
Akcja: Przeciągnij następujące pliki do okna HandBrake:
- movie.mp4
- movie.srt
- movie.ssa

Wynik: Film jest skanowany, a oba pliki napisów są automatycznie importowane
```

### Przypadek 2: Wybrać film i napisy z dialogu
```
Akcja: Otwórz dialog "FileScan" i zaznacz:
- folder/movie.mkv
- folder/subtitles.srt

Wynik: Film skanowany, napisy importowane automatycznie
```

### Przypadek 3: Film z napisami o tej samej nazwie
```
Katalog:
movie/
├── action-film.mkv
└── action-film.srt

Akcja: Otwórz film "action-film.mkv" w HandBrake
Wynik: action-film.srt jest automatycznie importowany
```

## Implementacja techniczna

### Zmiany w `MainViewModel.cs`

1. **Pole bufora napisów**
   ```csharp
   private string[] pendingSubtitleFiles;
   ```
   Przechowuje ścieżki napisów oczekujących na import po zakończeniu skanowania.

2. **Metoda weryfikacji typu pliku**
   ```csharp
   private static bool IsSubtitleFile(string fileName)
   ```
   Sprawdza, czy plik ma rozszerzenie `.srt`, `.ssa` lub `.ass` (case-insensitive).

3. **Metoda wyszukiwania napisów**
   ```csharp
   private string[] FindSubtitleFilesForSource(string sourcePath)
   ```
   Wyszukuje w katalogu źródła pliki napisów o tej samej nazwie co film.

4. **Modyfikacje procedury skanowania**
   - `FilesDropped()` - obsługuje przeciągnięcie filmów i napisów jednocześnie
   - `FileScan()` - obsługuje zaznaczenie filmów i napisów w dialogu
   - `ScanCompleted()` - po skanowaniu importuje oczekujące lub znalezione napisy

### Przepływ

```
Dodaj film → Separuj pliki wideo/napisy → Skan wideo
                                              ↓
                                    Skan zakończony
                                              ↓
                              Napisów w buforze? Tak → Import
                                           ↓ Nie
                              Szukaj napisów o tej samej nazwie
                                           ↓
                              Znaleziono? Tak → Import
                                           ↓ Nie
                              Koniec (bez napisów)
```

## Preferencje użytkownika

Automatyczny import napisów jest zawsze aktywny. Jeśli użytkownik nie chce automatycznie importowanych napisów, może je usunąć z zakładki Subtitles.

## Obsługiwane rozszerzenia

- `.srt` - SubRip
- `.ssa` - SubStation Alpha
- `.ass` - Advanced SubStation Alpha

## Notatki

- Import jest asynchroniczny — nie blokuje interfejsu użytkownika
- Jeśli skan zostanie anulowany, oczekujące napisy nie są importowane
- Jeśli skan się nie powiedzie, oczekujące napisy nie są importowane
- Wyszukiwanie napisów jest case-insensitive dla rozszerzeń plików
- Wyszukiwanie stosuje dokładne dopasowanie nazwy (bez wildcard)
