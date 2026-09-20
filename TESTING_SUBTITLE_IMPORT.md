# Plan testowania automatycznego importu napisów

## Przygotowanie testów

### Wymagane pliki testowe

Utwórz następującą strukturę katalogów do testowania:

```
TestMedia/
├── test_video.mp4          (jakikolwiek plik video)
├── test_video.srt          (plik napisów)
├── test_video.ssa
├── test_video.ass
├── video_only.mkv          (bez powiązanych napisów)
├── subtitles_only.srt      (napisy bez wideo)
└── multiple_subtitles/
    ├── movie.mkv
    ├── movie.srt           (główne napisy)
    ├── movie.ssa           (alternatywne napisy)
    └── movie.ass
```

## Scenariusze testowe

### Test 1: Przeciągnięcie wideo + napisy razem
**Cel:** Weryfikacja, że przeciągnięte razem napisy są importowane automatycznie

**Kroki:**
1. Otwórz HandBrake
2. Przeciągnij `test_video.mp4` i `test_video.srt` razem do okna HandBrake
3. Czekaj na zakończenie skanowania

**Oczekiwany rezultat:**
- Film jest skanowany
- Po skanowaniu zakładka Subtitles pokazuje zaimportowany `test_video.srt`
- Status bar: "Scan completed"

---

### Test 2: Przeciągnięcie wideo + wiele napisów
**Cel:** Weryfikacja jednoczesnego importu wielu formatów napisów

**Kroki:**
1. Otwórz HandBrake
2. Przeciągnij `test_video.mp4`, `test_video.srt`, `test_video.ssa`, `test_video.ass` razem
3. Czekaj na zakończenie skanowania

**Oczekiwany rezultat:**
- Wszystkie trzy formaty są importowane
- SubtitlesView zawiera 3 ścieżki napisów

---

### Test 3: Zaznaczenie w dialogu pliku (File Dialog)
**Cel:** Weryfikacja, że napisy zaznaczone w dialogu są importowane

**Kroki:**
1. Otwórz HandBrake
2. Kliknij "File" → "Open Source" (lub podobnie)
3. Zaznacz `test_video.mp4` i `test_video.srt` (hold Ctrl)
4. Kliknij "Open"
5. Czekaj na zakończenie skanowania

**Oczekiwany rezultat:**
- Film skanowany, napisy importowane automatycznie

---

### Test 4: Automatyczne odkrycie napisów (Sidecar)
**Cel:** Weryfikacja wyszukiwania napisów o tej samej nazwie

**Kroki:**
1. Przywołaj plik: `C:\TestMedia\multiple_subtitles\movie.mkv`
2. W HandBrake otwórz ten plik (bez przeciągania napisów)
3. Czekaj na zakończenie skanowania

**Oczekiwany rezultat:**
- Film skanowany
- Wszystkie trzy formaty (`movie.srt`, `movie.ssa`, `movie.ass`) znajdują się w tym samym katalogu
- Po skanowaniu SubtitlesView automatycznie wyświetla wszystkie znalezione napisy

---

### Test 5: Przeciągnięcie samych napisów (bez wideo)
**Cel:** Weryfikacja, że napisy można dodawać osobno

**Kroki:**
1. Otwórz HandBrake
2. Przeciągnij `subtitles_only.srt`
3. Powinna się pojawić zakładka Subtitles

**Oczekiwany rezultat:**
- Aplikacja przełącza się na zakładkę Subtitles
- Napisy są importowane do bieżącego zadania
- Jeśli nie ma aktualnie otwartego wideo, napisy czekają

---

### Test 6: Anulowanie skanowania
**Cel:** Weryfikacja, że anulowanie skanowania nie importuje napisów

**Kroki:**
1. Otwórz HandBrake
2. Przeciągnij `test_video.mp4` i `test_video.srt`
3. Zaraz po rozpoczęciu skanowania kliknij przycisk "Cancel"

**Oczekiwany rezultat:**
- Skanowanie anulowane
- Napisy **nie są** importowane
- Zakładka Subtitles pozostaje pusta lub z poprzednimi danymi

---

### Test 7: Błąd skanowania
**Cel:** Weryfikacja, że błąd skanowania nie importuje napisów

**Kroki:**
1. Otwórz HandBrake
2. Przeciągnij plik, który nie jest obsługiwanym formatem wideo, razem z napisami
3. Obserwuj błąd skanowania

**Oczekiwany rezultat:**
- Skanowanie się nie powiedzie
- Napisy **nie są** importowane
- Status bar pokazuje błąd

---

### Test 8: Wiele wideo + napisy
**Cel:** Weryfikacja obsługi wielu plików

**Kroki:**
1. Otwórz HandBrake
2. Przeciągnij `test_video.mp4`, `test_video.srt`, `video_only.mkv` razem

**Oczekiwany rezultat:**
- Obie wideo są skanowane
- Napisy importowane do `test_video` (pierwsze wideo w kolejności)
- `video_only.mkv` skanuje się bez automatycznych napisów

---

## Kryteria akceptacji

✅ **MUSI być spełnione:**
1. Napisy przesłane razem z wideo są importowane automatycznie
2. Wyszukiwanie napisów o tej samej nazwie funkcjonuje
3. Anulowanie skanowania nie powoduje importu napisów
4. Błąd skanowania nie powoduje importu napisów
5. Build przechodzi bez błędów
6. Brak regresji istniejącej funkcjonalności

✅ **POWINNO być spełnione:**
1. Obsługiwane formaty: `.srt`, `.ssa`, `.ass`
2. Case-insensitive sprawdzanie rozszerzeń
3. Obsługa ścieżek z polskimi znakami

⚠️ **OPCJONALNIE:**
1. Dodanie ustawienia do wyłączenia auto-importu
2. Powiadomienie użytkownika o zaimportowanych napisach
3. Obsługa napisów w podkatalogach

---

## Logowanie

Po każdym teście zanotuj:
- ✅ PASS / ❌ FAIL
- Obserwacje i nieoczekiwane zachowania
- Wersja HandBrake (ze statusu)
- OS (Windows 10/11, wersja)

Przykład:
```
Test 1: Przeciągnięcie wideo + napisy razem
Status: ✅ PASS
OS: Windows 11 Build 22621
HandBrake: [wersja z build output]
Notatka: Napisy poprawnie importowane, zakładka Subtitles wyświetla ścieżkę
```
