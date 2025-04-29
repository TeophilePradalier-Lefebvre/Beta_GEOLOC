import time
import turtle
import cmath
import socket
import json
import numpy as np
from collections import deque

# Configuration réseau
hostname = socket.gethostname()
UDP_IP = socket.gethostbyname(hostname)
print("***Local ip:" + str(UDP_IP) + "***")
UDP_PORT = 80
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((UDP_IP, UDP_PORT))
sock.listen(1)
data, addr = sock.accept()

# Configuration de l'affichage
distance_a1_a2 = 3.0  # Distance entre les ancres en mètres
meter2pixel = 100     # Facteur de conversion mètre -> pixel
range_offset = 0.9    # Compensation de distance

# Configuration du filtrage de position
POSITION_HISTORY_SIZE = 5  # Taille de l'historique des positions
position_history = deque(maxlen=POSITION_HISTORY_SIZE)


def screen_init(width=1200, height=800, t=turtle):
    t.setup(width, height)
    t.tracer(False)
    t.hideturtle()
    t.speed(0)


def turtle_init(t=turtle):
    t.hideturtle()
    t.speed(0)


def draw_line(x0, y0, x1, y1, color="black", t=turtle):
    t.pencolor(color)
    t.up()
    t.goto(x0, y0)
    t.down()
    t.goto(x1, y1)
    t.up()


def draw_fastU(x, y, length, color="black", t=turtle):
    draw_line(x, y, x, y + length, color, t)


def draw_fastV(x, y, length, color="black", t=turtle):
    draw_line(x, y, x + length, y, color, t)


def draw_cycle(x, y, r, color="black", t=turtle):
    t.pencolor(color)
    t.up()
    t.goto(x, y - r)
    t.setheading(0)
    t.down()
    t.circle(r)
    t.up()


def fill_cycle(x, y, r, color="black", t=turtle):
    t.up()
    t.goto(x, y)
    t.down()
    t.dot(r, color)
    t.up()


def write_txt(x, y, txt, color="black", t=turtle, f=('Arial', 12, 'normal')):
    t.pencolor(color)
    t.up()
    t.goto(x, y)
    t.down()
    t.write(txt, move=False, align='left', font=f)
    t.up()


def draw_rect(x, y, w, h, color="black", t=turtle):
    t.pencolor(color)
    t.up()
    t.goto(x, y)
    t.down()
    t.goto(x + w, y)
    t.goto(x + w, y + h)
    t.goto(x, y + h)
    t.goto(x, y)
    t.up()


def fill_rect(x, y, w, h, color=("black", "black"), t=turtle):
    t.begin_fill()
    draw_rect(x, y, w, h, color, t)
    t.end_fill()


def clean(t=turtle):
    t.clear()


def draw_ui(t):
    write_txt(-300, 250, "UWB Position (Filtered)", "black", t, f=('Arial', 32, 'normal'))
    fill_rect(-400, 200, 800, 40, "black", t)
    write_txt(-50, 205, "WALL", "yellow", t, f=('Arial', 24, 'normal'))
    
    # Ajouter une légende pour les statistiques
    write_txt(-400, -250, "Statistiques:", "black", t, f=('Arial', 18, 'normal'))
    write_txt(-400, -280, "Variance position: 0.00 m", "black", t, f=('Arial', 14, 'normal'))
    write_txt(-400, -300, "Qualité signal: 0%", "black", t, f=('Arial', 14, 'normal'))


def draw_uwb_anchor(x, y, txt, range, quality=0, t=turtle):
    r = 20
    fill_cycle(x, y, r, "green", t)
    write_txt(x + r, y, txt + ": " + str(range) + "m (Q:" + str(quality) + "%)",
              "black", t, f=('Arial', 16, 'normal'))


