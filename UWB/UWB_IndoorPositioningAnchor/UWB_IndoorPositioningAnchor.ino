// UWB projet Localisation
// Olivier CONET
// source https://github.com/Makerfabs/Makerfabs-ESP32-UWB/tree/main
// Version 21 mai 2025
// Resultat : Testé OK avec module sans afficheur !
// 3 anchors programmées : AA11, AA22 et AA33


/*

For ESP32 UWB or ESP32 UWB Pro

*/

#include <SPI.h>
#include "DW1000Ranging.h"

//#define ANCHOR_ADD "11:AA:5B:D5:A9:9A:E2:9C" // anchor AA11
//#define ANCHOR_ADD "22:AA:5B:D5:A9:9A:E2:9C" // anchor AA22
#define ANCHOR_ADD "33:AA:5B:D5:A9:9A:E2:9C" // anchor AA33

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
// ESP32 UWB and ESP32 UWB Pro : fonctionne OK
#define PIN_SS 4
#define PIN_RST 27
#define PIN_IRQ 34

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("Indoor positioning anchor");
    //init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
    //define the sketch as anchor. It will be great to dynamically change the type of module
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachBlinkDevice(newBlink);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    //Enable the filter to smooth the distance
    //DW1000Ranging.useRangeFilter(true);

    //we start the module as an anchor
    // DW1000Ranging.startAsAnchor("82:17:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);

    DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
    // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
    // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_FAST_LOWPOWER);
    // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_SHORTDATA_FAST_ACCURACY);
    // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_FAST_ACCURACY);
    // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_ACCURACY);
}

void loop()
{
    DW1000Ranging.loop();
}

void newRange()
{
    Serial.print("from: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
    Serial.print("\t Range: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getRange());
    Serial.print(" m");
    Serial.print("\t RX power: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
    Serial.println(" dBm");
}

void newBlink(DW1000Device *device)
{
    Serial.print("blink; 1 device added ! -> ");
    Serial.print(" short:");
    Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device *device)
{
    Serial.print("delete inactive device: ");
    Serial.println(device->getShortAddress(), HEX);
}