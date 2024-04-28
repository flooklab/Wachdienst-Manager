# Wachdienst-Manager

Wachdienst-Manager ist ein Programm zum Erstellen von Wachberichten für den Wasserrettungsdienst der DLRG.
Die erstellten Wachberichte können sowohl in einem programmeigenen Dateiformat gespeichert als auch zum Drucken
als PDF-Datei exportiert werden. Um die Exportfunktion nutzen zu können, muss zusätzlich zum Wachdienst-Manager
eine LaTeX-Installation (wie zum Beispiel [TeXLive](https://www.tug.org/texlive)) vorhanden sein.  

Neben generellen Informationen zum jeweiligen Diensttag und dem Wachpersonal, das aus einer programminternen Datenbank
ausgewählt oder manuell als externes Personal hinzugefügt werden kann, können Angaben zum Wetter, zur Hilfeleistungsstatistik
und zu eingesetzten Einsatzfahrzeugen eingetragen werden. Auch können dem Wachbericht angehängte Anlagen sowie eine
gegebenenfalls von der Leitstelle vergebene Einsatznummer eingetragen werden. Wichtige Dateien, wie zum Beispiel eine
Einsatzprotokoll-Vorlage, können in der Benutzeroberfläche verlinkt und direkt von dort aus geöffnet werden.  

Jeder Person des Wachpersonals wird eine Funktion, wie zum Beispiel "Bootsführer", zugewiesen, die je nach den für
die Person in der Datenbank eingetragenen Qualifikationen entsprechend ausgewählt werden kann. Die für jede Person
eingetragenen Dienstzeiten werden automatisch zusammengerechnet und mit dem Übertrag des letzten Wachberichtes zu
einem Gesamtwert verrechnet. Der Übertrag kann automatisch aus dem letzten Wachbericht errechnet und eingetragen
werden. Diese Übertragsfunktionalität setzt entsprechend auch automatisch die laufende Nummer des Wachberichts.  

Zusätzlich zu dem eigentlichen Wachbericht wird ein Eintrag für das Bootstagebuch generiert. Dazu können in derselben
Benutzeroberfläche die am Diensttag getätigten Bootsfahrten und weitere, allgemeine Daten zum Bootstagebuch wie unter
anderem der Betriebsstundenzählerstand und die Menge an getanktem Treibstoff eingetragen werden. Für die Bootsbesatzung
der Fahrten können jeweils die als Wachpersonal eingetragenen Personen ausgewählt werden. Diesen wird für die Dauer
der Fahrt eine Bootsgast-Funktion zugewiesen, die wiederum entsprechend der vorhandenen Qualifikationen ausgewählt
werden kann. Analog zu den Dienstzeiten des Wachpersonals werden auch hier die gefahrenen Bootsstunden automatisch
zusammengerechnet und mit dem Übertrag verrechnet. Auch dieser Übertrag kann, genauso wie der Startwert
des Betriebsstundenzählers, mit der zuvor genannten Übertragsfunktionalität geladen werden.

## Handbuch

Ein aktuelles Handbuch zur Installation, Konfiguration und Bedienung des Wachdienst-Managers wird jedem neuen Release angehängt.

## Wachbericht-Offline-Vorlage

Zur Sicherheit existiert eine "Offline-Vorlage" eines leeren Wachberichts, welche vorsorglich ausgedruckt und dann notfalls
manuell ausgefüllt werden kann. Das Format entspricht den aus dem Programm exportierten PDF-Dateien. Die Vorlage wird jedem
neuen Release als PDF-Datei angehängt. Sie kann aber mittels LaTeX auch selbst erstellt werden (siehe das Verzeichnis
".\misc\report-offline-template"), um analog zum Programm auch hier ein eigenes Logo verwenden zu können.

## Erstellen des Wachdienst-Managers (Windows)

- Voraussetzungen:
  - Benötigte Software: [Qt 6](https://www.qt.io/product/qt6), [CMake](https://cmake.org/).

- Vorbereitung:
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
  - `...\Path\To\...\Qt\Tools\QtInstallerFramework\...\bin\binarycreator.exe --offline-only -t ...\Path\To\...\Qt\Tools\QtInstallerFramework\...\bin\installerbase.exe -p installer\packages -c installer\config\config.xml Wachdienst-Manager-1.5.0_Setup.exe` ausführen (ggf. Version anpassen)

- *Optionale* Quellcode-Dokumentation (benötigt [Doxygen](https://github.com/doxygen/doxygen)):
  - In Verzeichnis ".\doc" wechseln
  - `doxygen` ausführen

## Lizenzhinweise

Copyright (C) 2021–2024 M. Frohne  

Wachdienst-Manager ist freie Software: Sie können ihn unter den Bedingungen der
von der Free Software Foundation veröffentlichten GNU Affero General Public License,
Version 3 oder (nach Ihrer Wahl) jeder neueren Version der Lizenz,
weiter verteilen und/oder modifizieren.  

Wachdienst-Manager wird in der Hoffnung, dass er nützlich sein wird,
aber OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
Siehe die GNU Affero General Public License für weitere Einzelheiten.  

Sie sollten eine Kopie der GNU Affero General Public License zusammen mit diesem
Programm erhalten haben. Falls nicht, siehe https://www.gnu.org/licenses/.

## Markenhinweis

Der im Programm gezeigte DLRG-Schriftzug ist eine Marke der Deutsche Lebens-Rettungs-Gesellschaft e. V.
