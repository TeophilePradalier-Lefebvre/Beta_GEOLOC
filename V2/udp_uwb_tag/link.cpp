#include "link.h"

//#define SERIAL_DEBUG

struct MyLink *init_link()
{
#ifdef SERIAL_DEBUG
    Serial.println("init_link");
#endif
    struct MyLink *p = (struct MyLink *)malloc(sizeof(struct MyLink));
    p->next = NULL;
    p->anchor_addr = 0;
    p->range[0] = 0.0;
    p->range[1] = 0.0;
    p->range[2] = 0.0;
    
    // Initialisation des nouveaux champs
    for (int i = 0; i < SAMPLE_WINDOW_SIZE; i++) {
        p->range_buffer[i] = 0.0;
    }
    p->samples_count = 0;
    p->dbm = 0.0;

    return p;
}

void add_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("add_link");
#endif
    struct MyLink *temp = p;
    //Find struct MyLink end
    while (temp->next != NULL)
    {
        temp = temp->next;
    }

    Serial.println("add_link:find struct MyLink end");
    //Create a anchor
    struct MyLink *a = (struct MyLink *)malloc(sizeof(struct MyLink));
    a->anchor_addr = addr;
    a->range[0] = 0.0;
    a->range[1] = 0.0;
    a->range[2] = 0.0;
    a->dbm = 0.0;
    
    // Initialisation des nouveaux champs
    for (int i = 0; i < SAMPLE_WINDOW_SIZE; i++) {
        a->range_buffer[i] = 0.0;
    }
    a->samples_count = 0;
    
    a->next = NULL;

    //Add anchor to end of struct MyLink
    temp->next = a;

    return;
}

struct MyLink *find_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("find_link");
#endif
    if (addr == 0)
    {
        Serial.println("find_link:Input addr is 0");
        return NULL;
    }

    if (p->next == NULL)
    {
        Serial.println("find_link:Link is empty");
        return NULL;
    }

    struct MyLink *temp = p;
    //Find target struct MyLink or struct MyLink end
    while (temp->next != NULL)
    {
        temp = temp->next;
        if (temp->anchor_addr == addr)
        {
            // Serial.println("find_link:Find addr");
            return temp;
        }
    }

    Serial.println("find_link:Can't find addr");
    return NULL;
}

// Cette fonction est conservée pour la compatibilité avec le code existant
// mais elle utilise maintenant store_measurement et filter_device_measurements en interne
void fresh_link(struct MyLink *p, uint16_t addr, float range, float dbm)
{
#ifdef SERIAL_DEBUG
    Serial.println("fresh_link");
#endif
    // Stocke la mesure dans le buffer
    store_measurement(p, addr, range, dbm);
    
    // Trouve le dispositif correspondant
    struct MyLink *temp = find_link(p, addr);
    if (temp != NULL)
    {
        // Applique le filtrage pour mettre à jour les valeurs range[0], range[1], range[2]
        filter_device_measurements(temp);
        return;
    }
    else
    {
        Serial.println("fresh_link:Fresh fail");
        return;
    }
}

void print_link(struct MyLink *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("print_link");
#endif
    struct MyLink *temp = p;

    while (temp->next != NULL)
    {
        Serial.println(temp->next->anchor_addr, HEX);
        Serial.println(temp->next->range[0]);
        Serial.println(temp->next->dbm);
        
        // Afficher également les données brutes dans le buffer
        Serial.print("Raw buffer: ");
        for (int i = 0; i < min(temp->next->samples_count, SAMPLE_WINDOW_SIZE); i++) {
            Serial.print(temp->next->range_buffer[i]);
            Serial.print(" ");
        }
        Serial.println();
        
        temp = temp->next;
    }

    return;
}

void delete_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("delete_link");
#endif
    if (addr == 0)
        return;

    struct MyLink *temp = p;
    while (temp->next != NULL)
    {
        if (temp->next->anchor_addr == addr)
        {
            struct MyLink *del = temp->next;
            temp->next = del->next;
            free(del);
            return;
        }
        temp = temp->next;
    }
    return;
}

void make_link_json(struct MyLink *p, String *s)
{
#ifdef SERIAL_DEBUG
    Serial.println("make_link_json");
#endif
    *s = "{\"links\":[";
    struct MyLink *temp = p;

    while (temp->next != NULL)
    {
        temp = temp->next;
        char link_json[80];  // Augmenté pour plus d'informations
        
        // Format avec 2 décimales pour la précision et ajout d'informations de qualité
        int sample_quality = (temp->samples_count >= MIN_SAMPLES_FOR_AVERAGE) ? 100 : (temp->samples_count * 100 / MIN_SAMPLES_FOR_AVERAGE);
        sprintf(link_json, "{\"A\":\"%X\",\"R\":\"%.2f\",\"Q\":\"%d\",\"S\":\"%d\"}", 
                temp->anchor_addr, 
                temp->range[0],
                sample_quality,
                temp->samples_count);
                
        *s += link_json;
        if (temp->next != NULL)
        {
            *s += ",";
        }
    }
    *s += "]}";
    Serial.println(*s);
}