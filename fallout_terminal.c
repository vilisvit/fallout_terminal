#include <curses.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int get_wordlist(int num, int len, char words[num][len+1]);
void generate_block(int block_len, int words_num, int words_len, int min_dist, char words[words_num][words_len], char block[block_len+1], int positions[]);
void console_print (bool enter_pressed, const int console_width, const int console_height, const char line_to_print[console_width + 1], char console[console_width][console_height]);
int check_probability(int len, char secret_word[len + 1], char guessed_word[len + 1]);
void dec_to_hex (int dec, char hex[6]);
void draw_screen(int max_spec_len, int cell, int words_len, int catch_pos, int attempts_remaining, int part_width, int part_height, char block[part_height * part_width * 2 + 1], int console_width, char console[part_height][console_width]);
int catch_word (int mouse_x, int mouse_y, int part_width, int part_height, char block[], int max_spec_len, int *end_spec, bool guessed_end_spec[part_width * part_height * 2]);
void game(int words_num, int words_len);
void draw_win_screen();
void remove_dud(int words_num, int words_len, int block_len, char block[block_len], int positions[words_num], int secret_word_pos);

int main(int argc, char *argv[]) {
    int words_num = 20, words_len = 4;
    if (argc == 1 || (argc == 3 && strcmp(argv[1], "-l") * strcmp(argv[1], "--level") == 0)) {
        switch ((int)argv[2][0]-48) {
            case 0:
                words_num = 18;
                words_len = 4;
                break;
            case 1:
                words_num = 18;
                words_len = 5;
                break;
            case 2:
                words_num = 14;
                words_len = 6;
                break;
            case 3:
                words_num = 12;
                words_len = 8;
                break;
            case 4:
                words_num = 9;
                words_len = 10;
                break;
        }
        game(words_num, words_len);
    } else {
        printf("Incorrect arguments. -l (0-4) to set difficulty level\n");
    }
    return 0;
}

