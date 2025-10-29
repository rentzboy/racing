#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <termios.h>
#include <time.h>

typedef struct ship_s
{
    int pos_x;
    int pos_y;
}ship_t; //para no tener que escribir struct ship_s cada vez

ship_t ship = {0, 0};
clock_t start, end;
int rows, cols;

#define ROAD_WIDTH       12
#define BACKGROUND_WIDTH rows/2

#define LEVEL_EASY      300000
#define LEVEL_MEDIUM    200000
#define LEVEL_HARD      150000
#define LEVEL_PRO       100000

/* Function prototypes */
void drawShip(void);
void clearShip(void);
void drawBackground(int first, int last);
void setPositionShip(int x, int y);
ship_t getPositionShip(void);
void moveShip(int key);
void scrolling(void);

int main(void) {
    
    /* Terminal configuration */
    struct termios term;
    tcgetattr(STDIN_FILENO, &term); // Get terminal settings for STDIN
    term.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
    cfsetispeed(&term, B115200);
    tcsetattr(STDIN_FILENO, TCSANOW, &term); // Apply changes NOW

    /* SCREEN,  WINDOW,  and  other data structures initialization */
    initscr(); // Initialize stdscr & curscr (WINDOW* type)
    nodelay(stdscr, FALSE); // getch TO BE BLOCKING
    keypad(stdscr, TRUE); // Enable arrow keys and function keys (F1, F2, etc.)
    /*cbreak() es CLAVE para que function keys, con keypad NO es sufiente */
    cbreak(); // take input chars one at a time, no wait for \n 
    noecho();
    curs_set(0); // cursor visibility (0 invisible, 1 visible)
    scrollok(stdscr, TRUE); // Enable scrolling, required for scrl(n);
    leaveok(stdscr, TRUE); // Enable cursor movement at the end of the screen
    idlok(stdscr, TRUE); // Enable insert/delete line mode

    /* Initialize game */
    mvwprintw(stdscr, 0, 0,"Welcome to Racing Team !\n");
    mvwprintw(stdscr, 1, 0,"Press any key to start !\n");
    getmaxyx(stdscr, rows, cols);
    mvwprintw(stdscr, 2, 0,"Columns: %d, Rows: %d\n", cols, rows);
    setPositionShip (cols/2, rows/2 + (rows/4));
    drawShip();
    drawBackground(3, rows);

    /* Waiting for the user to press a key */
    while (!getch()) { }
    nodelay(stdscr, TRUE); // getch() TO BE NON-BLOCKING
    drawBackground(1, rows);
 
    /* Game loop */ 
    while (1) {
        start = clock();
        int key = getch();
        if (key == 'q' || key == 'Q') break;                                                           
        moveShip(key);                                                           
        end = clock();
        //Ajust delay to be always const) devuelve la CPU durante x ms
        usleep(LEVEL_EASY - (end - start)); 
        scrolling();
    }
    
    /* Exit from ncurses */
    endwin();
    /* Restore terminal configuration */
    term.c_lflag |= (ECHO | ICANON); // Enable echo and canonical mode to restore default behavior
    tcsetattr(STDIN_FILENO, TCSANOW, &term);// Apply changes NOW

    return 0;
}

void scrolling(void) {

    clearShip();    
    wscrl(stdscr, -1);
    drawBackground(1, 1);  //pintar la primera row despues del scrolling
    drawShip();
}

void clearShip(void) {

    mvwprintw(stdscr, ship.pos_y, ship.pos_x," ");
    mvwprintw(stdscr, ship.pos_y+1, ship.pos_x-1, "   ");
    mvwprintw(stdscr, ship.pos_y+2, ship.pos_x-2, "     ");
    mvwprintw(stdscr, ship.pos_y+3, ship.pos_x-3, "       ");
}
void drawShip(void) {

    mvwprintw(stdscr, 0, 0,"Ship position: %d, %d\n", ship.pos_y, ship.pos_x);
    mvwprintw(stdscr, ship.pos_y, ship.pos_x,"^");
    mvwprintw(stdscr, ship.pos_y+1, ship.pos_x-1, "/#\\");
    mvwprintw(stdscr, ship.pos_y+2, ship.pos_x-2, "/| |\\");
    mvwprintw(stdscr, ship.pos_y+3, ship.pos_x-3, "/_*_*_\\");
    
    refresh();
}

void drawBackground(int first, int last) {

    /* HAY QUE GUARDAR EL CENTER EN UNA GLOBAL, PARA */

    static int center = 80; //IMPORTANTE: STATIC, 80 initial value must be constant.
    int limit_right = 80 + BACKGROUND_WIDTH;
    int limit_left = 80 - BACKGROUND_WIDTH;

    for (int i = first; i <= last; i++) //pintar las filas
    {
        int x;
        int random = rand() % 3; // 0, 1, 2

        if (center + random > limit_right)
        {
            x = center - random;
        }
        else if (center - random < limit_left)
        {
            x = center + random;
        }
        else if (random % 2) //curva a la derecha
        {
            x = center + random;
        }
        else //curva a la izquierda
        {
            x = center - random;
        }
        
        mvwprintw(stdscr, i, x - ROAD_WIDTH/2, "|");
        mvwhline(stdscr, i, 0, '*', x - ROAD_WIDTH/2);
        mvwprintw(stdscr, i, x + ROAD_WIDTH/2, "|");
        mvwhline(stdscr, i, x + ROAD_WIDTH/2 + 1 , '*', 100); //solo pinta hasta el boundary
        
        center = x;
    }
}

void setPositionShip(int x, int y) {
    ship.pos_x += x;
    ship.pos_y += y;
}

ship_t getPositionShip(void) {
    return ship;
}

void moveShip(int key) {
    switch (key)
    {
    case KEY_LEFT:
        clearShip();
        setPositionShip(-1, 0);
        drawShip();
        break;
    case KEY_RIGHT:
        clearShip();
        setPositionShip(1, 0);
        drawShip();
        break;
    case KEY_DOWN:
        clearShip();
        setPositionShip(0, 1);
        drawShip();
        break;
    case KEY_UP:
        clearShip();
        setPositionShip(0, -1);
        drawShip();
        break;
    case 32: // también podriamos haber puesto ' ' (= SPACE)
        /* Para salir del sleep antes hay que enviar una señal tipo crtl+c */
        sleep(5);
    default:
        break;
    }
    
    flushinp(); // Clear the input buffer to avoid retards
}