def draw_uwb_tag(x, y, txt, variance=0.0, t=turtle):
    pos_x = -250 + int(x * meter2pixel)
    pos_y = 150 - int(y * meter2pixel)
    r = 20
    
    # Dessiner un cercle de confiance basé sur la variance
    if variance > 0.01:
        confidence_radius = int(variance * meter2pixel * 2)  # 2x pour être plus visible
        t.pencolor("light blue")
        t.up()
        t.goto(pos_x, pos_y - confidence_radius)
        t.down()
        t.circle(confidence_radius)
        t.up()
    
    # Dessiner le tag
    fill_cycle(pos_x, pos_y, r, "blue", t)
    
    # Afficher les coordonnées avec la précision
    write_txt(pos_x + r, pos_y, txt + ": (" + str(round(x, 2)) + ", " + str(round(y, 2)) + ")",
              "black", t, f=('Arial', 16, 'normal'))
    
    # Mettre à jour les statistiques
    t_stats = turtle.Turtle()
    turtle_init(t_stats)
    t_stats.clear()
    write_txt(-400, -280, f"Variance position: {variance:.3f} m", "black", t_stats, f=('Arial', 14, 'normal'))


def read_data():
    try:
        line = data.recv(1024).decode('UTF-8')
        
        if not line:
            return []
            
        # Essayer de traiter le JSON
        try:
            uwb_data = json.loads(line)
            print(uwb_data)
            
            # Vérifier si les données sont au bon format
            if "links" not in uwb_data:
                print("Format JSON invalide: 'links' manquant")
                return []
                
            uwb_list = uwb_data["links"]
            for uwb_anchor in uwb_list:
                print(uwb_anchor)
                
            return uwb_list
            
        except json.JSONDecodeError:
            print("Erreur de décodage JSON:", line)
            return []
            
    except Exception as e:
        print(f"Erreur de lecture des données: {e}")
        return []


def tag_pos(a, b, c):
    """
    Calcule la position d'un tag en utilisant la trilateralization
    a: distance au deuxième anchor
    b: distance au premier anchor
    c: distance entre les deux anchors
    Retourne (x, y) - coordonnées en mètres
    """
    # Méthode utilisant la loi des cosinus
    try:
        cos_a = (b * b + c*c - a * a) / (2 * b * c)
        x = b * cos_a
        y = b * cmath.sqrt(1 - cos_a * cos_a)
        
        return round(x.real, 2), round(y.real, 2)
    except Exception as e:
        print(f"Erreur dans le calcul de position: {e}")
        # Retourner la dernière position valide si disponible
        if position_history:
            return position_history[-1]
        return 0.0, 0.0


def filter_position(new_x, new_y):
    """
    Filtre les positions pour réduire le bruit et détecter les valeurs aberrantes
    Retourne (x_filtered, y_filtered, variance)
    """
    # Ajouter la nouvelle position à l'historique
    position_history.append((new_x, new_y))
    
    # Si nous avons assez de données, filtrer
    if len(position_history) >= 3:
        # Extraire les coordonnées x et y séparément
        x_values = [pos[0] for pos in position_history]
        y_values = [pos[1] for pos in position_history]
        
        # Calculer les moyennes
        x_mean = sum(x_values) / len(x_values)
        y_mean = sum(y_values) / len(y_values)
        
        # Calculer la variance (mesure de stabilité)
        x_variance = sum((x - x_mean)**2 for x in x_values) / len(x_values)
        y_variance = sum((y - y_mean)**2 for y in y_values) / len(y_values)
        total_variance = (x_variance + y_variance) / 2
        
        # Détecter et ignorer les valeurs aberrantes
        if len(position_history) > 3:
            # Calculer la distance de chaque point par rapport à la moyenne
            distances = [((x - x_mean)**2 + (y - y_mean)**2)**0.5 for x, y in position_history]
            mean_distance = sum(distances) / len(distances)
            
            # Si la dernière position est trop éloignée (>2x la distance moyenne), la considérer comme aberrante
            if distances[-1] > 2 * mean_distance and mean_distance > 0.1:
                # Supprimer la dernière position (aberrante)
                position_history.pop()
                
                # Recalculer avec les positions restantes
                x_values = [pos[0] for pos in position_history]
                y_values = [pos[1] for pos in position_history]
                x_mean = sum(x_values) / len(x_values)
                y_mean = sum(y_values) / len(y_values)
        
        return x_mean, y_mean, total_variance
    
    # Pas assez de données pour filtrer
    return new_x, new_y, 0.0


def uwb_range_offset(uwb_range, quality=100):
    """
    Applique une correction à la mesure de distance en fonction de la qualité du signal
    """
    # Si la qualité est bonne, appliquer moins de correction
    if quality > 80:
        correction = range_offset * 0.8
    else:
        correction = range_offset
        
    return max(0.1, uwb_range - correction)