void game(int words_num, int words_len)
{
    const int part_width = 12, part_height = 16, min_dist = 3, console_width = 18, max_attempts = 4, block_len = part_width * part_height * 2, max_spec_len = 8, cell = 5000 + rand() % 65000; //could be changed for better interface or gameplay
    initscr();
    noecho();
    raw(); //disbales breaking program with Ctrl-C
    curs_set(FALSE); //disables curses cursor visibility
    keypad(stdscr, TRUE); //enables possibility to get mouse buttons klicks
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL); //disables masking mouse events
    mouseinterval(0); //makes mouse events triggered every 0 miliseconds
    printf("\033[?1003h\n"); // makes the terminal report mouse movement events
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    attron(COLOR_PAIR(1));
    srand(time(NULL));
    //game params
    int attempts = max_attempts, end_spec = -1, catch_pos = -1, mouse_initialized = 0;
    bool guessed_end_spec[block_len];
    for (int i = 0; i < block_len; i++) {
        guessed_end_spec[i] = false;
    }
    char console[part_height][console_width + 1];
    for (int i = 0; i < part_height; i++) {
       snprintf(console[i], console_width + 1, "%c", '\0');
    }
    char words[words_num][words_len+1];
    if (get_wordlist(words_num, words_len, words) == 1) {
       return;
    }
    char secret_word[words_len + 1];
    int secret_word_pos = rand() % words_num;
    snprintf(secret_word, words_len + 1, words[secret_word_pos]);
    //printw("%s ", secret_word);
    char block[block_len];
    bool is_blocked = false;
    int positions[words_num];
    generate_block(block_len, words_num, words_len, min_dist, words, block, positions);
    mvprintw(0, 0, "Make a click to initialize your mouse...");
    refresh();
    //main loop
    while (1) {
        int c = wgetch(stdscr);
        MEVENT event;
        if (getmouse(&event) == OK) {
            int t = catch_word(event.x, event.y, part_width, part_height, block, max_spec_len, &end_spec, guessed_end_spec);
            if (t != -1)
                catch_pos = t;
            if (mouse_initialized) {
                //getting current selected item to print it to console
                char line_to_print[console_width + 1];
                if (isupper(block[catch_pos])) {
                    for (int i = 0; i < words_len; i++)
                        line_to_print[i] = block[catch_pos+i];
                    line_to_print[words_len] = '\0';
                } else if (end_spec != -1) {
                    for (int i = 0; i <= end_spec - catch_pos; i++)
                        line_to_print[i] = block[catch_pos+i];
                    line_to_print[end_spec - catch_pos + 1] = '\0';
                }
                else {
                    line_to_print[0] = block[catch_pos];
                    line_to_print[1] = '\0';
                }
                if (event.bstate & BUTTON1_PRESSED) {
                    if (!is_blocked) {
                        console_print(true, console_width, part_height, line_to_print, console);
                        if (isupper(line_to_print[0])) {
                            int probability = check_probability(words_len, secret_word, line_to_print);
                            if (probability < words_len) {
                                attempts--;
                                console_print(true, console_width, part_height, "Entry denied.", console);
                                char probability_text[console_width + 1];
                                snprintf(probability_text, console_width + 1, "Likeness=%d", probability);
                                console_print(true, console_width, part_height, probability_text, console);
                                if (attempts <= 0) {
                                    //lose event
                                    console_print(true, console_width, part_height, "Attempts are over.", console);
                                    console_print(true, console_width, part_height, "Password was:", console);
                                    console_print(false, console_width, part_height, secret_word, console);
                                    is_blocked = true;
                                }
                            } else {
                                //win event
                                draw_win_screen();
                                printf("\033[?1003l\n");
                                c = wgetch(stdscr);
                                if (c == 409) {
                                    getch();
                                }
                                break;
                            }
                        } else if (end_spec != -1 && !guessed_end_spec[end_spec]) {
                            guessed_end_spec[end_spec] = true;
                            if (rand() % 2 == 0) {
                                //delete incorrect word
                                remove_dud(words_num, words_len, block_len, block, positions, secret_word_pos);
                                console_print(true, console_width, part_height, "Variant removed.", console);
                            } else {
                                //reset attempts
                                attempts = max_attempts;
                                console_print(true, console_width, part_height, "Tries reset.", console);
                            }
                        }
                        else {
                            console_print(true, console_width, part_height, "Error", console);
                        }
                    } else {
                        break;
                    }
                }
                if (catch_pos != -1 && !is_blocked) {
                    console_print(false, console_width, part_height, line_to_print, console);
                }
            }
            else {
                mouse_initialized = 1;
            }
            draw_screen(end_spec, cell, words_len, catch_pos, attempts, part_width, part_height, block, console_width, console);
        }

        if (c == 3) //correct way to break the program with Ctrl-C
            break;
    }

    attroff(COLOR_PAIR(1));
    printf("\033[?1003l\n"); // disable mouse movement events
    endwin();
}

void console_print (bool enter_pressed, const int console_width, const int console_height, const char line_to_print[console_width + 1], char console[console_height][console_width + 1]) {
    snprintf(console[0], console_width + 1, line_to_print);
    if (enter_pressed) {
        for (int i = console_height - 1; i >= 0; i--) {
            snprintf(console[i], console_width + 1, console[i-1]);
        }
        snprintf(console[0], console_width + 1, "%c", '\0');
    }
}


int get_wordlist(int num, int len, char words[num][len+1]) {
    refresh();
    // check if file exists first and is readable
    char filename[20];
    snprintf(filename, 12, "words%d.txt", len);
    FILE *fp = fopen(filename, "rb");
    if( fp == NULL ){
        printf("\033[?1003l\n");
        endwin();
        fprintf(stderr, "No such file or directory: %s\n", filename);
        return 1;
    }
    // get the filesize first
    struct stat st;
    stat(filename, &st);
    long int size = st.st_size;
    refresh();
    for (int i = 0; i < num; i++) {
        do{
            // generate random number between 0 and filesize
            long int random = (rand() % size) + 1;
            // seek to the random position of file
            fseek(fp, random, SEEK_SET);
            // get next word in row
            char lower_word[len + 1];
            char upper_word[len + 1];
            int result = fscanf(fp, "%*s %20s", lower_word);
            for (int i = 0; i < len; i++) {
                upper_word[i] = toupper(lower_word[i]);
            }
            snprintf(words[i], len + 1, upper_word);
            //check if the word was already chosen
            bool is_already_chosen = false;
            for (int j = 0; j < i; j++) {
                if (strcmp(words[j], words[i]) == 0)
                    is_already_chosen = true;
            }
            if( result != EOF && !is_already_chosen)
                break;
        } while(1);
    }
    fclose(fp);
    return 0;
}

