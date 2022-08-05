
#ifndef _UI_H
#define _UI_H

/* Color pairs */

enum { CLR_WINDOW=1, CLR_HINT, CLR_UNSEL, CLR_SEL, CLR_TEXT, CLR_EMPHASIS, CLR_STATUS };

static void set_color(int pair) {
    attron(COLOR_PAIR(pair));
    switch(pair) {
        case CLR_HINT: attroff(A_BOLD); break;
        case CLR_UNSEL: attroff(A_BOLD); break;
        case CLR_SEL: attroff(A_BOLD); break;
        case CLR_TEXT: attron(A_BOLD); break;
        case CLR_WINDOW: attron(A_BOLD); break;
        case CLR_EMPHASIS: attron(A_BOLD); break;
        case CLR_STATUS: attroff(A_BOLD); break;
        default: break;
    }
}

static void draw_box(int x, int y, int width, int height) {
    set_color(CLR_WINDOW);
    move(y - 1, x - 1);
    addch(ACS_ULCORNER);
    for(int i = 0; i < width; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);
    for(int i = 0; i < height; i++) {
        move(y + i, x - 1);
        addch(ACS_VLINE);
        move(y + i, x + width);
        addch(ACS_VLINE);
    }
    move(y + height, x - 1);
    addch(ACS_LLCORNER);
    for(int i = 0; i < width; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);
}

static void curses_init() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    start_color();

    if(COLS < 80 || LINES < 24 || has_colors() == FALSE) {
        endwin();
        printf("Terminal window too small or monochrome.\n");
        exit(1);
    }

    init_pair(CLR_WINDOW, COLOR_RED, COLOR_BLACK);
    init_pair(CLR_HINT, COLOR_WHITE, COLOR_BLACK);
    init_pair(CLR_TEXT, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CLR_UNSEL, COLOR_BLUE, COLOR_BLACK);
    init_pair(CLR_SEL, COLOR_GREEN, COLOR_BLACK);
    init_pair(CLR_EMPHASIS, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CLR_STATUS, COLOR_BLACK, COLOR_CYAN);
}

static void curses_end() {
    curs_set(1);
    nocbreak();
    echo();
    keypad(stdscr, FALSE);
    endwin();
}

static void sigint_handler() {
    curses_end();
    exit(0);
}

#endif
