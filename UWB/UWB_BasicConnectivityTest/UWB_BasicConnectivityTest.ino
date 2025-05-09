// UWB projet Localisation
// Olivier CONET
// source https://github.com/Makerfabs/Makerfabs-ESP32-UWB/tree/main
// Version 21 mai 2025
// Resultat : Testé OK avec module afficheur et OK avec module sans afficheur !

// Remarques :
// ESP32 UWB : fonctionne OK
// #define PIN_SS 4
// #define PIN_RST 27
// #define PIN_IRQ 34

// ESP32 Pro with display : fonctionne OK
// #define PIN_SS 21   // spi select pin
// #define PIN_RST 27  // reset pin
// #define PIN_IRQ 34  // irq pin




/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file BasicConnectivityTest.ino
 * Use this to test connectivity with your DW1000 from Arduino.
 * It performs an arbitrary setup of the chip and prints some information.
 * 
 * @todo
 *  - move strings to flash (less RAM consumption)
 *  - make real check of connection (e.g. change some values on DW1000 and verify)
 */

#include <SPI.h>
#include <DW1000.h>

// connection pins
// ESP32 UWB and ESP32 UWB Pro
#define PIN_SS 4
#define PIN_RST 27
#define PIN_IRQ 34

// ESP32 Pro with display
// #define UWB_SS 21   // spi select pin
// #define UWB_RST 27  // reset pin
// #define UWB_IRQ 34  // irq pin

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);
  // initialize the driver
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  Serial.println(F("DW1000 initialized ..."));
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDeviceAddress(5);
  DW1000.setNetworkId(10);
  DW1000.commitConfiguration();
  Serial.println(F("Committed configuration ..."));
  // wait a bit
  delay(1000);
}

void loop() {
  // DEBUG chip info and registers pretty printed
  char msg[128];
  DW1000.getPrintableDeviceIdentifier(msg);
  Serial.print("Device ID: "); Serial.println(msg);
  DW1000.getPrintableExtendedUniqueIdentifier(msg);
  Serial.print("Unique ID: "); Serial.println(msg);
  DW1000.getPrintableNetworkIdAndShortAddress(msg);
  Serial.print("Network ID & Device Address: "); Serial.println(msg);
  DW1000.getPrintableDeviceMode(msg);
  Serial.print("Device mode: "); Serial.println(msg);
  // wait a bit
  delay(10000);
}
