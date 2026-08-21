# UDP Object Detection Receiver

## Übersicht

Dieses Modul implementiert einen UDP-basierten Objekterkennungs-Receiver für piStudio. Es besteht aus einem eigenständigen C-Programm, das UDP-Pakete empfängt und diese über einen neuen "Inference"-Tab in der GUI anzeigt.

## Komponenten

### 1. C-Programm: `udp_object_detection.c`

Ein leichtgewichtiges C-Programm, das:
- UDP-Socket auf Port 12347 (konfigurierbar) öffnet
- Objekterkennungs-Datenpakete von `object_detect_udp_stage.cpp` empfängt
- Detektionen in strukturiertem Format ausgibt
- Für Integration in die GUI optimiert ist

**Paketformat:**
```
Delimiter (4 bytes): 0xDDCCBBAA (little-endian)
X-Position (4 bytes): int32_t
Y-Position (4 bytes): int32_t
Breite (4 bytes): int32_t
Höhe (4 bytes): int32_t
Namenslänge (1 byte): uint8_t
Objektname (variable): UTF-8 String
Konfidenz (4 bytes): float
```

**Ausgabeformat:**
```
[2025-11-03 14:32:45] DETECTION: person (87.50%) at [120,340] size [200x450]
```

### 2. GUI-Integration: Inference-Tab

Der neue Tab bietet:
- **Steuerung**: Start/Stop-Buttons für den Receiver
- **Port-Konfiguration**: Anpassbarer UDP-Port (Standard: 12347)
- **Detektionsliste**: Zeigt die letzten 100 erkannten Objekte
- **Filterung**: "Report only changes" Option zeigt nur neue Objekttypen

## Verwendung

### Im Terminal (Standalone)

```bash
# Standard-Port 12347
./udp_object_detection

# Benutzerdefinierten Port
./udp_object_detection 8888
```

### In der GUI

1. Öffne den **Inference**-Tab
2. Optional: Passe den UDP-Port an (Standard: 12347)
3. Optional: Aktiviere "Report only changes" um wiederholte Erkennungen zu filtern
4. Klicke auf **Start Receiver**
5. Die Detektionen erscheinen automatisch in der Liste
6. Klicke auf **Stop Receiver** zum Beenden

### Test mit Python-Skript

```bash
# Sende Test-Detektionen
./resources/utils/test_udp_detection.py --count 20 --interval 0.5
```

### Test mit netcat

```bash
# Testnachricht senden (Debugging)
echo "Test" | nc -u localhost 12347
```

## Technische Details

### Architektur

- **Prozess-Isolation**: Der C-Receiver läuft als separater Prozess
- **IPC**: Kommunikation über stdout/stderr mit QProcess
- **Threading**: Nicht blockierend durch QProcess-Event-Loop
- **Ressourcen**: Minimaler Speicher-Footprint (~100 KB)

### Fehlerbehandlung

- Validierung der Paket-Delimiter
- Größen-Checks für Puffer-Overflows
- Timeout-Behandlung für Socket-Operationen
- Automatisches Cleanup bei GUI-Schließung

### Performance

- **Latenz**: < 1ms pro Paket
- **Durchsatz**: > 1000 Pakete/Sekunde
- **CPU-Last**: < 1% bei typischer Nutzung

### Filteroptionen

- **Report only changes**: Zeigt nur Erkennungen wenn sich die Objektklasse ändert
- Nützlich zur Rauschreduzierung bei kontinuierlicher Erkennung desselben Objekts
- Letztes erkanntes Objekt wird gespeichert und verglichen

## Build-Integration

Das Programm wird automatisch über CMake gebaut:

```cmake
add_executable(udp_object_detection src/inference/udp_object_detection.c)
install(TARGETS udp_object_detection DESTINATION bin)
```

## Erweiterungen

Mögliche zukünftige Features:
- Visualisierung der Bounding Boxes im Video-Preview
- Filterung nach Objektklassen
- Export der Detektionen als CSV/JSON
- Statistiken (Objekte pro Sekunde, Durchschnittskonfidenzen)
- Multi-Port-Unterstützung für mehrere Quellen

## Lizenz

PolyForm Noncommercial License 1.0.0 (siehe LICENSE.md)
