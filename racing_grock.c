#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ROAD_WIDTH 20    // Ancho de la pantalla/road
#define ROAD_HEIGHT 24   // Altura de la pantalla
#define ROAD_LANE_WIDTH 9  // Ancho de la carretera
#define CAR_CHAR '@'     // Símbolo del coche
#define ROAD_EDGE '| '   // Bordes de la carretera
#define ROAD_FILL '.'    // Relleno de la carretera

int main() {
    // Inicializar ncurses
    initscr();
    noecho();              // No mostrar teclas presionadas
    keypad(stdscr, TRUE);  // Habilitar teclas especiales (flechas)
    curs_set(0);           // Ocultar cursor
    nodelay(stdscr, TRUE); // No bloquear getch()
    mousemask(ALL_MOUSE_EVENTS, NULL); // Habilitar ratón

    srand(time(NULL));     // Semilla para random

    int car_x = ROAD_LANE_WIDTH / 2;  // Posición inicial del coche (relativa a la carretera)
    int car_y = ROAD_HEIGHT - 2;      // Posición Y fija del coche (cerca del fondo)
    int road_offset[ROAD_HEIGHT];     // Offset horizontal de la carretera por fila
    for (int i = 0; i < ROAD_HEIGHT; i++) {
        road_offset[i] = (ROAD_WIDTH - ROAD_LANE_WIDTH) / 2;  // Centrada inicialmente
    }
    int curve_direction = 0;  // Dirección de curva (-1 izquierda, 0 recto, 1 derecha)
    int score = 0;            // Puntaje (tiempo survived)

    while (1) {
        clear();  // Limpiar pantalla

        // Desplazar la carretera hacia arriba (scroll)
        for (int i = 0; i < ROAD_HEIGHT - 1; i++) {
            road_offset[i] = road_offset[i + 1];
        }

        // Generar nuevo segmento de carretera con posible curva
        if (rand() % 5 == 0) {  // Probabilidad de cambiar dirección
            curve_direction += (rand() % 3 - 1);  // -1, 0 o +1
            if (curve_direction < -1) curve_direction = -1;
            if (curve_direction > 1) curve_direction = 1;
        }
        road_offset[ROAD_HEIGHT - 1] = road_offset[ROAD_HEIGHT - 2] + curve_direction;
        // Limitar offset para no salirse de la pantalla
        if (road_offset[ROAD_HEIGHT - 1] < 0) road_offset[ROAD_HEIGHT - 1] = 0;
        if (road_offset[ROAD_HEIGHT - 1] > ROAD_WIDTH - ROAD_LANE_WIDTH) {
            road_offset[ROAD_HEIGHT - 1] = ROAD_WIDTH - ROAD_LANE_WIDTH;
        }

        // Dibujar la carretera
        for (int y = 0; y < ROAD_HEIGHT; y++) {
            mvaddch(y, road_offset[y], ROAD_EDGE);                     // Borde izquierdo
            mvaddch(y, road_offset[y] + ROAD_LANE_WIDTH - 1, ROAD_EDGE);  // Borde derecho
            for (int x = 1; x < ROAD_LANE_WIDTH - 1; x++) {
                mvaddch(y, road_offset[y] + x, ROAD_FILL);  // Relleno
            }
        }

        // Dibujar el coche
        mvaddch(car_y, road_offset[car_y] + car_x, CAR_CHAR);

        // Mostrar puntaje
        mvprintw(0, 0, "Score: %d", score);
        mvprintw(1, 0, "Usa flechas o ratón. 'q' para salir.");

        refresh();  // Actualizar pantalla

        // Manejar input
        int ch = getch();
        MEVENT event;
        if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_PRESSED) {
                    // Mover coche a posición relativa del clic (si está dentro de la carretera)
                    int clicked_x = event.x - road_offset[car_y];
                    if (clicked_x >= 1 && clicked_x <= ROAD_LANE_WIDTH - 2) {
                        car_x = clicked_x;
                    }
                }
            }
        } else if (ch == KEY_LEFT) {
            car_x--;
        } else if (ch == KEY_RIGHT) {
            car_x++;
        } else if (ch == 'q' || ch == 'Q') {
            break;
        }

        // Limitar posición del coche dentro de la carretera
        if (car_x < 1) car_x = 1;
        if (car_x > ROAD_LANE_WIDTH - 2) car_x = ROAD_LANE_WIDTH - 2;

        // Verificar colisión (si coche toca borde)
        if (car_x <= 0 || car_x >= ROAD_LANE_WIDTH - 1) {
            mvprintw(ROAD_HEIGHT / 2, (ROAD_WIDTH - 10) / 2, "¡Game Over!");
            refresh();
            sleep(2);
            break;
        }

        usleep(100000);  // Delay de 0.1 segundos para velocidad del juego
        score++;         // Incrementar puntaje
    }

    endwin();  // Finalizar ncurses
    return 0;
}