
#include <ncurses.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include "vector.h"

#include "ui.h"
#include "dictionary.h"

#include "util.h"

struct game_settings {
    int seed, tempo, freq, dict_idx;
    vector(char *) dictionaries;
};

static void boxed_logo(int * y, int * x) {
    /* Find the coordinates where we want to place the window. */
    /* (47x9 logo) */
    const int logo_x = 47, logo_y = 9, menu_height = 7;
    const char * logo = 
        "  ______                                       "
        " /_  __/_  ______  ___                         "
        "  / / / / / / __ \\/ _ \\                        "
        " / / / /_/ / /_/ /  __/                        "
        "/_/  \\__, / .___/\\___/___                      "
        "    /____/_/        / __ \\____ _________  _____"
        "                   / /_/ / __ `/ ___/ _ \\/ ___/"
        "                  / _, _/ /_/ / /__/  __/ /    "
        "                 /_/ |_|\\__,_/\\___/\\___/_/     ";
    int middle_y = (LINES - logo_y - menu_height) / 2;
    int middle_x = (COLS - logo_x) / 2;

    nodelay(stdscr, FALSE);
    clear();

    set_color(CLR_EMPHASIS);
    for(int i = 0; i < logo_y; i++) {
        move(middle_y + i, middle_x);
        addnstr(logo + i * logo_x, logo_x);
    }

    set_color(CLR_WINDOW);
    draw_box(middle_x - 1, middle_y - 1, logo_x + 2, logo_y + 2);
    *y = middle_y + logo_y; *x = middle_x;
}

/* Main menu. */

static void main_menu(struct game_settings * settings) {
    int y, x; boxed_logo(&y, &x);

    int sel_id = 0, seldict = 0;
    while(true) {
        /* Repaint the menu. */
        char buf[7];

        #define TEXT_BOX(y_off, optid, text, hint, ...) \
            set_color(CLR_TEXT); \
            move(y + y_off, x); \
            printw(text); \
            set_color(sel_id == optid ? CLR_SEL : CLR_UNSEL); \
            __VA_ARGS__; \
            set_color(CLR_HINT); \
            printw(hint);

        TEXT_BOX(4, 0, "Level seed:  ", "  (leave empty for random)",
            sprintf(buf, "%06d", settings->seed); subst(buf, '0', '.'); addstr(buf))
        
        TEXT_BOX(5, 1, "Tempo:       ", "  (how quickly words move)",
            sprintf(buf, "%06d", settings->tempo); subst(buf, '0', '.'); addstr(buf))
        
        TEXT_BOX(6, 2, "Frequency:   ", "  (how quickly words appear)",
            sprintf(buf, "%06d", settings->freq); subst(buf, '0', '.'); addstr(buf))
        
        TEXT_BOX(7, 3, "Dictionary:  ", "",
            addnstr(settings->dictionaries[seldict], min(strlen(settings->dictionaries[seldict]), 20));
            for(int i = 0; i < 20 - strlen(settings->dictionaries[seldict]); i++) addch(' '))
        
        /* Draw OK/Quit buttons in the same row under the menu */
        set_color(sel_id == 4 ? CLR_SEL : CLR_UNSEL);
        mvaddstr(y + 9, (COLS - 4) / 2, "<OK>");

        int c = getch();
        switch(c) {
            case '\t': case KEY_DOWN: sel_id = (sel_id + 1) % 5; break;
            case KEY_UP: sel_id = (sel_id + 4) % 5; break;
            case '\n': if(sel_id == 4) {
                /* In case the values chosen are wrong, alter them. */
                if(settings->seed == 0) settings->seed = millis() % 1000000;
                if(settings->tempo < 50) settings->tempo = 50;
                else if(settings->tempo > 500) settings->tempo = 500;
                if(settings->freq < 50) settings->freq = 50;
                else if(settings->freq > 500) settings->freq = 500;
                settings->dict_idx = seldict;
                return;
            } break;
            case KEY_LEFT:
                if(sel_id == 0) settings->seed += 999999, settings->seed %= 1000000;
                else if(sel_id == 1) settings->tempo += 9999, settings->tempo %= 10000;
                else if(sel_id == 2) settings->freq += 9999, settings->freq %= 10000;
                else if(sel_id == 3) seldict = (seldict + vector_size(settings->dictionaries) - 1) % vector_size(settings->dictionaries);
                break;
            case KEY_RIGHT:
                if(sel_id == 0) settings->seed++, settings->seed %= 1000000;
                else if(sel_id == 1) settings->tempo++, settings->tempo %= 10000;
                else if(sel_id == 2) settings->freq++, settings->freq %= 10000;
                else if(sel_id == 3) seldict = (seldict + 1) % vector_size(settings->dictionaries);
                break;
            default:
                if(isdigit(c)) {
                    if(sel_id == 0 && settings->seed < 100000) {
                        settings->seed = settings->seed * 10 + c - '0';
                    } else if(sel_id == 1 && settings->tempo < 1000) {
                        settings->tempo = settings->tempo * 10 + c - '0';
                    } else if(sel_id == 2 && settings->freq < 1000) {
                        settings->freq = settings->freq * 10 + c - '0';
                    }
                } else if(c == KEY_BACKSPACE) {
                    if(sel_id == 0) {
                        settings->seed /= 10;
                    } else if(sel_id == 1) {
                        settings->tempo /= 10;
                    } else if(sel_id == 2) {
                        settings->freq /= 10;
                    }
                }
        }

        #undef TEXT_BOX
    }
}

