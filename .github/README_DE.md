<a name="top"></a>
<div align="center">

<img src="../resources/images/piStudio_wtext.svg#gh-dark-mode-only" alt="piStudio Logo" width="266">
<img src="../resources/images/piStudio_text.svg#gh-light-mode-only" alt="piStudio Logo" width="266">
<br><br>

[![Version](https://img.shields.io/badge/version-0.7.3-blue.svg)](https://github.com/Kletternaut/piStudio/releases)
[![Qt](https://img.shields.io/badge/Qt-5.15+-green.svg)](https://www.qt.io/)
[![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi-red.svg)](https://www.raspberrypi.com/)
[![Stars](https://img.shields.io/github/stars/Kletternaut/piStudio.svg)](https://github.com/Kletternaut/piStudio/stargazers)
[![EN](https://img.shields.io/badge/lang-EN-blue)](README.md)

</div>

**piStudio** ist eine native Qt5/C++ Desktop-Steuerzentrale für die offizielle rpicam-apps Suite. Sie ist zu 100 % parameterkompatibel mit rpicam-apps, führt die Apps im Hintergrund aus, generiert Profildateien und bietet eine intuitive Tab-Oberfläche mit Echtzeit-Vorschau, erweiterten Kamera-Einstellungen, GStreamer-Streaming, KI-Objekterkennung und intelligenter Konfigurationsverwaltung. Seit v0.6.9 steuert sie außerdem die Runtime-Control-Schnittstelle der rpicam-apps (`rpicam-rt`-Fähigkeit, Kletternaut/rpicam-apps-Fork, Branch `feature/rt-roi`) für Live-Parameter-Updates an.

> **Unstable-Version:** piStudio befindet sich in der Entwicklung (vor Version 1.0). Funktionen, APIs und Konfiguration können sich zwischen Versionen ändern. Getestet auf Raspberry Pi OS (Bookworm / Trixie) mit Wayland (wayfire / labwc) und X11.

> **Display-Manager / Wayland:** Je nach verwendetem Display-Manager bzw. Wayland-Compositor kann es zu Einschränkungen in Funktion und Bedienbarkeit kommen. Diese sind in den Unzulänglichkeiten des jeweiligen Display-Managers bzw. von Wayland selbst begründet (z. B. fehlende Unterstützung für absolute Fensterpositionierung, Fensterfokus oder Overlay-Fenster) und nicht in piStudio.

### Funktionen

- **organisierte Tabs** – General, Output, Video, Still, Image, Focus, Zoom, Audio, GStreamer, GST Viewer, Inference, Actions, Expert
- **Einklappbare UI** – Gruppen mit persistentem Zustand
- **Live-Streaming** – GStreamer RTSP/UDP-Streaming mit Hardware-Beschleunigung
- **KI-Erkennung** – Hailo/YOLO Objekterkennung mit automatisierten Aktionen
- **Tools-Tab** – Bild-zu-Video-Konverter mit ffmpeg-Integration
- **Erweiterte Steuerung** – V4L2-Autofokus, ROI-Zoom, HDR, Segmentierung, Circular Buffer
- **Netzwerk-Monitoring** – Multi-Tab Stream-Viewer für Remote-Kameras
- **i18n** – Vollständige Deutsch-/Englisch-Unterstützung mit Live-Umschaltung

[![Screenshot](../resources/images/piStudio_ss2_small.png)](../resources/images/piStudio_ss2.png)

### Bekannte Einschränkungen

**Autofokus-Objektive:** Originale Raspberry Pi Autofokus-Objektive werden derzeit nicht unterstützt. Wir suchen einen Sponsor, der die benötigte Hardware für Entwicklung und Tests bereitstellt. Bei Interesse bitte über [GitHub Issues](https://github.com/Kletternaut/piStudio/issues) melden.

**KI-Objekterkennung:** Für die KI-gestützte Objekterkennung wird eine Hailo-8L, Hailo-8 oder Hailo-10 NPU (Neural Processing Unit) benötigt. Ohne NPU sind die Inference- und Actions-Tabs nicht funktionsfähig.

### Schnellstart

**Option A — .deb-Paket installieren (einfachste Methode):**
```bash
wget -P /tmp https://github.com/Kletternaut/piStudio/releases/download/v0.7.3/piStudio_0.7.3_arm64.deb
sudo apt install /tmp/piStudio_0.7.3_arm64.deb
```

> Einige Funktionen benötigen zusätzliche Pakete (GStreamer, ffmpeg, v4l-utils, Audio-Tools).  
> Siehe [Optionale Laufzeit-Abhängigkeiten](../resources/docs/INSTALLATION.md#optional-runtime-dependencies) für Details.

Alle optionalen Abhängigkeiten auf einmal installieren:
```bash
sudo apt install -y gstreamer1.0-tools gstreamer1.0-plugins-ugly \
                    v4l-utils ffmpeg pulseaudio-utils alsa-utils
```

**Option B — Direkt bauen und starten:**
```bash
git clone https://github.com/Kletternaut/piStudio.git
cd piStudio
mkdir build && cd build
cmake ..
make -j$(nproc)
./piStudio
```

**Option C — Systemweit installieren:**
```bash
git clone https://github.com/Kletternaut/piStudio.git
cd piStudio
resources/scripts/install.sh
```

### Dokumentation

- [Installationsanleitung](../resources/docs/INSTALLATION.md) – Vollständige Build- und Einrichtungsanleitung
- [Parameter-Referenz](../resources/docs/PARAMETERS.md) – rpicam-apps Parameter mit Links zur offiziellen Raspberry Pi Dokumentation
- [Changelog](../resources/docs/CHANGELOG.md) – Versionshistorie

### Support

- [Issue Tracker](https://github.com/Kletternaut/piStudio/issues)
- [Diskussionen](https://github.com/Kletternaut/piStudio/discussions)

### KI-Hinweis

> **Entwickelt mit KI-Unterstützung**  
> Dieses Projekt wurde unter Einsatz von KI-gestützten Entwicklungswerkzeugen (GitHub Copilot) erstellt.  
> Sämtlicher Code wurde vom Autor geprüft, getestet und integriert.  
> KI-Werkzeuge dienten der Entwicklungsbeschleunigung – kreative Entscheidungen, Architektur und Verantwortung für die Software liegen ausschließlich beim Autor.

---

## Haftungsausschluss

Diese Software wird **„so wie sie ist“** bereitgestellt, ohne jegliche Gewährleistung. Die Benutzung erfolgt auf eigene Gefahr. Der Autor übernimmt keine Haftung für Schäden, Datenverluste, Hardware-Probleme oder sonstige Folgen — siehe [LICENSE.md](../LICENSE.md) für die vollständigen Bedingungen.

Diese Software steht unter der **PolyForm Noncommercial License 1.0.0**. Nichtkommerzielle Nutzung ist kostenlos gestattet. Für kommerzielle Nutzung ist eine separate Lizenz des Rechteinhabers erforderlich. Siehe [LICENSE.md](../LICENSE.md) für den vollständigen Lizenztext.

piStudio ist ein unabhängiges Open-Source-Projekt und steht in keiner Verbindung zu Raspberry Pi Ltd. Es steuert die offiziellen rpicam-apps-Werkzeuge funktional an (100 % Parameterkompatibilität), ohne Teil davon zu sein. „Raspberry Pi" ist ein eingetragenes Markenzeichen von Raspberry Pi Ltd.

---

## Projektstatistik

![Code Statistics](https://img.shields.io/badge/lines%20of%20code-18.6k-brightgreen.svg)
![Files](https://img.shields.io/badge/source%20files-80-blue.svg)
![Contributors](https://img.shields.io/github/contributors/Kletternaut/piStudio.svg)
![Last Commit](https://img.shields.io/github/last-commit/Kletternaut/piStudio.svg)

---

<div align="right"><a href="#top"><img src="https://img.shields.io/badge/%E2%96%B4_top-grey?style=flat-square" alt="back to top"></a></div>
<div align="center">

**Made for the Raspberry Pi Community**

[Star this project](https://github.com/Kletternaut/piStudio) • [Fork it](https://github.com/Kletternaut/piStudio/fork)

</div>