def calculate_signal_quality(a1_quality, a2_quality):
    """
    Calcule la qualité globale du signal à partir des qualités des anchors
    """
    if a1_quality is None or a2_quality is None:
        return 0
    
    # Moyenne pondérée des qualités
    return (a1_quality + a2_quality) / 2


def main():
    t_ui = turtle.Turtle()
    t_a1 = turtle.Turtle()
    t_a2 = turtle.Turtle()
    t_a3 = turtle.Turtle()
    t_stats = turtle.Turtle()
    
    turtle_init(t_ui)
    turtle_init(t_a1)
    turtle_init(t_a2)
    turtle_init(t_a3)
    turtle_init(t_stats)
    
    a1_range = 0.0
    a2_range = 0.0
    a1_quality = 0
    a2_quality = 0
    
    draw_ui(t_ui)
    
    reconnect_count = 0
    last_data_time = time.time()
    
    while True:
        node_count = 0
        uwb_list = read_data()
        
        # Vérifier si les données sont vides
        if uwb_list:
            last_data_time = time.time()
            reconnect_count = 0
        else:
            # Si pas de données depuis plus de 5 secondes, essayer de se reconnecter
            if time.time() - last_data_time > 5:
                print("Pas de données depuis 5 secondes, tentative de reconnexion...")
                try:
                    data.close()
                    sock.close()
                    
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.bind((UDP_IP, UDP_PORT))
                    sock.listen(1)
                    sock.settimeout(3)  # Timeout de 3 secondes pour accept()
                    
                    try:
                        data, addr = sock.accept()
                        print("Reconnecté avec succès!")
                        last_data_time = time.time()
                    except socket.timeout:
                        print("Échec de la reconnexion, nouvelle tentative...")
                        reconnect_count += 1
                        
                        # Si trop d'échecs, afficher un message d'erreur
                        if reconnect_count > 5:
                            clean(t_a3)
                            write_txt(-250, 0, "ERREUR: Connexion perdue avec le tag UWB",
                                     "red", t_a3, f=('Arial', 20, 'normal'))
                    
                except Exception as e:
                    print(f"Erreur lors de la reconnexion: {e}")
                
                last_data_time = time.time()  # Réinitialiser pour éviter des tentatives trop fréquentes
        
        for one in uwb_list:
            # Traiter les données de l'anchor 1
            if one["A"] == "1782":
                clean(t_a1)
                # Extraire la qualité si disponible
                a1_quality = int(one.get("Q", "0"))
                a1_range = uwb_range_offset(float(one["R"]), a1_quality)
                draw_uwb_anchor(-250, 150, "A1782(0,0)", a1_range, a1_quality, t_a1)
                node_count += 1
            
            # Traiter les données de l'anchor 2
            if one["A"] == "1783":
                clean(t_a2)
                # Extraire la qualité si disponible
                a2_quality = int(one.get("Q", "0"))
                a2_range = uwb_range_offset(float(one["R"]), a2_quality)
                draw_uwb_anchor(-250 + meter2pixel * distance_a1_a2,
                               150, f"A1783({distance_a1_a2})", a2_range, a2_quality, t_a2)
                node_count += 1
        
        # Si les deux anchors sont détectés, calculer la position
        if node_count == 2:
            # Calculer la position brute
            x_raw, y_raw = tag_pos(a2_range, a1_range, distance_a1_a2)
            
            # Filtrer la position pour réduire le bruit
            x_filtered, y_filtered, position_variance = filter_position(x_raw, y_raw)
            
            # Calculer la qualité globale du signal
            signal_quality = calculate_signal_quality(a1_quality, a2_quality)
            
            # Afficher le tag et les statistiques
            clean(t_a3)
            draw_uwb_tag(x_filtered, y_filtered, "TAG", position_variance, t_a3)
            
            # Mettre à jour les informations de qualité
            clean(t_stats)
            write_txt(-400, -300, f"Qualité signal: {signal_quality}%", 
                     "black", t_stats, f=('Arial', 14, 'normal'))
            
            print(f"Position filtrée: ({x_filtered:.2f}, {y_filtered:.2f}), Variance: {position_variance:.3f}")
        
        turtle.update()  # Mettre à jour l'affichage
        time.sleep(0.1)  # Pause courte pour éviter de surcharger le CPU

    turtle.mainloop()


if __name__ == '__main__':
    main()