/* Game Over */

#define CENTER_STATIC(y, str) \
    mvaddstr((y), (COLS - strlen(str)) / 2, (str))

/* Return values: 0 - main menu, 1 - replay with same settings, 2 - exit */
static int game_over(uint32_t score, uint16_t wordcount, uint32_t elapsed, int initial_seed, int freq, int tempo, int erased) {
    int cps = score / (elapsed == 0 ? 1 : elapsed);
    int wpm = ceil((float)(wordcount * 60) / (elapsed == 0 ? 1 : elapsed));
    float acc = score == 0 ? 100.0 : 100.0 - (erased * 100.0 / score);

    int y, x; boxed_logo(&y, &x); refresh();
    set_color(CLR_WINDOW);
    CENTER_STATIC(y + 4, " /// GAME OVER /// ");
    set_color(CLR_TEXT); mvaddstr(y + 6, x + 1,  "Score:  "); set_color(CLR_EMPHASIS); printw("%d", score);
    set_color(CLR_TEXT); mvaddstr(y + 7, x + 1,  "Words:  "); set_color(CLR_EMPHASIS); printw("%d", wordcount);
    set_color(CLR_TEXT); mvaddstr(y + 8, x + 1,  "Time:   "); set_color(CLR_EMPHASIS); printw("%d:%02d", elapsed / 60, elapsed % 60);
    set_color(CLR_TEXT); mvaddstr(y + 6, x + 17, "CPS:  ");   set_color(CLR_EMPHASIS); printw("%d", cps);
    set_color(CLR_TEXT); mvaddstr(y + 7, x + 17, "WPM:  ");   set_color(CLR_EMPHASIS); printw("%d", wpm);
    set_color(CLR_TEXT); mvaddstr(y + 8, x + 17, "Seed: ");   set_color(CLR_EMPHASIS); printw("%d", initial_seed);
    set_color(CLR_TEXT); mvaddstr(y + 6, x + 32, "Freq:  ");  set_color(CLR_EMPHASIS); printw("%d", freq);
    set_color(CLR_TEXT); mvaddstr(y + 7, x + 32, "Tempo: ");  set_color(CLR_EMPHASIS); printw("%d", tempo);
    set_color(CLR_TEXT); mvaddstr(y + 8, x + 32, "Acc:   ");  set_color(CLR_EMPHASIS); printw("%.2f%%", acc);

    /* Display the buttons (main menu, replay). */
    int sel_id = 0;
    while(true) {
        char * buttons[] = { "  New game  ", "  Replay  ", "  Exit  " };
        char * buttons_sel[] = { "/ New game /", "/ Replay /", "/ Exit /" };
        for(int i = 0; i < 3; i++) {
            set_color(i == sel_id ? CLR_SEL : CLR_UNSEL);
            CENTER_STATIC(y + 10 + i, i == sel_id ? buttons_sel[i] : buttons[i]);
        }
        move(y + 10 + sel_id, x + 1);
        int c = getch();
        switch(c) {
            case '\t': case KEY_DOWN: sel_id = (sel_id + 1) % 3; break;
            case KEY_UP: sel_id = (sel_id + 2) % 3; break;
            case '\n': return sel_id;
        }
        refresh();
    }
}

/* Game code */

struct word_t {
    char * word;
    int pos, row;
};

static uint32_t gen_random(struct game_settings * s) {
    return s->seed = s->seed * 1103515245 + 12345;
}

static char * random_word(struct game_settings * s, vector(char *) dict) {
    return dict[gen_random(s) % vector_size(dict)];
}