int check_probability (int len, char secret_word[len + 1], char guessed_word[len + 1]) {
    int num = 0;
    for (int i = 0; i < len; i++) {
        if (secret_word[i] == guessed_word[i])
            num++;
    }
    return num;
}

void generate_block(int block_len, int words_num, int words_len, int min_dist, char words[words_num][words_len+1], char block[block_len+1], int positions[words_num]) {
    //getting random ascii special charachters adnd filling block with them
    for (int i = 0; i < block_len; i++) {
        int randnum = (char)(rand() % 30);
        if (randnum < 5)
            randnum += 33;
        else if (randnum < 4+9)
            randnum += 39-5;
        else if (randnum < 4+9+7)
            randnum += 58-(4+9);
        else if (randnum < 4+9+7+6)
            randnum += 91-(4+9+7);
        else
            randnum += 123-(4+9+7+6);
        block[i] = (char)randnum;
    }
    //choosing word positions
    for (int i = 0; i < words_num; i++) {
        bool overlap;
        int position;
        do {
            position = rand() % (block_len - words_len);
            overlap = false;
            for (int j = 0; j < i; j++) {
                if (position > positions[j] - min_dist - words_len && position < positions[j] + words_len + min_dist)
                    overlap = true;
            }
        } while (overlap);
        positions[i] = position;
    }
    //putting words on their positions
    for (int i = 0; i < words_num; i++) {
        for (int j = 0; j < words_len; j++) {
            block[positions[i]+j] = words[i][j];
        }
    }
}
void remove_dud(int words_num, int words_len, int block_len, char block[block_len], int positions[words_num], int secret_word_pos) {
    //replacing random word in block with '.'
    int randpos = (rand() % words_num);
    while (randpos == secret_word_pos || block[positions[randpos]] == '.') {
        randpos = (rand() % words_num);
    }
    for (int i = 0; i < words_len; i++) {
        block[positions[randpos] + i] = '.';
    }
}

void dec_to_hex (int dec, char hex[6]) {
    //converting decimal int to hexadecimal string
    hex[0] = '0';
    hex[1] = 'x';
    int i = 5;
    while (dec != 0)
    {
        int r = dec % 16;
        if (r < 10)
            hex[i--] = 48 + r;
        else
            hex[i--] = 55 + r;
        dec = dec / 16;
    }
}

void draw_screen(int end_spec, int cell, int words_len, int catch_pos, int attempts_remaining, int part_width, int part_height, char block[part_height * part_width * 2 + 1], int console_width, char console[part_height][console_width + 1]) {
    attron(COLOR_PAIR(1));
    move(0, 0);
    clrtoeol();
    int x, y, position; //current curses cursor position
    //drawing 2 parts with characters from block
    for (int i = 0; i < part_height; i++) {
        for (int j = 0; j < part_width; j++) {
            y = 7 + i;
            x = 8 + j;
            position = i * part_width + j;
            if (catch_pos != -1) {
                if (isupper(block[catch_pos])) {
                    if (position >= catch_pos && position < catch_pos + words_len) {
                        attroff(COLOR_PAIR(1));
                        attron(COLOR_PAIR(2));
                    } else {
                        attroff(COLOR_PAIR(2));
                        attron(COLOR_PAIR(1));
                    }
                } else {
                    if (position == catch_pos) {
                        attroff(COLOR_PAIR(1));
                        attron(COLOR_PAIR(2));
                    }
                    else {
                        if (end_spec == -1 || position > end_spec) {
                            attroff(COLOR_PAIR(2));
                            attron(COLOR_PAIR(1));
                        }
                    }
                }
            }
            mvprintw(y, x, "%c", block[position]);
        }
    }
    for (int i = 0; i < part_height; i++) {
        for (int j = 0; j < part_width; j++) {
            y = 7 + i;
            x = 16 + part_width  + j;
            position = part_width * part_height + i * part_width + j;
            if (catch_pos != -1) {
                if (isupper(block[catch_pos])) {
                    if (position >= catch_pos && position < catch_pos + words_len) {
                        attroff(COLOR_PAIR(1));
                        attron(COLOR_PAIR(2));
                    } else {
                        attroff(COLOR_PAIR(2));
                        attron(COLOR_PAIR(1));
                    }
                } else {
                    if (position == catch_pos) {
                        attroff(COLOR_PAIR(1));
                        attron(COLOR_PAIR(2));
                    }
                    else {
                        if (end_spec == -1 || position > end_spec) {
                            attroff(COLOR_PAIR(2));
                            attron(COLOR_PAIR(1));
                        }
                    }
                }
            }
            mvprintw(y, x, "%c", block[position]);
        }
    }
    attroff(COLOR_PAIR(2));
    attron(COLOR_PAIR(1));
    //drawing "memory cells"
    for (int i = 0; i < part_height; i++) {
        char hex_cell[6];
        dec_to_hex(cell + i * part_width, hex_cell);
        for (int j = 0; j < 6; j++) {
            y = 7 + i;
            x = 1 + j;
            mvprintw(y, x, "%c", hex_cell[j]);
        }
    }
    for (int i = 0; i < part_height; i++) {
        char hex_cell[6];
        dec_to_hex(part_width * part_height + cell + i * part_width, hex_cell);
        for (int j = 0; j < 6; j++) {
            y = 7 + i;
            x = part_width + 9 + j;
            mvprintw(y, x, "%c", hex_cell[j]);
        }
    }
    //drawing text on top of screen
    mvprintw(1, 1, "Welcome to \"Password Guesser\" game");
    mvprintw(3, 1, "Password Required");
    mvprintw(5, 1, "Attempts Remaining:");
    clrtoeol();
    for (int i = 0; i < attempts_remaining; i++)
        printw(" #");
    //drawing "console"
    for (int i = 0; i < part_height; i++) {
        y = 7 + (part_height - i - 1);
        x = part_width * 2 + 17;
        mvprintw(y, x, ">");
        clrtoeol();
        mvprintw(y, x + 1, "%s", console[i]);
    }
    attroff(COLOR_PAIR(1));
}

