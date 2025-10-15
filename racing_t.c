#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <termios.h>

typedef struct ship_s
{
    int pos_x;
    int pos_y;
}ship_t; //para no tener que escribir struct ship_s cada vez

void drawShip(void);
void setPositionShip(int x, int y);
ship_t getPositionShip(void);
void moveShip(int key);

ship_t ship = {0, 0};

/* Function prototypes */
void drawShip(void);
void setPositionShip(int x, int y);
ship_t getPositionShip(void);
void moveShip(int key);

int main(void) {
    
    /* Terminal configuration */
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
    cfsetispeed(&term, B115200);
    tcsetattr(STDIN_FILENO, TCSANOW, &term); // Apply changes NOW

    /* SCREEN,  WINDOW,  and  other data structures initialization */
    initscr(); // Initialize stdscr & curscr (WINDOW* type)
    keypad(stdscr, TRUE); // Enable arrow keys and function keys (F1, F2, etc.)
    /*cbreak() es CLAVE para que function keys, con keypad NO es sufiente */
    cbreak(); // take input chars one at a time, no wait for \n 
    noecho();
    curs_set(0); // cursor visibility
    scrollok(stdscr, FALSE); // Disable scrolling
    leaveok(stdscr, TRUE); // Enable cursor movement at the end of the screen

    /* Get terminal size */
    int rows, cols;
    getmaxyx(stdscr, cols, rows);
    printw("Terminal size: %dx%d\n", rows, cols);
    
    /* Initialize game */
    printw("Welcome to Racing Team !\n");
    printw("Press any key to start !\n");
    setPositionShip (rows/2, cols/2);

    /* Game loop */
    while (1) {
        int key = getch();
        if (key == 'q' || key == 'Q') break;
        moveShip(key);
    }
    
    /* Exit from ncurses */
    endwin();
    /* Restore terminal configuration */
    term.c_lflag |= (ECHO | ICANON); // Enable echo and canonical mode to restore default behavior
    tcsetattr(STDIN_FILENO, TCSANOW, &term);// Apply changes NOW

    return 0;
}

void drawShip(void) {
    clear();

    mvprintw(0, 0,"Ship position: %d, %d\n", ship.pos_y, ship.pos_x);
    mvprintw(ship.pos_y, ship.pos_x,"^");
    mvprintw(ship.pos_y+1, ship.pos_x-1, "/#\\");
    mvprintw(ship.pos_y+2, ship.pos_x-2, "/| |\\");
    mvprintw(ship.pos_y+3, ship.pos_x-3, "/_*_*_\\");

    refresh();
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
        setPositionShip(-1, 0);
        break;
    case KEY_RIGHT:
        printw("KEY_RIGHT\n");
        setPositionShip(1, 0);
        break;
    case KEY_DOWN:
        setPositionShip(0, -1);
        break;
    case KEY_UP:
        setPositionShip(0, 1);
        break;
    default:
        break;
    }
    drawShip();
}