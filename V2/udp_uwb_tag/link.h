#include <Arduino.h>

// Configuration du buffer pour le filtrage
#define SAMPLE_WINDOW_SIZE 10

struct MyLink
{
    uint16_t anchor_addr;
    float range[3];           // Valeurs filtrées (pour compatibilité)
    float range_buffer[SAMPLE_WINDOW_SIZE]; // Buffer pour stocker les mesures brutes
    int samples_count;        // Nombre d'échantillons valides dans le buffer
    float dbm;                // Puissance du signal reçu
    struct MyLink *next;      // Pointeur vers le prochain dispositif
};

struct MyLink *init_link();
void add_link(struct MyLink *p, uint16_t addr);
struct MyLink *find_link(struct MyLink *p, uint16_t addr);
void fresh_link(struct MyLink *p, uint16_t addr, float range, float dbm);
void print_link(struct MyLink *p);
void delete_link(struct MyLink *p, uint16_t addr);
void make_link_json(struct MyLink *p, String *s);

// Nouvelles fonctions pour le filtrage avancé
void store_measurement(struct MyLink *p, uint16_t addr, float range, float dbm);
void filter_device_measurements(struct MyLink *device);
void filter_all_measurements(struct MyLink *p);