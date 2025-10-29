/* road_game.c
   Juego terminal básico: coche en carretera con curvas.
   Controles:
     Flechas izquierda/derecha -> mover coche
     Flechas arriba/abajo -> velocidad
     Ratón (botón izquierdo) -> mover coche a la X del clic
   Compilar:
     gcc -o road_game road_game.c -lncurses -O2
*/

/* Solamente actualiza la posición de la nave con cada scroll */

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define MIN(a,b) ((a)<(b)?(a):(b))

/* Configuración del juego */
static int ROAD_HALF_WIDTH = 12;   /* mitad del ancho de la carretera (en cols) */
static int TICK_MICROS_MIN = 35000;/* velocidad máxima (microseg) */
static int TICK_MICROS_MAX = 150000;/* velocidad mínima (microseg) */

/* Estados del juego */
typedef struct {
    int cols, rows;
    int player_x, player_y;
    int speed_level; /* 0..5, 0 lento, 5 rapido */
    int running;
    int score;
} GameState;

/* Camino: para cada fila tenemos el centro X de la carretera */
typedef struct {
    int *centers; /* length = rows */
    int len;
} Road;

static void init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); /* getch no bloqueante */
    curs_set(0);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    /* Enable mouse events on xterm-like terminals */
    printf("\033[?1003h\n"); fflush(stdout); /* enable mouse move events (optional) */
}

static void end_ncurses() {
    /* disable mouse reporting */
    printf("\033[?1003l\n"); fflush(stdout);
    endwin();
}

static Road *road_create(int rows) {
    Road *r = malloc(sizeof(Road));
    r->len = rows;
    r->centers = malloc(sizeof(int) * rows); //IMPORTANTE: Multiplicar x el # de filas
    for (int i = 0; i < rows; ++i) r->centers[i] = 0;
    return r;
}

static void road_free(Road *r) {
    if (!r) return;
    free(r->centers);
    free(r);
}

/* shift road down by one and generate new center at top using simple random walk */
static void road_scroll_and_generate(Road *r, int cols) {
    /* move down */
    for (int i = r->len - 1; i > 0; --i){
        r->centers[i] = r->centers[i-1];
    }

    /* generate new center for the new row */
    int prev = r->centers[1]; /* previous top-most meaningful */
    if (prev == 0) 
        prev = cols / 2;
    int change = (rand() % 7) - 3; /* -3..3 for stronger curves */
    int newc = prev + change;
    int margin = ROAD_HALF_WIDTH + 2;
    if (newc < margin)
        newc = margin;
    if (newc > cols - margin - 1) 
        newc = cols - margin - 1;
    r->centers[0] = newc;
}

/* initialize road centers with a gentle oscillation centered */
static void road_init(Road *r, int cols) {
    int center = cols / 2;
    for (int i = 0; i < r->len; ++i) {
        r->centers[i] = center + (int)(5.0 * sin((double)i / 6.0));
    }
}

/* draw the road and car. Car is drawn near the bottom. */
static void render(GameState *g, Road *road) {
    erase();
    int rows = g->rows;
    int cols = g->cols;

    /* Draw each row: road edges and fill */
    for (int row = 0; row < rows; ++row) {
        int center = road->centers[row];
        int left = center - ROAD_HALF_WIDTH;
        int right = center + ROAD_HALF_WIDTH;
        if (left < 0) 
            left = 0;
        if (right >= cols)
            right = cols - 1;

        /* Draw fill */
        for (int c = 0; c < cols; ++c) {
            if (c == left || c == right) {
                mvaddch(row, c, ACS_VLINE); /* border */
            } else if (c > left && c < right) {
                mvaddch(row, c, ' '); /* road interior blank */
            } else {
                mvaddch(row, c, '.'); /* off-road texture */
            }
        }

        /* center dashed line */
        if (row % 4 == 0) {
            if (center >= 0 && center < cols) mvaddch(row, center, ':');
        }
    }

    /* Draw player car - a 3x3 simple sprite */
    int px = g->player_x;
    int py = g->player_y;
    const char *car[] = { " ^ ", "/#\\", "/ \\" };
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int X = px - 1 + c;
            int Y = py - (2 - r);
            if (X >= 0 && X < cols && Y >= 0 && Y < rows) {
                mvaddch(Y, X, car[r][c]);
            }
        }
    }

    /* HUD */
    mvprintw(0, 1, "Score:%d  Speed:%d  Quit:q", g->score, g->speed_level);
    refresh();
}

