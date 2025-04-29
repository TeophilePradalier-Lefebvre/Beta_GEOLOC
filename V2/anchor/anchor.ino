/*

For ESP32 UWB or ESP32 UWB Pro

*/

#include <SPI.h>
#include "DW1000Ranging.h"

#define ANCHOR_ADD "86:17:5B:D5:A9:9A:E2:9C"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 21;   // spi select pin

// Configurations pour le filtrage des mesures
#define RANGE_BUFFER_SIZE 10    // Taille du buffer pour stocker les mesures
#define OUTLIER_THRESHOLD 0.3   // Seuil pour détecter les valeurs aberrantes (en mètres)
#define MIN_SAMPLES 5           // Nombre minimum d'échantillons pour calculer une moyenne
#define PRINT_INTERVAL 500      // Intervalle d'affichage en ms (limiter le débit de données)

// Variables pour le filtrage
float rangeBuffer[RANGE_BUFFER_SIZE];  // Buffer circulaire pour stocker les distances
int bufferIndex = 0;                   // Index actuel dans le buffer
int samplesCount = 0;                  // Nombre de mesures stockées
uint16_t lastDeviceAddress = 0;        // Adresse du dernier dispositif
unsigned long lastPrintTime = 0;       // Moment du dernier affichage

// Variables pour les statistiques
float filteredRange = 0;               // Mesure filtrée actuelle

void setup()
{
    Serial.begin(115200);
    delay(1000);
    //init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
    //define the sketch as anchor. It will be great to dynamically change the type of module
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachBlinkDevice(newBlink);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    //we start the module as an anchor
    DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
}

void loop()
{
    DW1000Ranging.loop();
}

// Fonction pour stocker une nouvelle mesure de distance
void storeRange(uint16_t deviceAddress, float range) {
    // Si c'est un nouveau dispositif, réinitialiser le buffer
    if (deviceAddress != lastDeviceAddress) {
        lastDeviceAddress = deviceAddress;
        bufferIndex = 0;
        samplesCount = 0;
        
        // Réinitialiser le buffer
        for (int i = 0; i < RANGE_BUFFER_SIZE; i++) {
            rangeBuffer[i] = 0;
        }
    }
    
    // Stocker la nouvelle mesure
    rangeBuffer[bufferIndex] = range;
    bufferIndex = (bufferIndex + 1) % RANGE_BUFFER_SIZE;
    samplesCount = min(samplesCount + 1, RANGE_BUFFER_SIZE);
}

// Fonction pour calculer une mesure filtrée à partir du buffer
float calculateFilteredRange() {
    if (samplesCount < MIN_SAMPLES) {
        // Pas assez d'échantillons, retourner la dernière mesure
        return rangeBuffer[(bufferIndex - 1 + RANGE_BUFFER_SIZE) % RANGE_BUFFER_SIZE];
    }
    
    // Copier les mesures dans un tableau temporaire pour le tri
    float tempBuffer[RANGE_BUFFER_SIZE];
    for (int i = 0; i < samplesCount; i++) {
        tempBuffer[i] = rangeBuffer[i];
    }
    
    // Tri simple pour trouver la médiane
    for (int i = 0; i < samplesCount - 1; i++) {
        for (int j = i + 1; j < samplesCount; j++) {
            if (tempBuffer[i] > tempBuffer[j]) {
                float temp = tempBuffer[i];
                tempBuffer[i] = tempBuffer[j];
                tempBuffer[j] = temp;
            }
        }
    }
    
    // Calculer la médiane
    float median;
    if (samplesCount % 2 == 0) {
        median = (tempBuffer[samplesCount/2] + tempBuffer[(samplesCount/2) - 1]) / 2.0;
    } else {
        median = tempBuffer[samplesCount/2];
    }
    
    // Calculer la moyenne en ignorant les valeurs aberrantes
    float sum = 0;
    int count = 0;
    
    for (int i = 0; i < samplesCount; i++) {
        if (abs(rangeBuffer[i] - median) <= OUTLIER_THRESHOLD) {
            sum += rangeBuffer[i];
            count++;
        }
    }
    
    if (count > 0) {
        return sum / count;
    } else {
        return median; // Si toutes les valeurs sont aberrantes, utiliser la médiane
    }
}

void newRange()
{
    // Obtenir l'adresse du dispositif et la mesure de distance
    uint16_t deviceAddress = DW1000Ranging.getDistantDevice()->getShortAddress();
    float range = DW1000Ranging.getDistantDevice()->getRange();
    float rxPower = DW1000Ranging.getDistantDevice()->getRXPower();
    
    // Stocker la mesure dans le buffer
    storeRange(deviceAddress, range);
    
    // Calculer la mesure filtrée
    filteredRange = calculateFilteredRange();
    
    // Limiter la fréquence d'affichage pour réduire le bruit dans la console
    unsigned long currentTime = millis();
    if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
        lastPrintTime = currentTime;
        
        // Afficher les informations
        Serial.print("from: ");
        Serial.print(deviceAddress, HEX);
        Serial.print("\t Range (raw): ");
        Serial.print(range);
        Serial.print(" m");
        Serial.print("\t Range (filtered): ");
        Serial.print(filteredRange, 2);  // 2 décimales
        Serial.print(" m");
        Serial.print("\t RX power: ");
        Serial.print(rxPower);
        Serial.print(" dBm");
        Serial.print("\t Samples: ");
        Serial.println(samplesCount);
    }
}

void newBlink(DW1000Device *device)
{
    Serial.print("blink; 1 device added ! -> ");
    Serial.print(" short:");
    Serial.println(device->getShortAddress(), HEX);
    
    // Réinitialiser le buffer quand un nouveau dispositif est détecté
    lastDeviceAddress = device->getShortAddress();
    bufferIndex = 0;
    samplesCount = 0;
}

void inactiveDevice(DW1000Device *device)
{
    Serial.print("delete inactive device: ");
    Serial.println(device->getShortAddress(), HEX);
    
    // Si le dispositif inactif est celui que nous suivions, réinitialiser
    if (device->getShortAddress() == lastDeviceAddress) {
        lastDeviceAddress = 0;
        bufferIndex = 0;
        samplesCount = 0;
    }
}