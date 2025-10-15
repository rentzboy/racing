#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define ROAD_WIDTH 20
#define DELAY 60000

int main() {
    int max_y, max_x;
    int car_pos_x, car_pos_y;
    int road_offset;
    int key;
    MEVENT event;

    // Inicialización de ncurses
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(0); // No bloquear en getch()

    // Inicializar colores
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_YELLOW, COLOR_BLACK); // Coche
        init_pair(2, COLOR_WHITE, COLOR_BLACK);  // Carretera
        init_pair(3, COLOR_GREEN, COLOR_BLACK);  // Hierba
    }

    // Obtener dimensiones de la terminal
    getmaxyx(stdscr, max_y, max_x);

    // Posición inicial del coche
    car_pos_x = max_x / 2;
    car_pos_y = max_y - 2;

    // Desplazamiento inicial de la carretera
    road_offset = 0;

    // Semilla para la generación de la carretera
    srand(time(NULL));

    // Habilitar la entrada del ratón
    mousemask(BUTTON1_PRESSED | REPORT_MOUSE_POSITION, NULL);

    // Bucle principal del juego
    while (1) {
        // --- LÓGICA DEL JUEGO ---

        // Leer entrada del usuario
        key = getch();

        if (key == KEY_LEFT) {
            car_pos_x--;
        } else if (key == KEY_RIGHT) {
            car_pos_x++;
        } else if (key == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                // Mover el coche a la posición X del ratón
                car_pos_x = event.x;
            }
        } else if (key == 'q' || key == 'Q') {
            break; // Salir del juego
        }
        
        // Limitar la posición del coche a la pantalla
        if (car_pos_x < 0) car_pos_x = 0;
        if (car_pos_x >= max_x) car_pos_x = max_x -1;

        // --- DIBUJADO EN PANTALLA ---

        clear(); // Limpiar la pantalla

        // Dibujar la carretera y la hierba
        for (int y = 0; y < max_y; y++) {
            // Calcular el centro de la carretera para esta fila
            // Usamos una función seno para crear curvas suaves
            int road_center = (max_x / 2) + (int)(15.0 * sin((double)(y + road_offset) * 0.1));

            // Calcular los límites de la carretera
            int road_left = road_center - ROAD_WIDTH / 2;
            int road_right = road_center + ROAD_WIDTH / 2;
            
            // Dibujar la hierba y la carretera
            for (int x = 0; x < max_x; x++) {
                if (x >= road_left && x < road_right) {
                    attron(COLOR_PAIR(2));
                    mvprintw(y, x, "|"); // Borde de la carretera
                    attroff(COLOR_PAIR(2));
                } else {
                    attron(COLOR_PAIR(3));
                    mvprintw(y, x, "."); // Hierba
                    attroff(COLOR_PAIR(3));
                }
            }
            
             // Dibujar el centro de la carretera
            attron(COLOR_PAIR(2));
            if ((y + road_offset) % 4 == 0) {
                 mvprintw(y, road_center, "'");
            }
            attroff(COLOR_PAIR(2));
        }

        // Actualizar el scroll de la carretera
        road_offset++;

        // --- DETECCIÓN DE COLISIÓN ---
        int current_road_center = (max_x / 2) + (int)(15.0 * sin((double)(car_pos_y + road_offset -1) * 0.1));
        int current_road_left = current_road_center - ROAD_WIDTH / 2;
        int current_road_right = current_road_center + ROAD_WIDTH / 2;

        if (car_pos_x <= current_road_left || car_pos_x >= current_road_right -1) {
            mvprintw(max_y / 2, max_x / 2 - 5, "GAME OVER");
            refresh();
            sleep(2);
            break;
        }

        // Dibujar el coche
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(car_pos_y, car_pos_x, "A");
        attroff(COLOR_PAIR(1) | A_BOLD);

        // Mostrar información
        mvprintw(0, 1, "Usa las flechas o el ratón para moverte. Pulsa 'q' para salir.");

        refresh(); // Actualizar la pantalla

        usleep(DELAY); // Pequeña pausa para controlar la velocidad del juego
    }

    // Finalizar ncurses
    endwin();

    return 0;
}