static int game(struct game_settings * s, vector(char *) dict) {
    int8_t lives = 5;
    uint32_t score = 0;
    uint16_t wordcount = 0;
    uint64_t ticks = 0, bcksp = 0;
    uint64_t begin = time(NULL);
    int initial_seed = s->seed;
    char cw[20] = { 0 };
    uint8_t * lanes = malloc(LINES - 2);
    vector(struct word_t) words = NULL;
    nodelay(stdscr, TRUE);
    curs_set(1);
    while(lives > 0) {
        erase();
        /* 1. Repaint the status bar. */
        set_color(CLR_STATUS);
        move(LINES - 1, 0);
        clrtoeol();
        hline(' ', COLS);
        int elapsed = time(NULL) - begin;
        int cps = score / (elapsed == 0 ? 1 : elapsed);
        int wpm = ceil((float)(wordcount * 60) / (elapsed == 0 ? 1 : elapsed));
        printw("Score: %010d | Words: %05d | .................... | Lives: %d | %ds %dWPM %dCPS", score, wordcount, lives, elapsed, wpm, cps);
        /* 2. Advance word or generate new? Game tick? */
        if(ticks % s->freq == 0) {
            char * word = random_word(s, dict);

            memset(lanes, 0, LINES - 2);
            
            for(int i = 0; i < vector_size(words); i++)
                if(words[i].pos < strlen(word) + 6) /* Lane definitely busy. Do not put anything here. */
                    lanes[words[i].row] = 2;
                else                                /* Lane busy, but it's OK. */
                    lanes[words[i].row] = 1;
            
            /* 33% probability for a busy lane, 66% probability for an OK lane: */
            int l_busy = 0, l_free = 0;
            for(int i = 0; i < LINES - 2; i++)
                if(lanes[i] == 1) l_busy++;
                else if(lanes[i] == 0) l_free++;
            
            if(l_busy != 0 || l_free != 0) {
                int id;
                if(l_busy == 0 || (l_free != 0 && gen_random(s) % 3 != 0)) {
                    /* Just pick one of the free lanes. */
                    id = (gen_random(s) % l_free) + 1;
                    for(int i = 0; i < LINES - 2; i++) {
                        if(lanes[i] == 0) id--;
                        if(id == 0) { id = i; break; }
                    }
                } else {
                    /* Pick a busy lane. */
                    id = (gen_random(s) % l_busy) + 1;
                    for(int i = 0; i < LINES - 2; i++) {
                        if(lanes[i] == 1) id--;
                        if(id == 0) { id = i; break; }
                    }
                }

                struct word_t wv = (struct word_t) {
                    .word = word,
                    .pos = 0,
                    .row = id
                };

                vector_push_back(words, wv);
            }
        } else if(ticks % s->tempo == 0) {
            for(int i = 0; i < vector_size(words); i++) {
                words[i].pos++;
                if(words[i].pos + strlen(words[i].word) >= COLS) {
                    vector_erase(words, i);
                    i--; lives--;
                    beep();
                }
            }
        }
        /* 3. Display the current state of affairs. */
        for(int i = 0; i < vector_size(words); i++) {
            set_color(CLR_TEXT); int ptr = 0;
            move(words[i].row, words[i].pos);
            while(words[i].word[ptr] == cw[ptr])
                addch(words[i].word[ptr++]);
            if(words[i].pos + strlen(words[i].word) > COLS * 2 / 3)
                set_color(CLR_EMPHASIS);
            else
                set_color(CLR_HINT);
            while(ptr < strlen(words[i].word))
                addch(words[i].word[ptr++]);
        }
        /* 4. Render the text. */
        move(LINES - 1, 35);
        set_color(CLR_STATUS);
        addstr(cw);
        /* 5. Read keypresses */
        int c = getch();
        if(c == KEY_F(1))
            break;
        if(isalnum(c) || c == '\'' || c == '-' || c == KEY_BACKSPACE || c == '\n') {
            if(c == KEY_BACKSPACE) {
                if(strlen(cw) > 0)
                    cw[strlen(cw) - 1] = 0, bcksp++;
            } else if(c == '\n') {
                memset(cw, 0, sizeof(cw));
                bcksp++;
            } else {
                if(strlen(cw) < sizeof(cw) - 1)
                    cw[strlen(cw)] = toupper(c);
            }

            for(int i = 0; i < vector_size(words); i++) {
                if(strcmp(cw, words[i].word) == 0) {
                    score += strlen(cw);
                    vector_erase(words, i);
                    i--; wordcount++;
                    memset(cw, 0, sizeof(cw));
                    beep();
                    break;
                }
            }
        }

        ticks++;
        usleep(5000);
    }
    curs_set(0);
    nodelay(stdscr, FALSE);
    free(lanes);
    vector_free(words);
    return game_over(score, wordcount, time(NULL) - begin, initial_seed, s->freq, s->tempo, bcksp);
}

/* Driver code. */

int main(void) {
    vector(char *) dictionaries = find_dictionaries();

    curses_init(); atexit(curses_end);
    
    sigaction(SIGINT, &(struct sigaction) {
        .sa_handler = sigint_handler,
        .sa_flags = SA_RESETHAND
    }, NULL);

    int status;
    do {
        struct game_settings s = { 0, 80, 280, 0, dictionaries };
        main_menu(&s);
        vector(char *) dictionary = NULL;
        FILE * input = fopen(s.dictionaries[s.dict_idx], "r");
        if(input == NULL) {
            curses_end();
            printf("Could not open the dictionary file: %s\n", s.dictionaries[s.dict_idx]);
            return 1;
        }
        char line[32];
        while(fgets(line, 32, input) != NULL) {
            if(line[strlen(line) - 1] == '\n')
                line[strlen(line) - 1] = 0;
            if(strlen(line) > 24) {
                curses_end();
                printf("Dictionary word too long: %s in %s.\n", line, s.dictionaries[s.dict_idx]);
                return 1;
            }
            vector_push_back(dictionary, strdup(line));
        }
        while((status = game(&s, dictionary)) == 1)
            ;
        vector_free(dictionary);
    } while(status == 0);

    vector_free(dictionaries);
}