void draw_win_screen() {
    clear();
    attron(COLOR_PAIR(1));
    mvprintw(1, 1, "Welcome to \"Password Guesser\" game");
    mvprintw(3, 1, "Access allowed!");
}

int catch_word (int mouse_x, int mouse_y, int part_width, int part_height, char block[], int max_spec_len, int *end_spec, bool guessed_end_spec[part_height * part_width * 2]) {
    //getting first position of the word or end position of special character construction
    int catch_pos;
    if (mouse_x >= 8 && mouse_x < 8 + part_width && mouse_y >= 7 && mouse_y < 7 + part_height) {
        catch_pos = (mouse_y - 7) * part_width + mouse_x - 8;
        while (catch_pos > 0 && isupper(block[catch_pos - 1]) && isupper(block[catch_pos]))
            catch_pos--;
    } else if (mouse_x >= 16 + part_width && mouse_x < 16 + part_width * 2 && mouse_y >= 7 && mouse_y < 7 + part_height) {
        catch_pos = part_width * part_height + (mouse_y - 7) * part_width + mouse_x - (16 + part_width);
        while (catch_pos > 0 && isupper(block[catch_pos - 1]) && isupper(block[catch_pos]))
            catch_pos--;
    } else
        return -1;
    *end_spec = -1;
    if (block[catch_pos] == '<')
    {
        for (int n = 0; n < max_spec_len; n++) {
            if (isupper(block[catch_pos + n]) || catch_pos % part_width + n >= part_width)
                break;
            if (block[catch_pos + n] == '>') {
                *end_spec = catch_pos + n;
                break;
            }
        }
    } else if (block[catch_pos] == '{') {
        for (int n = 0; n < max_spec_len; n++) {
            if (isupper(block[catch_pos + n]) || catch_pos % part_width + n >= part_width)
                break;
            if (block[catch_pos + n] == '}') {
                *end_spec = catch_pos + n;
                break;
            }
        }
    } else if (block[catch_pos] == '(') {
        for (int n = 0; n < max_spec_len; n++) {
            if (isupper(block[catch_pos + n]) || catch_pos % part_width + n >= part_width)
                break;
            if (block[catch_pos + n] == ')') {
                *end_spec = catch_pos + n;
                break;
            }
        }
    } else if (block[catch_pos] == '[') {
        for (int n = 0; n < max_spec_len; n++) {
            if (isupper(block[catch_pos + n]) ||  catch_pos % part_width + n >= part_width)
                break;
            if (block[catch_pos + n] == ']') {
                *end_spec = catch_pos + n;
                break;
            }
        }
    }
    if (guessed_end_spec[*end_spec])
        *end_spec = -1;
    return catch_pos;
}