/* returns 1 if collision (car leaves road boundaries) */
static int check_collision(GameState *g, Road *r) {
    /* check the row where car is located. We test the car's center column */
    int check_row = g->player_y;
    if (check_row < 0) 
        return 1;
    if (check_row >= r->len)
         check_row = r->len - 1;
    int center = r->centers[check_row];
    int left = center - ROAD_HALF_WIDTH;
    int right = center + ROAD_HALF_WIDTH;
    if (g->player_x < left + 1 || g->player_x > right - 1)
        return 1;
    return 0;
}

/* clamp helper */
static int clamp(int v, int a, int b) { if (v<a) return a; if (v>b) return b; return v; }

/* handle input: arrows and mouse */
static void handle_input(GameState *g) {
    int ch;
    MEVENT mev; //mouse Event
    while ((ch = getch()) != ERR) {
        if (ch == 'q' || ch == 'Q') {
            g->running = 0;
            return;
        } else if (ch == KEY_LEFT) {
            g->player_x -= 2;
        } else if (ch == KEY_RIGHT) {
            g->player_x += 2;
        } else if (ch == KEY_UP) {
            if (g->speed_level < 5) g->speed_level++;
        } else if (ch == KEY_DOWN) {
            if (g->speed_level > 0) g->speed_level--;
        } else if (ch == KEY_MOUSE) {
            if (getmouse(&mev) == OK) {
                /* Button pressed or movement */
                if (mev.bstate & BUTTON1_PRESSED) {
                    /* teleport/move car to clicked X */
                    g->player_x = mev.x;
                } else if (mev.bstate & BUTTON1_CLICKED) {
                    g->player_x = mev.x;
                } else if (mev.bstate & BUTTON1_RELEASED) {
                    /* ignore */
                } else if (mev.bstate & REPORT_MOUSE_POSITION) {
                    /* optional: if left button held, follow */
                    if (mev.bstate & BUTTON1_PRESSED) g->player_x = mev.x;
                }
            }
        }
    }
    /* clamp to screen */
    g->player_x = clamp(g->player_x, 1, g->cols - 2);
}

/* compute tick delay from speed_level */
static int compute_tick_us(int speed_level) {
    /* speed_level 0..5 map to delays */
    int t = TICK_MICROS_MAX - ( (TICK_MICROS_MAX - TICK_MICROS_MIN) * speed_level / 5 );
    return t;
}

int main() {
    srand(time(NULL));
    init_ncurses();

    GameState g;
    getmaxyx(stdscr, g.rows, g.cols);
    g.player_y = g.rows - 3; /* place car near bottom */
    g.player_x = g.cols / 2;
    g.speed_level = 3;
    g.running = 1;
    g.score = 0;

    Road *road = road_create(g.rows);
    road_init(road, g.cols);

    /* initial fill with center at middle */
    for (int i = 0; i < road->len; ++i) {
        road->centers[i] = g.cols / 2;
    }

    /* main loop */
    while (g.running) {
        handle_input(&g);

        /* scroll road according to speed */
        int steps = 1 + g.speed_level / 2; /* small relation to speed */
        for (int s = 0; s < steps; ++s) {
            road_scroll_and_generate(road, g.cols);
            g.score++;
        }

        /* render */
        render(&g, road);

        /* collision check */
        if (check_collision(&g, road)) {
            mvprintw(g.rows/2, (g.cols/2)-6, "¡COLISIÓN! Punt: %d", g.score);
            mvprintw(g.rows/2 + 1, (g.cols/2)-10, "Pulse r para reiniciar o q para salir");
            refresh();
            nodelay(stdscr, FALSE);
            int ch;
            while ((ch = getch())) {
                if (ch == 'q' || ch == 'Q') { g.running = 0; break; }
                if (ch == 'r' || ch == 'R') {
                    /* reset game */
                    g.player_x = g.cols/2;
                    g.speed_level = 3;
                    g.score = 0;
                    for (int i = 0; i < road->len; ++i) road->centers[i] = g.cols/2;
                    nodelay(stdscr, TRUE);
                    break;
                }
            }
            if (!g.running) break;
        }

        /* sleep according to speed */
        int delay = compute_tick_us(g.speed_level);
        usleep(delay);
    }

    road_free(road);
    end_ncurses();
    return 0;
}
