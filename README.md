# esp_dmx

Diese Bibliothek ermöglicht das Senden und Empfangen von ANSI-ESTA E1.11 DMX-512A und ANSI-ESTA E1.20 RDM mit einem Espressif ESP32. Sie bietet Kontrolle und Analyse der Paketkonfiguration und erlaubt synchrones oder asynchrones Lesen und Schreiben auf dem DMX-Bus über den gewünschten Hardware-UART-Port. Zusätzlich enthält sie Werkzeuge zur Fehlerprüfung und zur Extraktion von DMX-Paket-Metadaten für eine einfachere Fehlersuche.

## Inhalte

- [Bibliotheksinstallation](#library-installation)
  - [Arduino](#arduino)
  - [ESP-IDF](#esp-idf)
  - [PlatformIO](#platformio)
- [Schnellstartanleitung](#quick-start-guide)
- [Was ist DMX?](#what-is-dmx)
  - [Was ist RDM?](#what-is-rdm)
- [DMX-Grundlagen](#dmx-basics)
  - [Adressen und der Startcode](#addresses-and-the-start-code)
  - [Kanalbelegungen (Footprints)](#footprints)
  - [Universen](#universes)
- [RDM-Grundlagen](#rdm-basics)
  - [Eindeutige IDs](#unique-ids)
  - [Subgeräte](#sub-devices)
  - [Parameter](#parameters)
  - [Geräteerkennung](#discovery)
  - [Antworten](#responses)
- [DMX-Port konfigurieren](#configuring-the-dmx-port)
  - [Treiber installieren](#installing-the-driver)
  - [Kommunikations-Pins festlegen](#setting-communication-pins)
  - [Timing-Konfiguration](#timing-configuration)
- [DMX lesen und schreiben](#reading-and-writing-dmx)
  - [DMX lesen](#reading-dmx)
  - [DMX-Sniffer](#dmx-sniffer)
  - [DMX schreiben](#writing-dmx)
  - [DMX-Parameter](#dmx-parameters)
- [RDM lesen und schreiben](#reading-and-writing-rdm)
  - [RDM-Anfragen](#rdm-requests)
  - [Geräte entdecken](#discovering-devices)
  - [RDM-Responder](#rdm-responder)
- [Fehlerbehandlung](#error-handling)
  - [Timing-Makros](#timing-macros)
  - [DMX-Startcodes](#dmx-start-codes)
- [Weitere Hinweise](#additional-considerations)
  - [Flash-Nutzung oder Cache deaktivieren](#using-flash-or-disabling-cache)
  - [RS-485-Schaltung verdrahten](#wiring-an-rs-485-circuit)
  - [Hardware-Spezifikationen](#hardware-specifications)
- [Bauanleitung für kabelloses DMX](#wireless-dmx-build-guide)
  - [Hardware](#hardware)
  - [ESP32-Pins](#esp32-pins)
  - [MAX485-Modul-Pins](#max485-module-pins)
  - [XLR-3-Pinbelegung](#xlr-3-pinout)
  - [Senderbox — DMX IN zu Wireless](#sender-box--dmx-in-to-wireless)
  - [Empfängerbox — Wireless zu DMX OUT](#receiver-box--wireless-to-dmx-out)
  - [Abschluss, Entkopplung und Schutz](#termination-decoupling-and-protection)
  - [Voll funktionsfähiger Wireless-Code (Arduino)](#fully-functional-wireless-code-arduino)
  - [Hinweis zur Isolation](#isolation-note)
- [Aufgabenliste](#to-do)
- [Anhang](#appendix)
  - [Befehlsklassen](#command-classes)
  - [NACK-Grundcodes](#nack-reason-codes)
  - [Parameter-IDs](#parameter-ids)
  - [Produktkategorien](#product-categories)
  - [Antworttypen](#response-types)

<a id="library-installation"></a>
## Bibliotheksinstallation

### Arduino

Diese Bibliothek benötigt das Arduino-ESP32-Framework in Version 2.0.3 oder neuer. Um das richtige Framework zu installieren, folge den Espressif-Anleitungen auf der Arduino-ESP32-Dokumentationsseite [hier](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

Diese Bibliothek kann installiert werden, indem dieses Repository in deinen Ordner `Arduino/libaries` geklont wird oder indem nach `esp_dmx` im Arduino-IDE-Bibliotheksmanager gesucht wird. Binde die Bibliothek anschließend ein, indem du `#include "esp_dmx.h"` am Anfang deines Arduino-Sketches ergänzt.

### ESP-IDF

Diese Bibliothek benötigt ESP-IDF Version 4.4.1 oder neuer. Klone dieses Repository in den `components`-Ordner deines Projekts. Die Bibliothek kann eingebunden werden, indem `#include "esp_dmx.h"` am Anfang deiner `main.c` steht.

### PlatformIO

Diese Bibliothek ist mit der PlatformIO-IDE kompatibel. Suche die Bibliothek im PlatformIO-Bibliotheksregister und füge sie deinem Projekt hinzu. Eingebunden wird sie mit `#include "esp_dmx.h"` am Anfang deiner `main.c` oder `main.cpp`.

Diese Bibliothek enthält eine `Kconfig`-Datei zur Konfiguration von Build-Optionen auf dem ESP32. Bei Nutzung des ESP-IDF-Frameworks wird empfohlen, Bibliotheksordner in einen `components`-Ordner im Projektstamm zu verschieben, statt die von PlatformIO installierten Bibliotheken am Standardort zu belassen. Das ist nicht zwingend, kann aber zu einem leistungsfähigeren Treiber führen. Weitere Informationen unter [Flash-Nutzung oder Cache deaktivieren](#using-flash-or-disabling-cache).

<a id="quick-start-guide"></a>
## Schnellstartanleitung

Für den Einstieg rufe den folgenden Code in deiner `setup()`-Funktion auf, wenn du Arduino nutzt, oder in `app_main()` deiner `main.c`, wenn du ESP-IDF verwendest.

```c
const dmx_port_t dmx_num = DMX_NUM_1;

// First, use the default DMX configuration...
dmx_config_t config = DMX_CONFIG_DEFAULT;

// ...declare the driver's DMX personalities...
const int personality_count = 1;
dmx_personality_t personalities[] = {
  {1, "Default Personality"}
};

// ...install the DMX driver...
dmx_driver_install(dmx_num, &config, personalities, personality_count);

// ...and then set the communication pins!
const int tx_pin = 17;
const int rx_pin = 16;
const int rts_pin = 21;
dmx_set_pin(dmx_num, tx_pin, rx_pin, rts_pin);
```

Zum Schreiben von Daten auf den DMX-Bus stehen zwei Funktionen bereit. `dmx_write()` schreibt Daten in den DMX-Puffer und `dmx_send()` sendet sie auf den Bus. Mit `dmx_wait_sent()` wird der Task blockiert, bis der DMX-Bus wieder frei ist.

```c
uint8_t data[DMX_PACKET_SIZE] = {0};

while (true) {
  // Write to the packet and send it.
  dmx_write(dmx_num, data, DMX_PACKET_SIZE);
  dmx_send(dmx_num);
  
  // Do work here...

  // Block until the packet is finished sending.
  dmx_wait_sent(dmx_num, DMX_TIMEOUT_TICK);
}
```

Zum Lesen vom DMX-Bus stehen zwei weitere Funktionen bereit. `dmx_receive()` wartet, bis ein neues Paket empfangen wurde. `dmx_read()` liest die Daten aus dem Treiberpuffer in ein Array, damit sie verarbeitet werden können. Wenn RDM-Anfragen verarbeitet werden sollen, kann `rdm_send_response()` verwendet werden.

```c
dmx_packet_t packet;
while (true) {
  int size = dmx_receive(dmx_num, &packet, DMX_TIMEOUT_TICK);
  if (size > 0) {
    dmx_read(dmx_num, data, size);

    // Optionally handle RDM requests
    if (packet.is_rdm) {
      rdm_send_response(dmx_num);
    }

    // Process data here...
  }

  // Do other work here...

}
```

Das war's! Für detailliertere Informationen zur Funktionsweise dieser Bibliothek inklusive RDM-Details lies weiter.

<a id="what-is-dmx"></a>
## Was ist DMX?

DMX ist ein unidirektionales Kommunikationsprotokoll, das hauptsächlich in der Veranstaltungsbranche zur Steuerung von Licht- und Bühnentechnik verwendet wird. DMX wird als kontinuierlicher Paketstrom über halbduplexes RS-485-Signaling mit einem standardmäßigen UART-Port übertragen. DMX-Geräte werden typischerweise per XLR5 in Daisy-Chain-Konfiguration verbunden, wobei in Consumer-Produkten auch andere Steckverbinder wie XLR3 verbreitet sind.

Jedes DMX-Paket beginnt mit einem High-zu-Low-Übergang (Break), gefolgt von einem Low-zu-High-Übergang (Mark-After-Break) und anschließend einem 8-Bit-Byte. Dieses erste Byte heißt Startcode. Break, Mark-After-Break und Startcode bilden zusammen die Reset-Sequenz. Nach der Reset-Sequenz kann ein Paket mit bis zu 512 Datenbytes gesendet werden.

DMX hat sehr strenge Timing-Anforderungen, um Abwärtskompatibilität mit älterer Lichttechnik zu ermöglichen. Bildraten reichen von 1 fps bis ungefähr 830 fps. Ein typischer DMX-Controller sendet Pakete mit etwa 25 bis 44 fps. DMX-Empfänger und -Sender haben unterschiedliche Timing-Vorgaben, die sorgfältig eingehalten werden müssen, damit Befehle korrekt verarbeitet werden.

Heute stößt DMX bei den Anforderungen moderner Hardware oft an Grenzen. Die geringe Datenrate und kleine Paketgröße führen dazu, dass es gegenüber leistungsfähigeren Protokollen an Popularität verliert. Durch seine Einfachheit und Robustheit bleibt es jedoch oft die erste Wahl für kleinere Projekte.

Ausführliche Informationen zu DMX findest du im [E1.11-Standarddokument](https://tsp.esta.org/tsp/documents/docs/ANSI-ESTA_E1-11_2008R2018.pdf).

<a id="what-is-rdm"></a>
### Was ist RDM?

RDM steht für Remote Device Management. Es ist eine Erweiterung des DMX-Protokolls, die eine intelligente bidirektionale Kommunikation zwischen Geräten verschiedener Hersteller über einen angepassten DMX-Datenlink ermöglicht. RDM erlaubt einer Konsole oder einem anderen Steuergerät, Geräte im DMX-Netzwerk zu entdecken sowie zu konfigurieren, zu überwachen und zu verwalten.

Wenn RDM-fähige Geräte konfiguriert werden müssen, aber schlecht erreichbar sind, sagt man: RDM ist „schneller als die Leiter“. Statt auf eine Leiter steigen zu müssen, können Nutzer Geräteeinstellungen direkt vom DMX-Controller aus ändern.

Ausführliche Informationen zu RDM findest du im [E1.20-Standarddokument](https://getdlight.com/media/kunena/attachments/42/ANSI_E1-20_2010.pdf).

<a id="dmx-basics"></a>
## DMX-Grundlagen

In einer typischen Konfiguration besteht ein DMX-System aus einem DMX-Controller und bis zu 32 DMX-Geräten pro DMX-Port. Ein fünfpoliges XLR-Kabel (DMX-Kabel) wird vom DMX-Out des Controllers zum DMX-In des ersten Geräts verbunden. Jedes weitere Gerät wird per DMX-Kabel vom DMX-Out des vorherigen Geräts zum DMX-In des nächsten verbunden. Am DMX-Out des letzten Geräts kann ein DMX-Abschlusswiderstand angeschlossen werden. Das ist nur bei RDM erforderlich, kann aber auch bei mehr als 32 Geräten oder langen Kabelwegen zur Signalstabilität beitragen.

<a id="addresses-and-the-start-code"></a>
### Adressen und der Startcode

DMX-Adressen werden benötigt, damit DMX-Controller mit Geräten kommunizieren können. Die Adresse eines Geräts kann zwischen 1 und 512 (einschließlich) eingestellt werden, entweder über DIP-Schalter oder über das integrierte Display. DMX-Adressen entsprechen DMX-Slots im Paket. Adresse 1 entspricht Slot 1. Jeder DMX-Slot ist ein 8-Bit-Wert mit dem Bereich 0 bis 255. Um einen DMX-Dimmer auf Adresse 5 zu steuern, muss Slot 5 geschrieben werden. Für volle Intensität wird Slot 5 auf 255 gesetzt, für null Intensität auf 0. Werte dazwischen dimmen entsprechend.

Slot 0 im DMX-Paket wird DMX-Startcode genannt. Der Startcode teilt den DMX-Geräten mit, welcher Pakettyp gesendet wird. Standard-DMX-Pakete verwenden den Startcode `0x00`, auch Null-Startcode genannt. DMX-Geräte reagieren nicht auf DMX-Pakete, wenn das Paket nicht mit einem Null-Startcode beginnt. Eine Liste weiterer unterstützter Startcodes findest du im Abschnitt [DMX-Startcodes](#dmx-start-codes).

<a id="footprints"></a>
### Kanalbelegungen (Footprints)

Viele DMX-Geräte unterstützen mehrere steuerbare DMX-Parameter. Geräte mit mehreren Parametern verwenden mehrere DMX-Adressen. Ein RGB-LED-Gerät ist ein typisches Beispiel: Es hat die drei Parameter Rot, Grün und Blau. Entsprechend belegt es drei aufeinanderfolgende DMX-Adressen. Das nennt man den DMX-Footprint des Geräts. Bei einer Startadresse 5 werden Rot, Grün und Blau über die Slots 5, 6 und 7 gesteuert. Der Footprint eines Geräts steht im Handbuch. Es ist möglich, Geräte so zu adressieren, dass sich Footprints überlappen, üblich ist das jedoch nicht.

Geräte mit mehreren Parametern können mehrere Footprints unterstützen. Bei solchen Geräten ist immer nur ein Footprint gleichzeitig aktiv. Größere Footprints ermöglichen eine feinere Steuerung, kleinere Footprints reichen aus, wenn keine hohe Auflösung benötigt wird. Hinweise zum Umschalten des aktiven Footprints findest du im Handbuch des Geräts.

Ein Gerät mit mehreren Footprints besitzt mehrere Personalities. Jede DMX-Personality kann einen anderen Footprint unterstützen.

<a id="universes"></a>
### Universen

Wenn mehr als 512 DMX-Adressen verwendet werden, sind mehrere DMX-Ports nötig. Jeder DMX-Port wird DMX-Universum genannt. DMX-Geräte lassen sich eindeutig über Universum und Adresse identifizieren. Üblich ist die Schreibweise mit `/`, z. B. `3/475` für Universum 3, Adresse 475. Wichtig: DMX-Geräte kennen das Konzept „Universum“ nicht. Ein Gerät auf DMX-Adresse 1 reagiert auf Slot-1-Schreibvorgänge in dem Universum, an das es angeschlossen ist.

<a id="rdm-basics"></a>
## RDM-Grundlagen

Im Vergleich zu DMX ist RDM ein recht komplexes Protokoll, das je nach angeforderter Information unterschiedliche Paket- und Datentypen verwendet. Diese Bibliothek abstrahiert viele Details der RDM-Implementierung, um die Nutzung im Code zu vereinfachen und gleichzeitig einen leistungsfähigen Funktionsumfang zu bieten. Die folgenden Abschnitte geben eine Einführung in RDM im Kontext dieser Bibliothek. Für eine umfassendere Einführung siehe das [E1.20-Standarddokument](https://getdlight.com/media/kunena/attachments/42/ANSI_E1-20_2010.pdf).

<a id="unique-ids"></a>
### Eindeutige IDs

In einem RDM-Netzwerk gibt es ein Controller-Gerät und mehrere Responder-Geräte. Jedes Gerät besitzt eine Unique ID (UID), die es eindeutig im Netzwerk identifiziert. Hat ein RDM-Gerät mehrere DMX-Ports, kann es mehrere UIDs besitzen, eine pro DMX-Port. UIDs sind 48 Bit lang. Die oberen 16 Bit sind die Hersteller-ID, die unteren 32 Bit die Geräte-ID. Ein sinnvoller Vergleich sind MAC-Adressen in IP-Netzwerken: Jedes IP-fähige Gerät hat mindestens eine MAC-Adresse, bei mehreren Netzwerkschnittstellen auch mehrere. Ebenso kennzeichnen die oberen Bits einer MAC-Adresse den Hersteller der Schnittstelle.

UIDs werden textuell hexadezimal dargestellt, wobei Hersteller-ID und Geräte-ID durch `:` getrennt sind. Hat ein Gerät z. B. die Hersteller-ID `0xabcd` und die Geräte-ID `0x12345678`, lautet die vollständige UID `abcd:12345678`.

Wenn ein Controller eine RDM-Anfrage erstellt, muss sie mit einer Ziel-UID adressiert werden. Empfänger kann ein einzelnes Gerät (mit seiner UID) oder mehrere Geräte (per Broadcast-UID) sein. Broadcast-UIDs können an alle Geräte eines bestimmten Herstellers oder an alle Geräte im RDM-Netzwerk adressiert werden. Für einen Hersteller-Broadcast muss die Hersteller-ID der Ziel-UID dem gewünschten Hersteller entsprechen und die Geräte-ID `ffffffff` sein. Um an alle Geräte mit Hersteller-ID `05e0` zu senden, muss die UID `05e0:ffffffff` lauten. Für Broadcast an alle Geräte im RDM-Netzwerk wird `ffff:ffffffff` verwendet.

Die kleinste mögliche UID ist `0001:00000000`, die größte `ffff:fffffffe`. In der Praxis ist die Hersteller-ID `ffff` nicht zulässig, daher hätten reale RDM-Geräte nie eine UID größer als `7fff:fffffffe`. Diese Bibliothek repräsentiert die maximale UID mit der Konstante `RDM_UID_MAX`.

Organisationen können bei ESTA eine eindeutige Hersteller-ID beantragen. Anleitungen dazu und eine Liste registrierter Hersteller-IDs findest du [hier](https://tsp.esta.org/tsp/working_groups/CP/mfctrIDs.php). Diese Softwarebibliothek ist mit der Hersteller-ID `05e0` registriert. Nutzer dieser Bibliothek dürfen diese Hersteller-ID für ihre Geräte verwenden.

In dieser Bibliothek werden UIDs mit dem Typ `rdm_uid_t` dargestellt. Mit dem Makro `rdm_uid_broadcast_man()` kann eine UID für Broadcasts an eine gewünschte Hersteller-ID erzeugt werden, und mit `RDM_UID_BROADCAST_ALL` wird an alle Geräte im RDM-Netzwerk gesendet.

<a id="sub-devices"></a>
### Subgeräte

Jedes RDM-Gerät kann bis zu 512 Subgeräte unterstützen. Ein Beispiel ist ein Dimmerrack mit mehreren Dimmern. Anfragen können an einen bestimmten Dimmer adressiert werden, indem die UID des Racks verwendet und die entsprechende Subgeräte-Nummer angegeben wird.

Die Subgeräte-Nummer für das Root-Gerät ist `0x0000`. Mit der Subgeräte-Nummer `0xffff` kann eine Anfrage an alle Subgeräte eines Root-Geräts adressiert werden. Die Konstanten `RDM_SUB_DEVICE_ROOT` und `RDM_SUB_DEVICE_ALL` verbessern dabei die Lesbarkeit des Codes.

Ein Root-Gerät und seine Subgeräte können unterschiedliche RDM-Parameter unterstützen, aber alle Subgeräte innerhalb desselben Root-Geräts müssen untereinander denselben Parametersatz unterstützen.

<a id="parameters"></a>
### Parameter

RDM-Anfragen müssen Parameter lesen und aktualisieren können. Der RDM-Standard definiert mehr als 50 verschiedene Parameter IDs (PIDs), die ein Gerät unterstützen kann. Außerdem dürfen Hersteller eigene PIDs für ihre Geräte definieren.

Die meisten PIDs unterstützen GET oder SET, sofern das antwortende Gerät die angeforderte PID unterstützt. Manche PIDs unterstützen nur GET oder nur SET, manche beides. Drei PIDs unterstützen weder GET noch SET; sie werden für den RDM-Discovery-Algorithmus verwendet: `DISC_UNIQUE_BRANCH`, `DISC_MUTE` und `DISC_UN_MUTE`. Diese Bibliothek stellt für jede PID Konstanten bereit. Jede PID ist mit `RDM_PID_` präfixiert. Aus `DISC_UNIQUE_BRANCH` wird also `RDM_PID_DISC_UNIQUE_BRANCH`. In diesem Dokument werden PIDs konsistent mit diesem Präfix benannt.

RDM legt fest, dass jedes Gerät (nicht zwingend dessen Subgeräte) einen bestimmten Satz an PIDs unterstützen muss, damit Geräte korrekt miteinander kommunizieren können. Eine Liste unterstützter und erforderlicher PIDs findest du im [Anhang](#parameter-ids).

GET-Anfragen dürfen nicht an alle Subgeräte eines Root-Geräts gesendet werden. Daher ist eine GET-Anfrage an `RDM_SUB_DEVICE_ALL` nicht zulässig.

<a id="discovery"></a>
### Geräteerkennung

Bei RDM-Anfragen ist es üblicherweise sinnvoll (aber nicht zwingend), zuerst die UIDs der Geräte im RDM-Netzwerk zu ermitteln. Der Discovery-Prozess beginnt damit, dass der Controller den Befehl `RDM_PID_DISC_UNIQUE_BRANCH` an alle Geräte broadcastet. Die Nutzdaten dieser Anfrage definieren einen Adressraum über eine untere und obere UID-Grenze. Geräte antworten auf `RDM_PID_DISC_UNIQUE_BRANCH`, wenn ihre UID größer/gleich der unteren und kleiner/gleich der oberen Grenze ist. Antworten mehrere Geräte gleichzeitig, können Datenkollisionen entstehen. Dann teilt der Controller den Adressraum in zwei Bereiche und sendet je Bereich erneut `RDM_PID_DISC_UNIQUE_BRANCH`. Das wird wiederholt, bis in einem Bereich nur noch ein Gerät gefunden wird.

Wird in einem Adressraum ein einzelnes Gerät gefunden, wird an dieses Gerät `RDM_PID_DISC_MUTE` gesendet, damit es auf spätere `RDM_PID_DISC_UNIQUE_BRANCH`-Anfragen nicht mehr antwortet. Geräte mit mehreren RDM-Ports liefern bei Antworten auf `RDM_PID_DISC_MUTE` eine Binding-UID zurück, die ihre primäre UID repräsentiert.

Einige RDM-Geräte arbeiten als Proxy-Geräte. Ein Proxy ist ein Inline-Gerät, das als Vertreter für ein oder mehrere Geräte agiert. Es beantwortet Controller-Nachrichten im Namen der vertretenen Geräte so, als wäre es selbst das Gerät. Ob ein Gerät als Proxy arbeitet oder über ein anderes Gerät geproxyt wird, wird in Antworten auf `RDM_PID_DISC_MUTE` und `RDM_PID_DISC_UN_MUTE` signalisiert.

Die Geräteerkennung sollte regelmäßig durchgeführt werden, da Geräte aus dem RDM-Netzwerk entfernt oder neu hinzugefügt werden können. Bevor der Discovery-Algorithmus neu startet, sollte `RDM_PID_DISC_UN_MUTE` an alle Geräte gebroadcastet werden, um Änderungen erkennen zu können.

<a id="responses"></a>
### Antworten

Antwortende Geräte sollen nur auf Anfragen reagieren, wenn es sich nicht um Broadcast-Anfragen handelt. Antwortende Geräte können mit folgenden Antworttypen reagieren:

- `RDM_RESPONSE_TYPE_ACK` zeigt an, dass der Responder die Controller-Nachricht korrekt empfangen hat und die Anfrage verarbeitet.
- `RDM_RESPONSE_TYPE_ACK_OVERFLOW` zeigt an, dass der Responder die Anfrage verarbeitet, aber mehr Antwortdaten vorliegen, als in ein einzelnes Antwortpaket passen. Um die restlichen Informationen zu erhalten, kann der Controller wiederholt dieselbe PID anfragen, bis alles in eine einzelne Nachricht passt.
- `RDM_RESPONSE_TYPE_ACK_TIMER` zeigt an, dass der Responder die angeforderten GET-Informationen oder SET-Bestätigung nicht innerhalb der geforderten Antwortzeit liefern kann. In dieser Antwort gibt das Gerät eine geschätzte Wartezeit an, nach der die benötigte Information bereitsteht.
- `RDM_RESPONSE_TYPE_NACK_REASON` zeigt an, dass der Responder die angeforderten GET-Informationen nicht liefern oder den angegebenen SET-Befehl nicht verarbeiten kann. Die Antwort muss einen NACK-Grundcode enthalten. NACK-Grundcodes sind im [Anhang](#nack-reason-codes) aufgeführt.

Zusätzlich sind in dieser Bibliothek zwei weitere Antworttypen definiert, die bei der Verarbeitung von RDM-Daten helfen.

- `RDM_RESPONSE_TYPE_NONE` zeigt an, dass keine Antwort empfangen wurde.
- `RDM_RESPONSE_TYPE_INVALID` zeigt an, dass eine Antwort empfangen wurde, diese aber ungültig war. Das kann z. B. durch eine ungültige Prüfsumme oder ein ungültiges Paketformat auftreten.

Responder müssen auf jede nicht gebroadcastete RDM-Anfrage antworten sowie auf jede gebroadcastete `RDM_PID_DISC_UNIQUE_BRANCH`-Anfrage, wenn ihre Discovery nicht gemutet ist und ihre UID im angefragten Adressraum liegt. Bei Antworten auf `RDM_PID_DISC_UNIQUE_BRANCH` dürfen sie keinen DMX-Break und kein Mark-After-Break senden, um die Discovery zu beschleunigen, und müssen ihre Antwort so codieren, dass Datenverlust bei Kollisionen reduziert wird. Das Weglassen von Break und Mark-After-Break übernimmt der DMX-Treiber automatisch. Auf `RDM_PID_DISC_UNIQUE_BRANCH`, `RDM_PID_DISC_MUTE` und `RDM_PID_DISC_UN_MUTE` darf nur mit `RDM_RESPONSE_TYPE_ACK` geantwortet werden.

<a id="configuring-the-dmx-port"></a>
## DMX-Port konfigurieren

Die Funktionen des DMX-Treibers identifizieren jeden UART-Controller über `dmx_port_t`. Diese Kennung wird bei allen folgenden Funktionsaufrufen benötigt.

<a id="installing-the-driver"></a>
### Treiber installieren

Bevor DMX-Funktionen aufgerufen werden können, muss der DMX-Treiber installiert werden. Die Installation erfolgt über `dmx_driver_install()`. Diese Funktion reserviert die benötigten Ressourcen und initialisiert den Treiber mit Standard-DMX-Timing. Folgende Parameter werden übergeben:

- Der zu verwendende DMX-Port.
- Die zu verwendende DMX-Konfiguration. Mit dem Makro `DMX_CONFIG_DEFAULT` kann eine Struktur mit Standardwerten deklariert werden.
- Die DMX-Personalities des Geräts als Array vom Typ `dmx_personality_t`. Wenn das Gerät keine DMX-Slots nutzt, kann dieser Wert `NULL` sein.
- Die Anzahl der Personalities oder 0, wenn das Gerät keine DMX-Slots nutzt. Maximal sind 255 Personalities zulässig.

```c
dmx_config_t config = DMX_CONFIG_DEFAULT;
dmx_personality_t personalities[] = {
  {1, "Single-channel Mode"},  // Single-address DMX personality
  {3, "RGB"},                  // Three-address RGB mode
  {4, "RGBW"},                 // Four-address RGBW personality
  {7, "RGBW with Macros"}      // RGBW with three additional macro parameters
};
const int personality_count = 4;
dmx_driver_install(DMX_NUM_1, &config, personalities, personality_count);
```

`dmx_config_t` setzt dauerhafte Konfigurationswerte im DMX-Treiber. Diese Werte werden für die DMX-Gerätekonfiguration und den RDM-Responder genutzt. Zu den Feldern von `dmx_config_t` gehören:

- `interrupt_flags`: Zu verwendende Interrupt-Flags. Standardwert: `DMX_INTR_FLAGS_DEFAULT`.
- `root_device_parameter_count`: Anzahl der vom Root-Gerät unterstützten Parameter. Das ist die Anzahl registrierbarer Parameter auf dem Root-Gerät. Standardwert: `32`.
- `sub_device_parameter_count`: Anzahl der von Subgeräten unterstützten Parameter. Das ist die Anzahl registrierbarer Parameter pro Subgerät. Standardwert: `0`.
- `model_id`: Modell-ID des Root-Geräts. Ein frei wählbarer Wert, um unterschiedliche Modelle eines Herstellers eindeutig zu unterscheiden. Standardwert: `0`.
- `product_category`: Geräte melden eine Produktkategorie entsprechend ihrer Hauptfunktion. Die Kategorien sind in `product_category_t` aufgeführt. Standardwert: `RDM_PRODUCT_CATEGORY_FIXTURE`.
- `software_version_id`: Softwareversions-ID des Geräts. Diese 32-Bit-ID wird vom Hersteller festgelegt. Standardwert basiert auf der aktuellen *esp_dmx*-Version.
- `software_version_label`: Dieses RDM-Parameter liefert ein beschreibendes ASCII-Textlabel der laufenden Softwareversion. Der Text ist für die Benutzeranzeige gedacht. Standardwert ist ein String basierend auf der aktuellen *esp_dmx*-Version.
- `queue_size_max`: Maximale Größe der RDM-Queue. Der Wert 0 deaktiviert die RDM-Queue. Standardwert: `32`.

Der Typ `dmx_personality_t` ist eine Struktur mit zwei Feldern: `footprint` und `description`. `footprint` ist der DMX-Footprint der Personality, also die Anzahl der verwendeten DMX-Slots. `description` ist ein String, der den Zweck der DMX-Personality beschreibt. Dieses Feld wird für RDM-Antworten verwendet und darf inklusive Nullterminator bis zu 33 Zeichen lang sein.

```c
dmx_config_t config = {
  .interrupt_flags = DMX_INTR_FLAGS_DEFAULT,
  .root_device_parameter_count = 32,
  .sub_device_parameter_count = 0,
  .model_id = 0,
  .product_category = RDM_PRODUCT_CATEGORY_FIXTURE,
  .software_version_id = ESP_DMX_VERSION_ID,
  .software_version_label = ESP_DMX_VERSION_LABEL,
  .queue_size_max = 32
};
dmx_driver_install(DMX_NUM_1, &config, personalities, personality_count);
```

<a id="setting-communication-pins"></a>
### Kommunikations-Pins festlegen

Nach der Installation des DMX-Treibers können die physischen GPIO-Pins konfiguriert werden, mit denen der DMX-Port verbunden wird. Rufe dazu `dmx_set_pin()` auf und gib an, welche GPIOs den Signalen TX, RX und RTS zugeordnet werden. Soll ein bereits belegter Pin für ein Signal unverändert bleiben, übergib das Makro `DMX_PIN_NO_CHANGE`. Dieses Makro sollte auch verwendet werden, wenn ein Pin nicht genutzt wird.

```c
// Set TX: GPIO16 (port 2 default), RX: GPIO17 (port 2 default), RTS: GPIO21.
dmx_set_pin(DMX_NUM_1, DMX_PIN_NO_CHANGE, DMX_PIN_NO_CHANGE, 21);
```

<a id="timing-configuration"></a>
### Timing-Konfiguration

In den meisten Situationen ist es nicht nötig, das Standard-Timing des DMX-Treibers anzupassen. Dennoch erlaubt diese Bibliothek eine individuelle Konfiguration von DMX-Baudrate, Break und Mark-After-Break für den DMX-Controller. Diese Funktionen wirken nicht beim Empfang von DMX; sie beeinflussen Baudrate, Break und Mark-After-Break nur beim Senden von DMX oder RDM. Nach der Treiberinstallation können folgende Funktionen aufgerufen werden.

```c
dmx_set_baud_rate(DMX_NUM_1, DMX_BAUD_RATE);     // Set DMX baud rate.
dmx_set_break_len(DMX_NUM_1, DMX_BREAK_LEN_US);  // Set DMX break length.
dmx_set_mab_len(DMX_NUM_1, DMX_MAB_LEN_US);      // Set DMX MAB length.
```

Werden Timing-Werte außerhalb der DMX-Spezifikation übergeben, werden sie auf gültige DMX-Werte begrenzt. Beachte, dass Werte innerhalb der DMX-Spezifikation trotzdem außerhalb der RDM-Spezifikation liegen können. Die Funktionen sollten daher mit Bedacht genutzt werden, damit RDM-Fähigkeit erhalten bleibt.

Die oben genannten Funktionen besitzen jeweils `_get_`-Gegenstücke, um die aktuell gesetzten DMX-Timing-Parameter auszulesen.

<a id="reading-and-writing-dmx"></a>
## DMX lesen und schreiben

DMX ist ein unidirektionales Protokoll. Das bedeutet: Auf dem DMX-Bus kann immer nur ein Gerät senden, während viele Geräte zuhören. Daher erlaubt diese Bibliothek entweder Lesen oder Schreiben auf dem Bus, aber nicht beides gleichzeitig. Wenn gleichzeitiges Senden und Empfangen benötigt wird, können zwei UART-Ports verwendet und auf jedem Port ein Treiber installiert werden.

<a id="reading-dmx"></a>
### DMX lesen

Lesen vom DMX-Bus kann synchron oder asynchron erfolgen. Üblicherweise ist synchrones Lesen gewünscht. Dabei wird nur gelesen, wenn ein neues DMX-Paket empfangen wurde. Das ist ideal, weil dieselben Daten normalerweise nicht mehrfach gelesen werden sollen.

Für synchrones Lesen vom DMX-Bus muss der DMX-Treiber auf ein neues Paket warten. Dafür kann die blockierende Funktion `dmx_receive()` verwendet werden.

```c
dmx_packet_t packet;
// Wait for a packet. Returns the size of the received packet or 0 on timeout.
int packet_size = dmx_receive(DMX_NUM_1, &packet, DMX_TIMEOUT_TICK);
```

Die Funktion `dmx_receive()` hat drei Argumente. Das erste ist `dmx_port_t` und bestimmt, welcher DMX-Port genutzt wird. Das zweite ist ein Zeiger auf eine `dmx_packet_t`-Struktur. Beim Empfang eines Pakets werden Paketinformationen in diese Struktur kopiert. Dazu gehören:

- `err` meldet Fehler, die beim Empfang des Pakets aufgetreten sind (siehe: [Fehlerbehandlung](#error-handling)).
- `sc` ist der Startcode des Pakets.
- `size` ist die Paketgröße in Bytes inklusive DMX-Startcode. Dieser Wert ist nie größer als `DMX_PACKET_SIZE`.
- `is_rdm` ist true, wenn das Paket ein RDM-Paket ist und die RDM-Prüfsumme gültig ist.

Die Verwendung der `dmx_packet_t`-Struktur ist optional. Wenn DMX- oder RDM-Paketdaten nicht verarbeitet werden sollen, kann statt eines Zeigers auf `dmx_packet_t` auch `NULL` übergeben werden.

`dmx_receive()` liefert nur dann einen Wert ungleich null zurück, wenn neue Daten empfangen wurden. Als „neu“ gelten Daten, wenn ein DMX-Break empfangen wurde. Daten können auch bei einer `RDM_PID_DISC_UNIQUE_BRANCH`-Antwort als „neu“ gelten, da diese RDM-Antworten ohne DMX-Break gesendet werden.

Das letzte Argument von `dmx_receive()` ist die Anzahl der FreeRTOS-Ticks, die bis zum Timeout blockiert wird. Diese Bibliothek definiert dafür die Konstante `DMX_TIMEOUT_TICK`, also die Wartezeit, nach der ein DMX-Signal gemäß Spezifikation als verloren gilt. Laut DMX-Spezifikation entspricht das 1250 Millisekunden. Für nicht blockierendes Verhalten sollte dieser Wert auf 0 gesetzt werden.

Nach dem Paketempfang kann `dmx_read()` aufgerufen werden, um das Paket in einen Nutzerpuffer zu lesen. Es wird empfohlen, vor dem Lesen auf DMX-Fehler zu prüfen, ist aber nicht zwingend erforderlich.

```c
uint8_t data[DMX_PACKET_SIZE];

dmx_packet_t packet;
if (dmx_receive(DMX_NUM_1, &packet, DMX_TIMEOUT_TICK)) {

  // Check that no errors occurred.
  if (packet.err == DMX_OK) {
    dmx_read(DMX_NUM_1, data, packet.size);
  } else {
    printf("An error occurred receiving DMX!");
  }

} else {
  printf("Timed out waiting for DMX.");
}
```

Die Funktion `dmx_receive_num()` empfängt vor der Rückkehr eine festgelegte Anzahl von DMX-Slots. Sie entspricht `dmx_receive()`, bietet aber ein zusätzliches Argument zur Anzahl der zu empfangenden Slots. Beim Empfang von RDM-Paketen wird dieser Wert ignoriert, sodass `dmx_receive()` und `dmx_receive_num()` immer vollständige RDM-Pakete empfangen.

```c
dmx_packet_t packet;
int num_slots_to_receive = 96;
dmx_receive_num(DMX_NUM_1, &packet, num_slots_to_receive, DMX_TIMEOUT_TICK);
```

`dmx_receive()` kann als Wrapper um `dmx_receive_num()` betrachtet werden, wobei die Anzahl zu empfangender Slots der Paketgröße des zuletzt empfangenen DMX-Pakets entspricht. Ist die gewünschte Slot-Anzahl größer als tatsächlich empfangen (z. B. 513 erwartet, aber nur 128 empfangen), entblockt die Funktion beim DMX-Break des Folgepakets und setzt `packet.err` auf `DMX_ERR_NOT_ENOUGH_SLOTS`.

Es gibt zwei Varianten von `dmx_read()`. `dmx_read_offset()` ist ähnlich zu `dmx_read()`, erlaubt aber das Lesen eines Teilbereichs (Footprints) des gesamten DMX-Pakets.

```c
const int size = 12;   // The size of this device's DMX footprint.
const int offset = 5;  // The start address of this device.
uint8_t data[size];

// Read slots 5 through 17. Returns the number of slots that were read.
int num_slots_read = dmx_read_offset(DMX_NUM_1, offset, data, size);
```

Zuletzt kann `dmx_read_slot()` verwendet werden, um einen einzelnen DMX-Slot zu lesen.

```c
const int slot_num = 0;  // The slot to read. Slot 0 is the DMX start code!

// Read slot 0. Returns the value of the desired slot or -1 on error.
int value = dmx_read_slot(DMX_NUM_1, slot_num);
```

<a id="dmx-sniffer"></a>
### DMX-Sniffer

Diese Bibliothek bietet die Möglichkeit, Break- und Mark-After-Break-Zeiten empfangener DMX-Pakete zu messen. Der Sniffer ist deutlich ressourcenintensiver als der Standard-DMX-Treiber und muss daher explizit mit `dmx_sniffer_enable()` aktiviert werden.

Der DMX-Sniffer installiert einen flankengesteuerten Interrupt auf dem angegebenen GPIO-Pin. Diese Bibliothek nutzt die von ESP-IDF bereitgestellte GPIO-ISR, die individuelle Handler für bestimmte GPIO-Interrupts erlaubt. Der Handler iteriert über alle GPIOs, prüft auf ausgelöste Interrupts und ruft bei Bedarf den passenden Handler auf.

Eine Eigenheit der Standard-ESP-IDF-GPIO-ISR ist, dass niedrigere GPIO-Nummern früher verarbeitet werden als höhere. Es wird empfohlen, den DMX-Lesepin auf eine möglichst niedrige GPIO-Nummer zu legen, damit der DMX-Sniffer mit geringer Latenz laufen kann.

Wichtig ist, dass der Sniffer für niedrige Latenz eine hohe Taktfrequenz benötigt. Für eine verlässliche Genauigkeit sollte der ESP32 auf mindestens 160 MHz CPU-Takt eingestellt werden. Diese Einstellung kann bei Nutzung von ESP-IDF in `Kconfig` konfiguriert werden.

Vor dem Aktivieren des Sniffers muss `gpio_install_isr_service()` mit den benötigten DMX-Sniffer-Interrupt-Flags aufgerufen werden. Das Makro `DMX_SNIFFER_INTR_FLAGS_DEFAULT` liefert die passenden Flags.

```c
gpio_install_isr_service(DMX_SNIFFER_INTR_FLAGS_DEFAULT);

const int sniffer_pin = 4; // Lowest exposed pin on the Feather breakout board.
dmx_sniffer_enable(DMX_NUM_1, sniffer_pin);
```

Break- und Mark-After-Break-Zeiten werden bei aktiviertem Sniffer erfasst. Um Daten auszulesen, rufe nach dem Empfang eines DMX-Pakets `dmx_sniffer_get_data()` auf, um die Daten in eine `dmx_metadata_t`-Struktur zu kopieren. Wenn Daten kopiert wurden, gibt die Funktion `true` zurück.

```c
dmx_packet_t packet;
if (dmx_receive(DMX_NUM_1, &packet, DMX_TIMEOUT_TICK)) {
  dmx_metadata_t metadata;
  if (dmx_sniffer_get_data(DMX_NUM_1, &metadata, DMX_TIMEOUT_TICK)) {
    printf("The DMX break length was: %i\n", metadata.break_len);
    printf("The DMX mark-after-break length was: %i\n", metadata.mab_len);
  }
}
```

<a id="writing-dmx"></a>
### DMX schreiben

Zum Schreiben auf den DMX-Bus kann `dmx_write()` aufgerufen werden. Das schreibt Daten in den DMX-Treiber, sendet aber noch kein Paket. Um die geschriebenen Daten tatsächlich zu senden, muss `dmx_send()` aufgerufen werden.

```c
uint8_t data[DMX_PACKET_SIZE] = { 0, 1, 2, 3 };

// Write the packet and send it out on the DMX bus.
const int num_bytes_to_write = DMX_PACKET_SIZE;
dmx_write(DMX_NUM_1, data, num_bytes_to_write);
dmx_send(DMX_NUM_1,);
```

Das Senden eines typischen DMX-Pakets dauert etwa 22 Millisekunden. Währenddessen können mit `dmx_write()` bereits neue Daten geschrieben werden, solange keine RDM-Daten gesendet werden. Das führt jedoch zu asynchronem Schreiben, was ggf. unerwünscht ist. Für synchrones Schreiben muss gewartet werden, bis das aktuelle DMX-Paket vollständig gesendet wurde. Dafür dient `dmx_wait_sent()`.

```c
uint8_t data[DMX_PACKET_SIZE] = { 0, 1, 2, 3 };

while (true) {
  // Send the DMX packet.
  dmx_send(DMX_NUM_1);

  // Process the next DMX packet (while the previous is being sent) here.
  for (int i = 1; i < DMX_PACKET_SIZE; i++) {
    data[i]++;  // Increment the value of each slot, excluding the start code.
  }

  // Wait until the packet is finished being sent before proceeding.
  dmx_wait_sent(DMX_NUM_1, DMX_TIMEOUT_TICK);

  // Now write the packet synchronously!
  dmx_write(DMX_NUM_1, data, DMX_PACKET_SIZE);
}
```

Beim Senden von DMX sendet `dmx_send()` die maximal nach DMX-Standard erlaubte Slot-Anzahl. Wird mit `dmx_send()` ein RDM-Paket gesendet, überträgt der DMX-Treiber automatisch nur die Slots, die dieses RDM-Paket enthält.

Um eine bestimmte Anzahl an DMX-Slots zu senden, kann `dmx_send_num()` verwendet werden. Beim Senden von RDM-Daten wird diese Slot-Anzahl ignoriert.

```c
const int num_bytes_to_send = 96;
dmx_send_num(DMX_NUM_1, num_bytes_to_send);
```

Ein Bereich von DMX-Slots kann mit `dmx_write_offset()` geschrieben werden, einzelne DMX-Slots mit `dmx_write_slot()`. Das entspricht dem Verhalten von `dmx_read_offset()` und `dmx_read_slot()` beim Lesen.

```c
uint8_t data[DMX_PACKET_SIZE] = { 0, 1, 2, 3 };

// Write slots 10 through 17 (inclusive)
const int offset = 10;
const size_t size = 7;
dmx_write_offset(DMX_NUM_1, offset, data, size);

// Set slot number 5 to value 127.
const int slot_num = 5;
const uint8_t value = 127;
dmx_write_slot(DMX_NUM_1, slot_num, value);

// Don't forget to call dmx_send()!
```

<a id="dmx-parameters"></a>
### DMX-Parameter

Bei der Installation des DMX-Treibers werden einige Parameterwerte gesetzt, die vom Nutzer gelesen oder geschrieben werden können. Dazu gehören die aktuelle DMX-Personality, die Personality-Anzahl, der Footprint einer gewählten Personality, die Personality-Beschreibung und die DMX-Startadresse.

Das Auslesen oder Setzen der DMX-Startadresse erfolgt über `dmx_get_start_address()` und `dmx_set_start_address()`. Ist RDM aktiviert, verhalten sich diese Funktionen ähnlich zu `rdm_get_dmx_start_address()` und `rdm_set_dmx_start_address()`.

```c
// Get the DMX start address and increment it by one
uint16_t dmx_start_address = dmx_get_start_address(DMX_NUM_1);
dmx_start_address++;
if (dmx_start_address >= DMX_PACKET_SIZE_MAX) {
  dmx_start_address = 1;  // Ensure DMX start address is within bounds
}
dmx_set_start_address(DMX_NUM_1, dmx_start_address);
```

Auf Personalities, Personality-Anzahl, Personality-Beschreibungen und Footprint-Größen kann mit `dmx_get_current_personality()`, `dmx_set_current_personality()`, `dmx_get_personality_count()`, `dmx_get_personality_description()` und `dmx_get_footprint()` zugegriffen werden. Personalities sind ab 1 indiziert; eine Personality 0 gibt es nicht.

```c
const uint8_t personality_count = dmx_get_personality_count(DMX_NUM_1);
uint8_t current_personality = dmx_get_current_personality(DMX_NUM_1);
if (current_personality < personality_count) {
  // Increment the personality.
  current_personality++;
  /* It is ok if current_personality == personality_count because personalities
    start at 1, not 0! */

  // Get and print the new personality description and footprint.
  const char *desc = dmx_get_personality_description(DMX_NUM_1, 
                                                     current_personality);
  uint16_t footprint = dmx_get_footprint(DMX_NUM_1, current_personality);
  printf("Setting the current personality to %i: '%s'\n", current_personality,
         desc);
  printf("Personality %i has a footprint of %i\n", current_personality,
         footprint);
  
  dmx_set_current_personality(DMX_NUM_1, current_personality);
}
```

<a id="reading-and-writing-rdm"></a>
## RDM lesen und schreiben

Bereits mit den oben genannten Funktionen können RDM-Pakete gesendet und empfangen werden. Wird ein RDM-Paket mit `dmx_write()` geschrieben, reagiert der DMX-Treiber entsprechend und stellt sicher, dass RDM-Timing-Anforderungen eingehalten werden. Beispielsweise senden `dmx_send()` und `dmx_send_num()` bei einem DMX-Paket mit Null-Startcode üblicherweise DMX-Break und Mark-After-Break. Beim Senden eines RDM-Discovery-Antwortpakets entfernt der DMX-Treiber Break und Mark-After-Break automatisch gemäß RDM-Standard. Das Senden von RDM-Antworten mit `dmx_send()` oder `dmx_send_num()` kann außerdem fehlschlagen, wenn der Treiber erkennt, dass das RDM-Antwort-Timeout bereits abgelaufen ist. Das reduziert Datenkollisionen auf dem RDM-Bus und hält den Bus stabil.

```c
// This is a hard-coded discovery response packet.
const uint8_t discovery_response[] = {
  0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xaa, 0xaf, 0x55, 0xea, 0xf5, 0xba, 
  0x57, 0xbb, 0xdd, 0xbf, 0x55, 0xba, 0xdf, 0xaa, 0x5d, 0xbb, 0x7d 
};
dmx_write(DMX_NUM_1, discovery_response, sizeof(discovery_response));

// This function will not send a DMX break or mark-after-break 
dmx_send(DMX_NUM_1);
```

Ebenso verhalten sich die `dmx_receive()`-Funktionen kontextabhängig beim Empfang von DMX- oder RDM-Paketen. Beim DMX-Empfang greifen `dmx_receive()` und `dmx_receive_num()` gemäß übergebenem Timeout (z. B. `DMX_TIMEOUT_TICK`). Beim Empfang von RDM-Paketen kann der Treiber deutlich früher aussteigen, da die Umschaltzeiten auf dem RDM-Bus wesentlich kürzer sind als bei DMX.

```c
// This is a hard-coded GET DEVICE_INFO request.
const uint8_t get_device_info[] = {
  0xcc, 0x01, 0x18, 0x3b, 0x10, 0x44, 0xc0, 0x6f, 0xbf, 0x05, 0xe0, 0x12, 0x99,
  0x15, 0x9a, 0x14, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x60, 0x00, 0x06, 0x38
};
dmx_write(DMX_NUM_1, get_device_info, sizeof(get_device_info));
dmx_send(DMX_NUM_1, sizeof(get_device_info));

dmx_packet_t packet;

// This function will unblock early because it is expecting a reply!
dmx_receive(DMX_NUM_1, &packet, DMX_TIMEOUT_TICK);  // Unblocks in 3ms
```

Da das Schreiben von RDM-Anfragen und -Antworten auf diese Weise aufwendig sein kann, stellt die Bibliothek spezielle Funktionen dafür bereit. Diese werden über `#include "rdm/controller.h"` für Anfragen und `#include "rdm/responder.h"` für Antworten eingebunden.

<a id="rdm-requests"></a>
### RDM-Anfragen

Diese Bibliothek unterstützt die im RDM-Standard geforderten PIDs. Anfragefunktionen sind nach dem Muster Präfix `rdm_send_`, GET/SET-Typ und Parametername benannt. Um z. B. `RDM_PID_DEVICE_INFO` eines Responders per GET anzufragen, kann `rdm_send_get_device_info()` aufgerufen werden. Um `RDM_PID_DMX_START_ADDRESS` eines Geräts per SET zu setzen, kann `rdm_send_set_dmx_start_address()` verwendet werden. Alle GET-Funktionen liefern `true`, wenn `RDM_RESPONSE_TYPE_ACK` empfangen wurde, sonst `false`. Alle SET-Funktionen liefern bei `RDM_RESPONSE_TYPE_ACK` die Anzahl empfangener Bytes in den RDM-Parameterdaten, sonst 0. So liefert `rdm_send_get_software_version_label()` z. B. die Anzahl Zeichen des empfangenen Softwareversionslabels.

Zusätzlich zur DMX-Portnummer verwenden die meisten RDM-Anfragefunktionen mindestens zwei Argumente, um das Ziel der Anfrage festzulegen: `dest_uid` (Ziel-UID) und `sub_device` (RDM-Subgerät, das die Anfrage empfangen soll).

Beim Ausgeben von UIDs im Terminal können in printf-ähnlichen Funktionen die Makros `UIDSTR` und `UID2STR()` verwendet werden.

```c
rdm_uid_t dest_uid = {0x05e0, 0x44c06fbf};  // The destination UID
rdm_sub_device_t sub_device = RDM_SUB_DEVICE_ROOT;
rdm_ack_t ack;  // Stores response information

rdm_device_info_t device_info;  // Stores the response parameter data.
if (rdm_send_get_device_info(DMX_NUM_1, &dest_uid, sub_device, &device_info,
                             &ack)) {
  printf("Successfully received device info from " UIDSTR "!\n",
          UID2STR(ack.src_uid));
}

const uint16_t new_address = 123;  // The new RDM_PID_DMX_START_ADDRESS to send.
if (rdm_send_set_dmx_start_address(DMX_NUM_1, &dest_uid, sub_device, 
                                   new_address, &ack)) {
  printf("Device " UIDSTR " has been set to DMX address %i.\n",
          UID2STR(dest_uid), new_address);
}
```

Antwortinformationen aus Anfragen werden in einen vom Nutzer bereitgestellten `rdm_ack_t`-Zeiger geschrieben. Mit diesem Typ kann geprüft werden, ob Anfragen erfolgreich waren, und bei Fehlern entsprechend reagiert werden. `rdm_ack_t` enthält folgende Felder:

- `err` wird auf einen Fehlerwert ungleich null gesetzt, wenn beim Lesen von DMX-Daten ein Fehler auftrat. Dieses Feld zeigt nur Fehler beim Lesen roher DMX-Daten an, nicht den Empfang ungültiger RDM-Pakete. Mehr dazu im Abschnitt [Fehlerbehandlung](#error-handling).
- `size` ist die Größe des empfangenen Pakets inklusive Startcode, RDM-Sub-Startcode und Prüfsumme.
- `src_uid` ist die UID des Geräts, das das Antwortpaket gesendet hat.
- `pid` ist die PID des Antwortpakets. Sie ist in der Regel identisch mit der in der Anfrage gesendeten PID, kann bei manchen Anfragen aber abweichen.
- `type` ist der Typ der empfangenen RDM-Antwort. Er kann jeder der in [Antworttypen](#response-types) aufgeführten RDM-Antworttypen sein.
- `message_count` wird vom RDM-Responder genutzt, um anzuzeigen, dass zusätzliche Daten zur Abholung durch einen Controller bereitstehen.

Das verbleibende Feld ist eine Union und sollte abhängig vom Wert in `type` gelesen werden.

- `pdl` sollte gelesen werden, wenn `type` den Wert `RDM_RESPONSE_TYPE_ACK` hat. Es beschreibt die Größe der empfangenen RDM-Parameterdaten.
- `timer` sollte gelesen werden, wenn `type` den Wert `RDM_RESPONSE_TYPE_TIMER` hat. Es beschreibt die Anzahl FreeRTOS-Ticks, die vergehen müssen, bis der RDM-Responder die Anfrage verarbeiten kann.
- `nack_reason` sollte gelesen werden, wenn `type` den Wert `RDM_RESPONSE_TYPE_NACK_REASON` hat. Es beschreibt den vom RDM-Responder empfangenen NACK-Grundcode.

<a id="discovering-devices"></a>
### Geräte entdecken

Diese Bibliothek bietet zwei Funktionen für eine vollständige RDM-Discovery. `rdm_discover_devices_simple()` ist eine einfache Implementierung des Discovery-Algorithmus. Sie erhält einen Zeiger auf ein UID-Array zur Ablage gefundener UIDs und gibt die Anzahl gefundener UIDs zurück.

```c
const int array_size = 10;
rdm_uid_t uids[array_size];

// This function blocks and may take some time to complete!
int num_uids = rdm_discover_devices_simple(DMX_NUM_1, uids, array_size);

printf("Discovery found %i UIDs!\n", num_uids);
```

Die Geräteerkennung kann mehrere Sekunden dauern. Oft soll bei jeder neu gefundenen UID eine Aktion ausgeführt werden, z. B. das Aktualisieren einer Fortschrittsanzeige. Dafür kann `rdm_discover_with_callback()` verwendet werden, um eine Callback-Funktion anzugeben, die bei jeder neu gefundenen UID aufgerufen wird.

`RDM_PID_DISC_UNIQUE_BRANCH` unterstützt weder GET noch SET. Diese PID-Anfrage wird über `rdm_send_disc_unique_branch()` gesendet. `RDM_PID_DISC_UNIQUE_BRANCH` darf nur an das Root-Gerät und nur als Adresse an alle Geräte im RDM-Netzwerk gesendet werden. Deshalb gibt es für diese Funktion keine Argumente `dest_uid` und `sub_device`.

```c
rdm_ack_t ack;

// Define the address space within which devices will be discovered.
const rdm_disc_unique_branch_t branch = {
  .upper_bound = RDM_UID_MAX,
  .lower_bound = 0  // Set to 0000:00000000
};

rdm_send_disc_unique_branch(DMX_NUM_1, &branch, &ack);
if (ack.size > 0) {
  // Got a response!
  if (ack.type == RDM_RESPONSE_TYPE_ACK) {
    // Only one device was found - print its UID.
    printf("Found the UID " UIDSTR ".\n", UID2STR(ack.src_uid));
  } else if (ack.type == RDM_RESPONSE_TYPE_INVALID) {
    // The checksum was invalid indicating a data collision occurred.
    printf("Multiple devices detected within this address space!\n");

    // Branch the address space here...

  }
} else {
  // No response was received - stop searching this address space.
  printf("No RDM devices were discovered in this address space.\n");
}
```

`RDM_PID_DISC_MUTE` und `RDM_PID_DISC_UN_MUTE` unterstützen ebenfalls weder GET noch SET. Geräte können mit `rdm_send_disc_mute()` und `rdm_send_disc_un_mute()` gemutet bzw. entmutet werden. Diese Anfragen dürfen an jede Ziel-UID gesendet werden, aber nur an das Root-Gerät. Daher gibt es das Argument `dest_uid`, aber kein `sub_device`. Beide Anfragen liefern dieselben Antwortdaten, deshalb kann `rdm_disc_mute_t` für beide zum Speichern der Parameterdaten verwendet werden.

```c
rdm_uid_t dest_uid = RDM_UID_BROADCAST_ALL;
rdm_ack_t ack;

rdm_disc_mute_t mute;  // Stores the response parameter data.

rdm_send_disc_un_mute(DMX_NUM_1, &dest_uid, &mute, &ack);
if (ack.size > 0) {
  /* This code will never run because the RDM controller does not receive a
    response from RDM responders when the destination UID is a broadcast UID.
    Therefore its return value can be ignored and the function can be passed
    NULL instead of an rdm_ack_t pointer or an rdm_disc_mute_t pointer. */
}
```

<a id="rdm-responder"></a>
### RDM-Responder

Ein RDM-Responder muss auf jedes an ihn adressierte Nicht-Discovery- und Nicht-Broadcast-Paket antworten. Wenn ein Responder ein `RDM_PID_DISC_UNIQUE_BRANCH`-Paket erhält, muss er antworten, wenn seine UID im angefragten Adressraum liegt und er nicht gemutet ist.

Der DMX-Treiber parst RDM-Anfragen und sendet Antworten innerhalb von `rdm_send_response()`. Daher müssen alle RDM-Responder RDM-Anfragen mit `dmx_receive()` oder `dmx_receive_num()` empfangen und Antworten mit `rdm_send_response()` senden. Wird `rdm_send_response()` nicht aufgerufen, wird keine RDM-Antwort gesendet. Wenn Geräte nicht auf RDM-Anfragen reagieren sollen, kann der Aufruf entfallen. Für RDM-Konformität sollten Nutzer `rdm_send_response()` nach jeder empfangenen RDM-Anfrage aufrufen.

```c
dmx_packet_t packet;
if (dmx_receive(DMX_NUM_1, &packet, DMX_TIMEOUT_TICK)) {
  if (packet.is_rdm) {
    rdm_send_response(DMX_NUM_1);  // Only sends responses to relevant requests
  }
}
```

RDM stellt strenge Timing-Anforderungen an Responder. Typischerweise müssen Antworten innerhalb von etwa 3 Millisekunden erfolgen. Deshalb sollte `rdm_send_response()` schnell nach dem Empfang neuer RDM-Daten aufgerufen werden. Von langen Funktionsaufrufen (z. B. Terminalausgaben) zwischen `dmx_receive()` und `rdm_send_response()` wird abgeraten.

```c
dmx_packet_t packet;
if (dmx_receive(DMX_NUM_1, &packet, DMX_TIMEOUT_TICK)) {

  // Caution! Printing log messages may take too long!
  printf("A DMX packet has been received!");

  if (packet.is_rdm) {
    rdm_send_response(DMX_NUM_1);  // Only sends responses to relevant requests
  }
}
```

RDM-Parameter können beim DMX-Treiber über Funktionen mit dem Präfix `rdm_register_` registriert werden. Der Parameter `RDM_PID_DMX_START_ADDRESS` wird z. B. mit `rdm_register_dmx_start_address()` registriert. Parameterdaten werden vom DMX-Treiber verwaltet und initialisiert, aber Nutzer können für einige Parameter Initialwerte über die Argumente der `rdm_register_`-Funktionen setzen.

RDM-Parameter, die GET aber nicht SET unterstützen, erlauben in der Regel das Setzen des Initialwerts als zweites Argument der `rdm_register_`-Funktion. Dieser Initialwert wird beim ersten Aufruf gesetzt; danach wird das Argument ignoriert und kann `NULL` bleiben. Parameter mit GET und SET erhalten bei der Registrierung meist einen vordefinierten Initialwert und müssen über die entsprechende `rdm_set_`-Funktion manuell geändert werden.

Mit den `rdm_register_`-Funktionen können Nutzer Callback-Funktionen an PIDs hängen. Wenn eine gültige Anfrage für einen Parameter eingeht, ruft der DMX-Treiber die Callback-Funktion nach der Verarbeitung der Anfrage auf. Ein Callback-Aufruf bedeutet nicht zwingend, dass bereits ein Antwortpaket gesendet wurde.

```c
void custom_callback(dmx_port_t dmx_num, rdm_header_t *request,
                     rdm_header_t *response, void *context) {
  if (request->pid == RDM_PID_SOFTWARE_VERSION_LABEL) {
    printf("A RDM_PID_SOFTWARE_VERSION_LABEL request was received!\n");
  }
}
```

Die Argumente der Callback-Funktion entsprechen dem in der Anfrage empfangenen RDM-Header und dem in der Antwort gesendeten RDM-Header. Zusätzlich werden die DMX-Portnummer und ein Benutzerkontext übergeben.

```c
void *context = NULL;  // Context not needed for the above callback 
const char *new_software_label = "My Custom Software";
if (rdm_register_software_version_label(DMX_NUM_1, new_software_label, 
                                        custom_callback, context)) {
  printf("A new software version label has been registered!\n");
}
```

Wenn eine Anfrage für eine nicht registrierte PID empfangen wird, antwortet der DMX-Treiber automatisch mit `RDM_RESPONSE_NACK_REASON` und `RDM_NR_UNKNOWN_PID`. Wird ein bereits definierter Parameter erneut registriert, wird der zuvor registrierte Callback überschrieben, nicht jedoch der Initialwert. Registrierte Parameter können nicht wieder deregistriert werden.

Der RDM-Standard definiert mehrere Parameterantworten, die von allen RDM-konformen Respondern unterstützt werden müssen. Diese Funktionen werden bei der Installation des DMX-Treibers automatisch registriert. So wird sichergestellt, dass mit dieser Bibliothek erstellte RDM-Responder der RDM-Spezifikation entsprechen. Folgende Parameter sind laut RDM-Spezifikation erforderlich und werden daher automatisch registriert:

- `RDM_PID_DISC_UNIQUE_BRANCH`
- `RDM_PID_DISC_MUTE`
- `RDM_PID_DISC_UN_MUTE`
- `RDM_PID_DEVICE_INFO`
- `RDM_PID_SOFTWARE_VERSION_LABEL`
- `RDM_PID_IDENTIFY_DEVICE`
- `RDM_PID_DMX_START_ADDRESS`, wenn das Gerät einen DMX-Slot verwendet.
- `RDM_PID_SUPPORTED_PARAMETERS`, wenn Parameter über den minimal erforderlichen Satz hinaus unterstützt werden.
- `RDM_PID_PARAMETER_DESCRIPTION`, wenn herstellerspezifische Parameter unterstützt werden.

Die folgenden Parameter sind laut RDM-Spezifikation nicht erforderlich, werden aber bei der Installation des DMX-Treibers automatisch registriert. Die Registrierung erfolgt in folgender Reihenfolge, sofern im DMX-Treiber ausreichend Parameterspeicher vorhanden ist:

- `RDM_PID_QUEUED_MESSAGE`, sofern in `dmx_config_t` angegeben.
- `RDM_PID_MANUFACTURER_LABEL`
- `RDM_PID_DMX_PERSONALITY`, wenn das Gerät einen DMX-Slot verwendet.
- `RDM_PID_DMX_PERSONALITY_DESCRIPTION`, wenn das Gerät einen DMX-Slot verwendet.
- `RDM_PID_DEVICE_LABEL`

Registrierte Parameter können über Getter- und Setter-Funktionen gelesen oder gesetzt werden. Parameter mit der Command Class `RDM_CC_GET_COMMAND` haben einen Getter mit Präfix `rdm_get_`, Parameter mit `RDM_CC_SET_COMMAND` einen Setter mit Präfix `rdm_set_`. Setter liefern `true`, wenn der Wert erfolgreich gesetzt wurde. Getter liefern die Größe der Parameterdaten in Bytes oder bei Fehler 0.

Einige Parameter, z. B. `RDM_PID_DMX_START_ADDRESS`, werden in nichtflüchtigen Speicher kopiert, damit Werte nach einem Neustart des ESP32 erhalten bleiben. Das Kopieren erfolgt beim Setzen über die jeweilige `rdm_set_`-Funktion oder nach dem Empfang einer gültigen SET-Anfrage.

```c
uint16_t dmx_start_address;
if (rdm_get_dmx_start_address(DMX_NUM_1, &dmx_start_address) == 0) {
  printf("An error occurred getting the DMX start address.\n");
}

dmx_start_address = 123;
if (!rdm_set_dmx_start_address(DMX_NUM_1, dmx_start_address)) {
  printf("An error occurred setting the DMX start address.\n");
}
```

<a id="error-handling"></a>
## Fehlerbehandlung

In seltenen Fällen können DMX-Pakete beschädigt sein. Fehler werden typischerweise direkt nach dem Verbinden mit einem aktiven DMX-Bus erkannt und mit dem nächsten Paket wieder behoben. Fehler lassen sich über den Fehlercode in der Struktur `dmx_packet_t` prüfen. Folgende Fehlertypen gibt es:

- `DMX_OK` bedeutet, dass Daten erfolgreich gelesen wurden.
- `DMX_ERR_TIMEOUT` bedeutet, dass der Treiber beim Warten auf ein Paket in einen Timeout gelaufen ist.
- `DMX_ERR_IMPROPER_SLOT` tritt auf, wenn der DMX-Treiber fehlende Stoppbits erkennt. In diesem Fall verwirft der Treiber den fehlerhaft gerahmten Slot und alle folgenden Slots des Pakets. Bei diesem Fehler kann über `dmx_packet_t.size` ermittelt werden, an welchem Slot der Fehler auftrat.
- `DMX_ERR_UART_OVERFLOW` tritt auf, wenn die ESP32-Hardware überläuft und dadurch Daten verloren gehen.
- `DMX_ERR_NOT_ENOUGH_SLOTS` tritt auf, wenn weniger Slots empfangen wurden als in `dmx_receive_num()` angefordert.

```c
uint8_t data[DMX_PACKET_SIZE];

int num_slots = DMX_PACKET_SIZE;
dmx_packet_t packet;
while (true) {
  if (dmx_receive_num(DMX_NUM_1, &packet, num_slots, DMX_TIMEOUT_TICK)) {
    switch (packet.err) {
      case DMX_OK:
        printf("Received packet with start code: %02X and size: %i.\n",
          packet.sc, packet.size);
        // Data is OK. Now read the packet into the buffer.
        dmx_read(DMX_NUM_1, data, packet.size);
        break;
      
      case DMX_ERR_TIMEOUT:
        printf("The driver timed out waiting for the packet.\n");
        /* If the provided timeout was less than DMX_TIMEOUT_TICK, it may be
          worthwhile to call dmx_receive() again to see if the packet could be
          received. */
        break;

      case DMX_ERR_IMPROPER_SLOT:
        printf("Received malformed byte at slot %i.\n", packet.size);
        /* A slot in the packet is malformed. Data can be recovered up until 
          packet.size. */
        break;

      case DMX_ERR_UART_OVERFLOW:
        printf("The DMX port overflowed.\n");
        /* The ESP32 UART overflowed. This could occur if the DMX ISR is being
          constantly preempted. */
        break;
      
      case DMX_ERR_NOT_ENOUGH_SLOTS:
        printf("DMX packet size is too small. %i expected, %i received.\n",
               num_slots, packet.size);
        /* The packet was smaller than expected. This only occurs when receiving
          DMX data. This error will not occur when receiving RDM packets.*/
        num_slots = packet.size;  // Update expected packet size
        break;
    }
  } else {
    printf("Lost DMX signal.\n");
    // A packet hasn't been received in DMX_TIMEOUT_TICK ticks.

    // Handle packet timeout here...
  }
}
```

Beim Lesen von RDM-Paketen wird das Feld `packet.err` in den Typ `rdm_ack_t` übernommen. Wichtig: RDM-Paketfehler werden dabei nicht als `err` gemeldet. `err` zeigt nur Fehler bei der Verarbeitung roher DMX-Daten an. Ein ungültiges RDM-Paket wird über das Feld `type` in `rdm_ack_t` gemeldet, und zwar als `RDM_RESPONSE_TYPE_INVALID`.

<a id="timing-macros"></a>
### Timing-Makros

Diese Bibliothek prüft DMX-Timing-Fehler nicht automatisch. Sie stellt jedoch Makros zur Unterstützung bereit; die eigentliche Implementierung solcher Prüfungen liegt beim Nutzer. Da DMX und RDM eigene Timing-Anforderungen haben, gibt es Makros für beide. Folgende Makros helfen bei der Timing-Fehlerprüfung.

- `dmx_baud_rate_is_valid()` ergibt true, wenn die Baudrate für DMX gültig ist.
- `dmx_break_len_is_valid()` ergibt true, wenn die DMX-Break-Dauer gültig ist.
- `dmx_mab_len_is_valid()` ergibt true, wenn die DMX-Mark-After-Break-Dauer gültig ist.
- `rdm_baud_rate_is_valid()` ergibt true, wenn die Baudrate für RDM gültig ist.
- `rdm_break_len_is_valid()` ergibt true, wenn die RDM-Break-Dauer gültig ist.
- `rdm_mab_len_is_valid()` ergibt true, wenn die RDM-Mark-After-Break-Dauer gültig ist.

DMX und RDM definieren unterschiedliche Timing-Anforderungen für Empfänger und Sender. Diese Bibliothek vereinfacht die Fehlerprüfung, indem Anforderungen für Senden und Empfangen zusammengefasst werden. Daher gibt es nur die sechs oben genannten Makros statt jeweils sechs separater Makros für Empfang und Übertragung.

<a id="dmx-start-codes"></a>
### DMX-Startcodes

Diese Bibliothek bietet die folgenden Makro-Konstanten zur Verwendung als DMX-Startcodes. Mehr Informationen zu jedem Startcode findest du im DMX-Standarddokument oder in [dmx/include/types.h](src/dmx/include/types.h).

- `DMX_SC` ist der standardmäßige DMX-Null-Startcode.
- `RDM_SC` ist der standardmäßige Startcode für Remote Device Management.
- `DMX_TEXT_SC` ist der ASCII-Text-Startcode.
- `DMX_TEST_SC` ist der Startcode für Testpakete.
- `DMX_UTF8_SC` ist der Startcode für UTF-8-Textpakete.
- `DMX_ORG_ID_SC` ist der Startcode für Organisations-/Hersteller-ID.
- `DMX_SIP_SC` ist der Startcode für System Information Packets.

Weitere Makro-Konstanten sind:

- `RDM_SUB_SC` ist der Sub-Startcode für Remote Device Management. Es ist das erste Byte nach dem RDM-Startcode.
- `RDM_PREAMBLE` gilt nicht als Startcode, ist aber oft das erste Byte in einem RDM-Discovery-Antwortpaket.
- `RDM_DELIMITER` gilt nicht als Startcode, ist aber das Trennbyte am Ende einer RDM-Discovery-Antwortpräambel.

Einige Startcodes gelten als ungültig und sollten nicht in DMX-Paketen verwendet werden. Die Gültigkeit eines Startcodes kann mit `dmx_start_code_is_valid()` geprüft werden. Ist der Startcode gültig, ergibt das Makro true. Diese Bibliothek prüft Startcodes nicht automatisch; diese Prüfung muss vom Nutzer implementiert werden.

<a id="additional-considerations"></a>
## Weitere Hinweise

<a id="using-flash-or-disabling-cache"></a>
### Flash-Nutzung oder Cache deaktivieren

Beim Aufruf von Funktionen, die auf dem ESP32 aus dem Flash lesen oder in den Flash schreiben, wird der Cache kurzzeitig deaktiviert und bestimmte Interrupts werden unterdrückt. Ist der DMX-Treiber falsch konfiguriert, kann das zu Datenkorruption führen.

Die enthaltene `Kconfig`-Datei weist das ESP32-Buildsystem an, den DMX-Treiber und einige seiner Funktionen in IRAM zu platzieren. Diese und weitere Optionen können über `menuconfig` des ESP32 deaktiviert werden. Bei Nutzung des Arduino-Frameworks werden `menuconfig` und `Kconfig` nicht unterstützt.

Der DMX-Treiber kann entweder in IRAM oder im Flash liegen. Standardmäßig werden der Treiber und zugehörige Funktionen in IRAM platziert, um Ladeverzögerungen aus dem Flash zu reduzieren. Die Platzierung im Flash ist möglich, aber weniger performant. Beim Arduino-Framework kann der Treiber im Flash liegen.

Liegt der Treiber nicht in IRAM, deaktivieren cache-deaktivierende Funktionen auch den DMX-Treiber vorübergehend. Um Datenkorruption zu vermeiden, sollte der DMX-Treiber vor dem Deaktivieren des Caches sauber deaktiviert werden. Das geht mit `dmx_driver_disable()`. Reaktiviert wird er mit `dmx_driver_enable()`. Der Status kann mit `dmx_driver_is_enabled()` geprüft werden.

```c
// Disable the DMX driver if it isn't already
if (dmx_driver_is_enabled(DMX_NUM_1)) {
  dmx_driver_disable(DMX_NUM_1);
}

// Read from or write to flash memory (or otherwise disable the cache) here...

dmx_driver_enable(DMX_NUM_1);
```

Das Deaktivieren und erneute Aktivieren des DMX-Treibers vor einer Cache-Deaktivierung ist nicht nötig, wenn der Treiber in IRAM liegt.

<a id="wiring-an-rs-485-circuit"></a>
### RS-485-Schaltung verdrahten

DMX wird über RS-485 übertragen. RS-485 nutzt verdrillte Adernpaare, Halbduplex und differenzielle Signalisierung, damit Datenpakete über größere Distanzen übertragen werden können. DMX beginnt als UART-Signal und wird anschließend über einen RS-485-Transceiver getrieben. Da der ESP32 keinen integrierten RS-485-Transceiver besitzt, muss er in den meisten Fällen extern mit einem Transceiver verbunden werden.

RS-485-Transceiver haben typischerweise vier Datenpins: `RO`, `DI`, `DE` und `/RE`. `RO` ist der Empfängerausgang und wird mit UART-RX verbunden, damit Daten von anderen Geräten zum ESP32 gelesen werden können. `DI` ist der Treibereingang und wird mit UART-TX verbunden, damit Daten vom ESP32 an andere Geräte gesendet werden können. `DE` aktiviert den Treiber (active high). `/RE` aktiviert den Empfänger (active low). Der Überstrich bei `/RE` bedeutet: low = aktiv, high = inaktiv.

Da `DE` das Senden und `/RE` das Lesen steuert und `DE` active high sowie `/RE` active low ist, werden beide Pins häufig zusammengeschaltet. In diesem Beispiel sind sie verbunden und werden über einen einzigen ESP32-Pin gesteuert. Dieser wird Enable-Pin bzw. auch RTS-Pin genannt.

In dieser Beispielschaltung haben R1 und R3 jeweils 680 Ohm. Viele RS-485-Breakout-Boards verwenden hier 20 kOhm oder mehr. Solch hohe Widerstandswerte sind akzeptabel und erlauben in der Regel weiterhin DMX-Schreiben und -Lesen.

R2, der 120-Ohm-Widerstand, ist ein Abschlusswiderstand. Er ist nur bei RDM zwingend erforderlich. In Schaltplänen kann er außerdem zur Stabilität beitragen, wenn lange DMX-Leitungen mit mehreren Geräten verbunden sind. Wird dieser Widerstand nicht verwendet, dürfen DMX-A und DMX-B nicht kurzgeschlossen werden.

Viele RS-485-Chips wie der [Maxim MAX485](https://datasheets.maximintegrated.com/en/ds/MAX1487-MAX491.pdf) sind 3,3-V-tolerant. Das bedeutet, sie können ohne zusätzliche Bauteile direkt mit dem ESP32 angesteuert werden. Andere RS-485-Chips benötigen ggf. 5-V-Datenpegel für DMX. In diesem Fall muss der ESP32-Ausgang über einen Pegelwandler auf 5 V umgesetzt werden.

<a id="hardware-specifications"></a>
<a id="hardware"></a>
### Hardware-Spezifikationen

ANSI-ESTA E1.11 DMX512-A fordert, dass DMX-Geräte elektrisch von anderen Geräten auf dem DMX-Bus isoliert sind. Bei einer Überspannung wäre dann im Worst Case die RS-485-Schaltung betroffen und nicht das gesamte DMX-Gerät. Manche DMX-Geräte funktionieren ohne Isolation, der Einsatz nicht isolierter Hardware wird jedoch nicht empfohlen.


<a id="wireless-dmx-build-guide"></a>
## Bauanleitung für kabelloses DMX

Diese Anleitung zeigt Schritt für Schritt, wie du ein **kabelloses DMX-Sender/Empfänger-System** mit zwei ESP32-Boards und zwei DollaTek-5V-MAX485-TTL-zu-RS485-Modulen aufbaust.

Der Aufbau ist **nicht galvanisch getrennt** und damit für Lernen, Tests und kurze Leitungen gedacht. Für Bühne/Veranstaltung nutze bitte die Hinweise unter [Hinweis zur Isolation](#isolation-note).

### Hardware

| Anzahl | Bauteil |
|-----|------|
| 2 | ESP32-Entwicklungsboard |
| 2 | DollaTek 5V MAX485 TTL-zu-RS485 Modul |
| 2 | XLR-3 Steckverbinder (1x female für DMX IN, 1x male für DMX OUT) |
| 1 | 1.8 kΩ Widerstand (nur Sender, Spannungsteiler) |
| 1 | 3.3 kΩ Widerstand (nur Sender, Spannungsteiler) |
| 2 | 0.1 µF Keramikkondensator |
| 2 | 10 µF Elektrolytkondensator |
| 1 | 120 Ω Widerstand (Abschluss, Empfänger) |

<a id="esp32-pins"></a>
### ESP32-Pins

Auf beiden Boards wird **UART2** genutzt:

| ESP32-Label | GPIO | Zweck |
|-------------|------|---------|
| `RX2` | GPIO16 | UART2 Empfang |
| `TX2` | GPIO17 | UART2 Senden |
| `VIN` | — | 5 V Versorgung ausgeben |
| `3V3` | — | 3.3 V Versorgung ausgeben |
| `GND` | — | Masse |

<a id="max485-module-pins"></a>
### MAX485-Modul-Pins

TTL-Stiftleiste (von oben nach unten) am DollaTek-Board:

| Pin | Name | Funktion |
|-----|------|-----------|
| 1 | VCC | Versorgung (5 V) |
| 2 | GND | Masse |
| 3 | DI | Treiber-Eingang (TTL TX → RS-485) |
| 4 | DE | Treiber aktivieren (aktiv high) |
| 5 | RE | Empfänger aktivieren (aktiv low) |
| 6 | RO | Empfänger-Ausgang (RS-485 → TTL RX) |

Die grüne Schraubklemme ist der RS-485-Bus mit **A** und **B**.

> Die A/B-Beschriftung ist je nach Hersteller unterschiedlich. Wenn nichts funktioniert, A und B tauschen.

<a id="xlr-3-pinout"></a>
### XLR-3-Pinbelegung

| XLR-3 Pin | Signal |
|-----------|--------|
| 1 | Schirm / Signalmasse |
| 2 | Data− (DMX−) |
| 3 | Data+ (DMX+) |

<a id="sender-box--dmx-in-to-wireless"></a>
### Senderbox — DMX IN zu Wireless

Der Sender liest DMX über eine XLR-3-Buchse ein, setzt den MAX485 auf **Empfang**, und übergibt die Daten an den ESP32.

#### Komplette Sender-Verdrahtung (Pin für Pin)

| Sender-Seite | Verbinden mit |
|-------------|------------|
| ESP32 `VIN` | MAX485 `VCC` |
| ESP32 `GND` | MAX485 `GND` |
| ESP32 `RX2` (GPIO16) | Mittelpunkt des Teilers (zwischen 1.8 kΩ und 3.3 kΩ) |
| MAX485 `RO` | 1.8 kΩ Widerstand zum Teiler-Mittelpunkt |
| Teiler-Mittelpunkt | 3.3 kΩ Widerstand zu ESP32 `GND` |
| MAX485 `DE` | ESP32 `GND` |
| MAX485 `RE` | ESP32 `GND` |
| XLR female Pin 1 | ESP32 `GND` und MAX485 Bus-Masse |
| XLR female Pin 2 (Data−) | MAX485 `B` |
| XLR female Pin 3 (Data+) | MAX485 `A` |

#### UART-Einstellungen (DMX512)

| Parameter | Wert |
|-----------|-------|
| Baudrate | 250 000 |
| Datenbits | 8 |
| Parität | Keine |
| Stoppbits | 2 |

#### Versorgung
- MAX485 **VCC** → ESP32 **VIN (5 V)**
- MAX485 **GND** → ESP32 **GND**

#### Modus-Pins (Empfangsmodus: Treiber aus, Empfänger an)
- MAX485 **DE** → **GND**
- MAX485 **RE** → **GND**

#### Datenleitung — Pegelanpassung RO → RX2

Da MAX485 mit 5 V versorgt wird, kann **RO** bis etwa 5 V ausgeben. ESP32-GPIO darf maximal **3.3 V** sehen. Daher ist ein Spannungsteiler Pflicht:

```
MAX485 RO ---[ 1.8 kΩ ]---+--- ESP32 RX2 (GPIO16)
                          |
                        [ 3.3 kΩ ]
                          |
                         GND
```

- MAX485 **RO** → 1.8 kΩ → **ESP32 RX2 (GPIO16)**
- ESP32 **RX2 (GPIO16)** → 3.3 kΩ → **GND**

Widerstände sind unpolarisiert, die Einbaurichtung ist egal.

#### DMX IN — XLR-3 zu MAX485
- XLR **Pin 1** → **GND**
- XLR **Pin 2 (Data−)** → MAX485 **B**
- XLR **Pin 3 (Data+)** → MAX485 **A**

Falls kein DMX empfangen wird: A und B tauschen.

<a id="receiver-box--wireless-to-dmx-out"></a>
### Empfängerbox — Wireless zu DMX OUT

Der Empfänger bekommt Funkframes vom Sender-ESP32 und gibt daraus DMX über eine XLR-3-Steckerseite aus.

#### Komplette Empfänger-Verdrahtung (Pin für Pin)

| Empfänger-Seite | Verbinden mit |
|---------------|------------|
| ESP32 `VIN` | MAX485 `VCC` |
| ESP32 `GND` | MAX485 `GND` |
| ESP32 `TX2` (GPIO17) | MAX485 `DI` |
| ESP32 `3V3` | MAX485 `DE` |
| ESP32 `3V3` | MAX485 `RE` |
| XLR male Pin 1 | ESP32 `GND` und MAX485 Bus-Masse |
| XLR male Pin 2 (Data−) | MAX485 `B` |
| XLR male Pin 3 (Data+) | MAX485 `A` |

#### UART-Einstellungen

Wie beim Sender: **250 000 8N2**.

#### Versorgung
- MAX485 **VCC** → ESP32 **VIN (5 V)**
- MAX485 **GND** → ESP32 **GND**

#### Modus-Pins (Sendemodus: Treiber an, Empfänger aus)
- MAX485 **DE** → ESP32 **3V3**
- MAX485 **RE** → ESP32 **3V3**

Du kannst **DE** und **RE** verbinden und gemeinsam auf **3V3** legen.

#### Datenleitung
- ESP32 **TX2 (GPIO17)** → MAX485 **DI**

Keine Pegelanpassung nötig: 3.3 V von TX2 sind für MAX485 DI passend.

#### DMX OUT — XLR-3 zu MAX485
- XLR **Pin 1** → **GND**
- XLR **Pin 2 (Data−)** → MAX485 **B**
- XLR **Pin 3 (Data+)** → MAX485 **A**

<a id="termination-decoupling-and-protection"></a>
### Abschluss, Entkopplung und Schutz

#### Abschlusswiderstand (empfohlen, nur Empfänger)
Setze **120 Ω zwischen A und B** am Leitungsende. Beim DMX-OUT-Empfänger am besten schaltbar (Jumper oder Schalter), weil nur das **letzte Gerät** in der Linie terminiert werden soll.

#### Entkopplungskondensatoren (beide Boxen)
Direkt nahe am MAX485 zwischen **VCC** und **GND** platzieren:

- **0.1 µF (100 nF) Keramik**: ein Bein an 5 V, das andere an GND
- **10 µF Elektrolyt**: **+** an 5 V (VCC), **−** an GND (Markierung beachten)

Beide Kondensatoren liegen **parallel** zwischen VCC und GND. Das reduziert Versorgungsrauschen und verhindert Resets.

#### Optionale Serienwiderstände
Bei längeren Leitungen helfen **33–68 Ω** in Serie zu **A** und **B**, möglichst nahe an der Schraubklemme.

#### Optionale TVS-Diode
Für bessere ESD-/Hotplug-Festigkeit kann ein RS-485-TVS-Diodenarray zwischen **A** und **B** eingesetzt werden.

<a id="fully-functional-wireless-code-arduino"></a>
### Voll funktionsfähiger Wireless-Code (Arduino)

Nutze diese vollständigen Beispiele:

- **Sender (DMX IN → ESP-NOW):**  
  [`examples/Arduino_WirelessDMXSender/Arduino_WirelessDMXSender.ino`](examples/Arduino_WirelessDMXSender/Arduino_WirelessDMXSender.ino)
- **Empfänger (ESP-NOW → DMX OUT):**  
  [`examples/Arduino_WirelessDMXReceiver/Arduino_WirelessDMXReceiver.ino`](examples/Arduino_WirelessDMXReceiver/Arduino_WirelessDMXReceiver.ino)

Wichtige Setup-Hinweise:
- Beide Sketches müssen denselben `ESPNOW_CHANNEL` verwenden.
- Die MAC-Adresse des Empfänger-Boards in `RECEIVER_MAC` im Sender-Sketch eintragen.
- UART-Verdrahtung auf **250000 8N2** belassen.
- Erst Empfänger-Sketch, dann Sender-Sketch flashen.

<a id="isolation-note"></a>
### Hinweis zur Isolation

Isolierband schützt vor Kurzschluss, ist aber **keine galvanische Isolation**. ANSI-ESTA E1.11 DMX512-A fordert eine galvanische Trennung zum DMX-Bus.

Für Bühne/Produktion:
- Digitalisolator (z. B. ISO7221) oder Optokoppler auf den UART-Leitungen, **und**
- Isolierter DC-DC-Wandler für die RS-485-Seite, **oder**
- Ein vollständig isoliertes RS-485-Transceiver-Modul verwenden.

<a id="to-do"></a>
## Aufgabenliste

Eine Liste geplanter Funktionen findest du auf der Seite [esp_dmx GitHub Projects](https://github.com/users/someweisguy/projects/5).

<a id="appendix"></a>
## Anhang

<a id="command-classes"></a>
### Befehlsklassen

Die Befehlsklasse legt die Aktion der RDM-Nachricht fest. Responder müssen immer auf `RDM_CC_GET_COMMAND`- und `RDM_CC_SET_COMMAND`-Nachrichten antworten, außer wenn die Ziel-UID eine Broadcast-Adresse ist. Auf per Broadcast adressierte Befehle sollen Responder nicht antworten, um Kollisionen zu vermeiden.

- `RDM_CC_DISC_COMMAND` Das Paket ist ein RDM-Discovery-Befehl.
- `RDM_CC_DISC_COMMAND_RESPONSE` Das Paket ist eine Antwort auf einen RDM-Discovery-Befehl.
- `RDM_CC_GET_COMMAND` Das Paket ist eine RDM-GET-Anfrage.
- `RDM_CC_GET_COMMAND_RESPONSE` Das Paket ist eine Antwort auf eine RDM-GET-Anfrage.
- `RDM_CC_SET_COMMAND` Das Paket ist eine RDM-SET-Anfrage.
- `RDM_CC_SET_COMMAND_RESPONSE` Das Paket ist eine Antwort auf eine RDM-SET-Anfrage.

<a id="nack-reason-codes"></a>
### NACK-Grundcodes

Der NACK-Grund beschreibt, warum der Responder die Anfrage nicht ausführen kann.

- `RDM_NR_UNKNOWN_PID` Der Responder kann die Anfrage nicht erfüllen, weil diese Nachricht im Responder nicht implementiert ist.
- `RDM_NR_FORMAT_ERROR` Der Responder kann die Anfrage nicht interpretieren, da die Controller-Daten nicht korrekt formatiert sind.
- `RDM_NR_HARDWARE_FAULT` Der Responder kann die Anfrage wegen eines internen Hardwarefehlers nicht erfüllen.
- `RDM_NR_PROXY_REJECT` Der Proxy ist nicht der RDM-Line-Master und kann die Nachricht nicht erfüllen.
- `RDM_NR_WRITE_PROTECT` SET-Befehl ist grundsätzlich erlaubt, aktuell aber blockiert.
- `RDM_NR_UNSUPPORTED_COMMAND_CLASS` Für die angeforderte Befehlsklasse ungültig. Kann verwendet werden, wenn GET erlaubt ist, SET aber nicht unterstützt wird.
- `RDM_NR_DATA_OUT_OF_RANGE` Wert für den angegebenen Parameter liegt außerhalb des zulässigen Bereichs oder wird nicht unterstützt.
- `RDM_NR_BUFFER_FULL` Im Buffer bzw. in der Queue ist aktuell kein freier Speicher für Daten vorhanden.
- `RDM_NR_PACKET_SIZE_UNSUPPORTED` Eingehende Nachricht überschreitet die Buffer-Kapazität.
- `RDM_NR_SUB_DEVICE_OUT_OF_RANGE` Subgerät liegt außerhalb des zulässigen Bereichs oder ist unbekannt.
- `RDM_NR_PROXY_BUFFER_FULL` Der Proxy-Buffer ist voll und kann keine weiteren Queue- oder Statusmeldungsantworten speichern.

<a id="parameter-ids"></a>
### Parameter-IDs

Die folgende Tabelle listet die im RDM-Standard definierten Parameter-IDs auf. Parameter mit GET- oder SET-Unterstützung sind entsprechend markiert. Erforderliche Parameter werden vom DMX-Treiber automatisch registriert, sofern ausreichend Parameterspeicher vorhanden ist. Aktuell von dieser Bibliothek unterstützte PIDs sind in der Spalte "Unterstützt" mit der frühesten unterstützenden Bibliotheksversion angegeben.

Parameter                                   | GET | SET |Unterstützt|Hinweise|
:-------------------------------------------|:---:|:---:|:-------:|:----|
`RDM_PID_DISC_UNIQUE_BRANCH`                | | |v3.1.0|Muss an alle Geräte gebroadcastet werden. Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_DISC_MUTE`                         | | |v3.1.0|Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_DISC_UN_MUTE`                      | | |v3.1.0|Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_PROXIED_DEVICES`                   |✔️| |      |Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_PROXIED_DEVICE_COUNT`              |✔️| |      |Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_COMMS_STATUS`                      |✔️|✔️|      |Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_QUEUED_MESSAGE`                    |✔️| |v4.0.0|Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_STATUS_MESSAGE`                    |✔️| |      |Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_STATUS_ID_DESCRIPTION`             |✔️| |      |Muss an das Root-Subgerät gesendet werden.|
`RDM_PID_CLEAR_STATUS_ID`                   | |✔️|      | |
`RDM_PID_SUB_DEVICE_STATUS_REPORT_THRESHOLD`|✔️|✔️|      |Darf **nicht** an das Root-Subgerät gesendet werden.|
`RDM_PID_SUPPORTED_PARAMETERS`              |✔️| |v4.0.0|Nur erforderlich, wenn Parameter über den minimal erforderlichen Satz hinaus unterstützt werden.|
`RDM_PID_PARAMETER_DESCRIPTION`             |✔️| |v4.0.0|Muss an das Root-Subgerät gesendet werden. Unterstützung erforderlich für herstellerspezifische PIDs in `RDM_PID_SUPPORTED_PARAMETERS`.|
`RDM_PID_DEVICE_INFO`                       |✔️| |v3.1.0| |
`RDM_PID_PRODUCT_DETAIL_ID_LIST`            |✔️| |      | |
`RDM_PID_DEVICE_MODEL_DESCRIPTION`          |✔️| |v4.1.0| |
`RDM_PID_MANUFACTURER_LABEL`                |✔️| |v4.0.0| |
`RDM_PID_DEVICE_LABEL`                      |✔️|✔️|v3.1.0| |
`RDM_PID_FACTORY_DEFAULTS`                  |✔️|✔️|      | |
`RDM_PID_LANGUAGE_CAPABILITIES`             |✔️| |      | |
`RDM_PID_LANGUAGE`                          |✔️|✔️|v4.1.0| |
`RDM_PID_SOFTWARE_VERSION_LABEL`            |✔️| |v3.1.0| |
`RDM_PID_BOOT_SOFTWARE_VERSION_ID`          |✔️| |      | |
`RDM_PID_BOOT_SOFTWARE_VERSION_LABEL`       |✔️| |      | |
`RDM_PID_DMX_PERSONALITY`                   |✔️|✔️|      | |
`RDM_PID_DMX_PERSONALITY_DESCRIPTION`       |✔️| |      | |
`RDM_PID_DMX_START_ADDRESS`                 |✔️|✔️|v3.1.0|Erforderlich, wenn das Gerät einen DMX-Slot verwendet.|
`RDM_PID_SLOT_INFO`                         |✔️| |      | |
`RDM_PID_SLOT_DESCRIPTION`                  |✔️| |      | |
`RDM_PID_DEFAULT_SLOT_VALUE`                |✔️| |      | |
`RDM_PID_SENSOR_DEFINITION`                 |✔️| |v4.1.0| |
`RDM_PID_SENSOR_VALUE`                      |✔️|✔️|v4.0.0| |
`RDM_PID_RECORD_SENSORS`                    | |✔️|v4.0.0| |
`RDM_PID_DEVICE_HOURS`                      |✔️|✔️|v4.1.0|Manche Geräte unterstützen evtl. keine RDM-SET-Anfragen. SET-Unterstützung kann über ESP-IDF-Kconfig deaktiviert sein.|
`RDM_PID_LAMP_HOURS`                        |✔️|✔️|v4.1.0| |
`RDM_PID_LAMP_STRIKES`                      |✔️|✔️|      | |
`RDM_PID_LAMP_STATE`                        |✔️|✔️|      | |
`RDM_PID_LAMP_ON_MODE`                      |✔️|✔️|      | |
`RDM_PID_DEVICE_POWER_CYCLES`               |✔️|✔️|      | |
`RDM_PID_DISPLAY_INVERT`                    |✔️|✔️|      | |
`RDM_PID_DISPLAY_LEVEL`                     |✔️|✔️|      | |
`RDM_PID_PAN_INVERT`                        |✔️|✔️|      | |
`RDM_PID_TILT_INVERT`                       |✔️|✔️|      | |
`RDM_PID_PAN_TILT_SWAP`                     |✔️|✔️|      | |
`RDM_PID_REAL_TIME_CLOCK`                   |✔️|✔️|      | |
`RDM_PID_IDENTIFY_DEVICE`                   |✔️|✔️|v3.1.0| |
`RDM_PID_RESET_DEVICE`                      | |✔️|v4.1.0| |
`RDM_PID_POWER_STATE`                       |✔️|✔️|      | |
`RDM_PID_PERFORM_SELFTEST`                  |✔️|✔️|      | |
`RDM_PID_SELF_TEST_DESCRIPTION`             |✔️| |      | |
`RDM_PID_CAPTURE_PRESET`                    | |✔️|      | |
`RDM_PID_PRESET_PLAYBACK`                   |✔️|✔️|      | |

<a id="product-categories"></a>
### Produktkategorien

Geräte sollen eine Produktkategorie entsprechend der primären Funktion des Produkts melden.

- `RDM_PRODUCT_CATEGORY_NOT_DECLARED` Die Produktkategorie ist nicht deklariert.
- `RDM_PRODUCT_CATEGORY_FIXTURE` Das Produkt ist ein Leuchtmittel/Fixture zur Erzeugung von Beleuchtung.
- `RDM_PRODUCT_CATEGORY_FIXTURE_ACCESSORY` Das Produkt ist ein Zubehörteil für ein Fixture oder einen Projektor.
- `RDM_PRODUCT_CATEGORY_PROJECTOR` Das Produkt ist eine Lichtquelle, die realistische Bilder aus einem anderen Medium erzeugen kann.
- `RDM_PRODUCT_CATEGORY_ATMOSPHERIC` Das Produkt erzeugt atmosphärische Effekte wie Haze, Nebel oder Pyrotechnik.
- `RDM_PRODUCT_CATEGORY_DIMMER` Das Produkt dient der Intensitätssteuerung, insbesondere Dimmtechnik.
- `RDM_PRODUCT_CATEGORY_POWER` Das Produkt dient der Leistungs-/Stromsteuerung außerhalb von Dimmtechnik.
- `RDM_PRODUCT_CATEGORY_SCENIC` Das Produkt ist ein szenisches Gerät ohne direkten Bezug zur Lichttechnik.
- `RDM_PRODUCT_CATEGORY_DATA` Das Produkt ist ein DMX-Konverter, Interface oder sonstiger Teil der DMX-Infrastruktur.
- `RDM_PRODUCT_CATEGORY_AV` Das Produkt ist Audio-/Video-Equipment.
- `RDM_PRODUCT_CATEGORY_MONITOR` Das Produkt ist Überwachungsequipment.
- `RDM_PRODUCT_CATEGORY_CONTROL` Das Produkt ist ein Controller- oder Backup-Gerät.
- `RDM_PRODUCT_CATEGORY_TEST` Das Produkt ist Testequipment.
- `RDM_PRODUCT_CATEGORY_OTHER` Das Produkt wird durch keine der anderen Produktkategorien beschrieben.

<a id="response-types"></a>
### Antworttypen

Antwortende Geräte sollen nur auf Anfragen reagieren, wenn es sich nicht um Broadcast-Anfragen handelt. Antwortende Geräte können mit folgenden Antworttypen reagieren:

- `RDM_RESPONSE_TYPE_ACK` zeigt an, dass der Responder die Controller-Nachricht korrekt empfangen hat und die Anfrage verarbeitet.
- `RDM_RESPONSE_TYPE_ACK_OVERFLOW` zeigt an, dass der Responder die Anfrage verarbeitet, aber mehr Antwortdaten vorliegen, als in ein einzelnes Antwortpaket passen. Um die restlichen Informationen zu erhalten, kann der Controller wiederholt dieselbe PID anfragen, bis alles in eine einzelne Nachricht passt.
- `RDM_RESPONSE_TYPE_ACK_TIMER` zeigt an, dass der Responder die angeforderten GET-Informationen oder SET-Bestätigung nicht innerhalb der geforderten Antwortzeit liefern kann. In dieser Antwort gibt das Gerät eine geschätzte Wartezeit an, nach der die benötigte Information bereitsteht.
- `RDM_RESPONSE_TYPE_NACK_REASON` zeigt an, dass der Responder die angeforderten GET-Informationen nicht liefern oder den angegebenen SET-Befehl nicht verarbeiten kann. Die Antwort muss einen NACK-Grundcode enthalten. NACK-Grundcodes sind im [Anhang](#nack-reason-codes) aufgeführt.
- `RDM_RESPONSE_TYPE_NONE` zeigt an, dass keine Antwort empfangen wurde.
- `RDM_RESPONSE_TYPE_INVALID` zeigt an, dass eine Antwort empfangen wurde, diese aber ungültig war. Das kann z. B. durch eine ungültige Prüfsumme oder ein ungültiges Paketformat auftreten.
