# Wachdienst-Manager

Wachdienst-Manager ist ein Verwaltungsprogramm für DLRG-Wachdienstberichte.

## Erstellen des Wachdienst-Managers (Windows)

- Voraussetzungen:
  - Benötigte Software: [Qt 6](https://www.qt.io/product/qt6), [CMake](https://cmake.org/).

- Vorbereitung:
  - DLRG-Logo (z.B. Logo aus https://de.wikipedia.org/wiki/Datei:DLRG_Logo.svg mit Inkscape exportieren) mit Größe {475px x 386px} unter ".\resources\images\dlrg-logo.png" speichern (**Urheberrechte beachten!**)
  - Eingabekonsole (`cmd.exe`) starten

- Build:
  - In Verzeichnis ".\build" wechseln
  - `cmake -DCMAKE_BUILD_TYPE=Release ..` ausführen
  - `make` ausführen

- Deploy:
  - "Wachdienst-Manager.exe" aus ".\build" nach ".\deploy" kopieren
  - In Verzeichnis ".\deploy" wechseln
  - `...\Path\To\...\Qt\...\mingw..._64\bin\windeployqt.exe Wachdienst-Manager.exe` ausführen
  - Ggf. Starten testen und ggf. benötigte Compiler-DLLs hinzufügen (jeweils von "...\Path\To\...\Qt\...\mingw..._64\bin" kopieren)

- Installer erstellen:
  - Anpassen von ".\installer\installer\config\config.xml": Feld `/Installer/Publisher` ersetzen, ggf. Feld `/Installer/Version` anpassen
  - Anpassen von ".\installer\installer\packages\org.dlrg.wachdienstmanager\meta\package.xml": Ggf. Feld `/Package/Version` und `/Package/ReleaseDate` anpassen
  - Inhalt von ".\deploy" nach ".\installer\installer\packages\org.dlrg.wachdienstmanager\data\Wachdienst-Manager" kopieren
  - ".\resources\icons\application-icon.ico" nach ".\installer\installer\packages\org.dlrg.wachdienstmanager\data\Wachdienst-Manager" kopieren
  - In Verzeichnis ".\installer" wechseln
  - `...\Path\To\...\Qt\Tools\QtInstallerFramework\...\bin\binarycreator.exe --offline-only -t ...\Path\To\...\Qt\Tools\QtInstallerFramework\...\bin\installerbase.exe -p installer\packages -c installer\config\config.xml Wachdienst-Manager-1.1.0_Setup.exe` ausführen (ggf. Version anpassen)

- *Optionale* Quellcode-Dokumentation (benötigt [Doxygen](https://github.com/doxygen/doxygen)):
  - In Verzeichnis ".\doc" wechseln
  - `doxygen` ausführen

## Handbuch

Ein aktuelles Handbuch zur Installation, Konfiguration und Bedienung des Wachdienst-Managers wird jedem neuen Release angehängt.

## Lizenzhinweise

Copyright (C) 2021 M. Frohne  
  
Wachdienst-Manager ist Freie Software: Sie können es unter den Bedingungen
der GNU Affero General Public License, wie von der Free Software Foundation,
Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
veröffentlichten Version, weiter verteilen und/oder modifizieren.  
  
Wachdienst-Manager wird in der Hoffnung, dass es nützlich sein wird, aber
OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
Siehe die GNU Affero General Public License für weitere Details.  
  
Sie sollten eine Kopie der GNU Affero General Public License zusammen mit diesem
Programm erhalten haben. Wenn nicht, siehe https://www.gnu.org/licenses/.
