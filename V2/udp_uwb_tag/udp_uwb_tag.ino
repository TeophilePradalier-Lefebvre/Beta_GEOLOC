/*

For ESP32 UWB or ESP32 UWB Pro

*/

#include <SPI.h>
#include <DW1000Ranging.h>
#include <WiFi.h>
#include "link.h"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4
#define PIN_RST 27
#define PIN_IRQ 34

// Paramètres de filtrage
#define SAMPLE_WINDOW_SIZE 10      // Nombre de mesures à considérer pour la moyenne
#define OUTLIER_THRESHOLD 0.5      // Seuil pour détecter les valeurs aberrantes (en mètres)
#define MIN_SAMPLES_FOR_AVERAGE 5  // Nombre minimum de mesures pour calculer une moyenne
#define UWB_UPDATE_INTERVAL 2000   // Intervalle d'envoi des données en ms

const char *ssid = "JUNIA_LAB";
const char *password = "813nV3nue@2025!";
const char *host = "10.224.0.77";
WiFiClient client;

struct MyLink *uwb_data;
long runtime = 0;
String all_json = "";

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    if (client.connect(host, 80))
    {
        Serial.println("Success");
        client.print(String("GET /") + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: close\r\n" +
                     "\r\n");
    }

    delay(1000);

    //init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, DW_CS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    //we start the module as a tag
    DW1000Ranging.startAsTag("9C:24:7E:1B:F3:8D:C0:47", DW1000.MODE_LONGDATA_RANGE_LOWPOWER);

    uwb_data = init_link();
}

void loop()
{
    DW1000Ranging.loop();
    if ((millis() - runtime) > UWB_UPDATE_INTERVAL)
    {
        // Applique les filtres aux données avant de les envoyer
        filter_all_measurements(uwb_data);
        
        // Génère le JSON et l'envoie
        make_link_json(uwb_data, &all_json);
        send_udp(&all_json);
        runtime = millis();
    }
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
    
    // Ajout des nouvelles mesures à la structure de données
    store_measurement(uwb_data, 
                     DW1000Ranging.getDistantDevice()->getShortAddress(), 
                     DW1000Ranging.getDistantDevice()->getRange(), 
                     DW1000Ranging.getDistantDevice()->getRXPower());
}

void newDevice(DW1000Device *device)
{
    Serial.print("ranging init; 1 device added ! -> ");
    Serial.print(" short:");
    Serial.println(device->getShortAddress(), HEX);

    add_link(uwb_data, device->getShortAddress());
}

void inactiveDevice(DW1000Device *device)
{
    Serial.print("delete inactive device: ");
    Serial.println(device->getShortAddress(), HEX);

    delete_link(uwb_data, device->getShortAddress());
}

void send_udp(String *msg_json)
{
    if (client.connected())
    {
        client.print(*msg_json);
        Serial.println(*msg_json);
    }
    else
    {
        Serial.println("Client disconnected, attempting to reconnect...");
        if (client.connect(host, 80))
        {
            Serial.println("Reconnected successfully");
            client.print(*msg_json);
        }
        else
        {
            Serial.println("Reconnection failed");
        }
    }
}

// Fonction pour stocker les mesures brutes
void store_measurement(struct MyLink *p, uint16_t addr, float range, float dbm)
{
    struct MyLink *temp = find_link(p, addr);
    if (temp != NULL)
    {
        // Décale les mesures précédentes
        for (int i = SAMPLE_WINDOW_SIZE - 1; i > 0; i--)
        {
            temp->range_buffer[i] = temp->range_buffer[i-1];
        }
        
        // Stocke la nouvelle mesure
        temp->range_buffer[0] = range;
        temp->samples_count = min(temp->samples_count + 1, SAMPLE_WINDOW_SIZE);
        temp->dbm = dbm;  // Mise à jour de la puissance du signal
    }
}

// Fonction pour filtrer les mesures d'un dispositif
void filter_device_measurements(struct MyLink *device)
{
    if (device->samples_count < MIN_SAMPLES_FOR_AVERAGE)
    {
        // Pas assez d'échantillons, utilisez la dernière mesure
        device->range[0] = device->range_buffer[0];
        return;
    }
    
    // Calculer la médiane des mesures
    float sorted_ranges[SAMPLE_WINDOW_SIZE];
    for (int i = 0; i < device->samples_count; i++)
    {
        sorted_ranges[i] = device->range_buffer[i];
    }
    
    // Tri simple des mesures pour calculer la médiane
    for (int i = 0; i < device->samples_count - 1; i++)
    {
        for (int j = i + 1; j < device->samples_count; j++)
        {
            if (sorted_ranges[i] > sorted_ranges[j])
            {
                float temp = sorted_ranges[i];
                sorted_ranges[i] = sorted_ranges[j];
                sorted_ranges[j] = temp;
            }
        }
    }
    
    float median;
    if (device->samples_count % 2 == 0)
    {
        median = (sorted_ranges[device->samples_count/2] + sorted_ranges[device->samples_count/2 - 1]) / 2.0;
    }
    else
    {
        median = sorted_ranges[device->samples_count/2];
    }
    
    // Calculer la moyenne des mesures non aberrantes
    float sum = 0;
    int count = 0;
    
    for (int i = 0; i < device->samples_count; i++)
    {
        if (fabs(device->range_buffer[i] - median) <= OUTLIER_THRESHOLD)
        {
            sum += device->range_buffer[i];
            count++;
        }
    }
    
    // Mettre à jour la valeur filtrée
    if (count > 0)
    {
        // Moyenne des mesures non aberrantes
        device->range[0] = sum / count;
        
        // Mise à jour des autres valeurs dans le buffer de range pour la compatibilité
        device->range[1] = device->range[0];
        device->range[2] = device->range[0];
    }
    else
    {
        // Si toutes les mesures sont aberrantes, utiliser la médiane
        device->range[0] = median;
        device->range[1] = median;
        device->range[2] = median;
    }
    
    // Arrondir à 2 décimales pour plus de lisibilité
    device->range[0] = round(device->range[0] * 100) / 100.0;
}

// Fonction pour filtrer toutes les mesures
void filter_all_measurements(struct MyLink *p)
{
    struct MyLink *temp = p;
    
    while (temp->next != NULL)
    {
        temp = temp->next;
        filter_device_measurements(temp);
    }
}