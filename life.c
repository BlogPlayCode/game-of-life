#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include <stdatomic.h>
#include <conio.h>

#define MENU_WIDTH          180
#define ALIVE               '#'
#define DEAD                '.'

#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_YELLOW        6
#define COLOR_WHITE         7
#define COLOR_GRAY          8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_YELLOW  14
#define COLOR_BRIGHT_WHITE  15

#define KEY_ENTER           13
#define ARROW_LEFT          37
#define ARROW_UP            38
#define ARROW_RIGHT         39
#define ARROW_DOWN          40

#define KEY_N               78
#define KEY_P               80
#define KEY_Q               81
#define KEY_R               82

int WIDTH = 50, HEIGHT = 20;
int USE_RAND = 1;
int UPDATE_INTERVAL = 200;
atomic_int last_key = 0;
int LIMIT_WIDTH_MIN = 3;
int LIMIT_WIDTH_MAX = 256;
int LIMIT_HEIGHT_MIN = 3;
int LIMIT_HEIGHT_MAX = 128;
int LIMIT_INTERVAL_MIN = -1;
int LIMIT_INTERVAL_MAX = 10000;

DWORD WINAPI keyboard_handler() {
    Sleep(1000);
    while (1) {
        for (int i=8; i<256; i++) {
            if (GetAsyncKeyState(i) & 0x8000) {
                atomic_store(&last_key, i);
                Sleep(200);
                atomic_store(&last_key, 0);
            }
        }
        Sleep(10);
    }
}

void clear_input_buffer() {
    while (_kbhit()) _getch();
}

void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void print_field(char *field) {
    for (int i=0; i < HEIGHT; i++) {
        for (int j=0; j < WIDTH; j++) {
            if (*(field+i*WIDTH+j) == ALIVE) setColor(COLOR_LIGHT_MAGENTA*16);
            else setColor(COLOR_LIGHT_GREEN*16);
            printf(" %c", *(field+i*WIDTH+j));
        }
        printf("\n");
    }
    setColor(COLOR_WHITE);
    for (int i=0; i<WIDTH*2; i++) printf(" ");
    printf("\n");
}

int init_field(char *field) {
    printf("\e[1;1H\e[2J");
    for (int i=0; i < HEIGHT; i++) {
        for (int j=0; j < WIDTH; j++) {
            *(field+i*WIDTH+j) = DEAD;
            if (USE_RAND && rand()%5 == 0) *(field+i*WIDTH+j) = ALIVE;
        }
    }
    print_field(field);
    setColor(COLOR_WHITE);
    printf("    FIELD SETUP        Q - quit    R - run game\n");
    printf("\033[H");
    printf("\033[C");
    int key;
    int pos_x = 0, pos_y = 0;
    while (1) {
        Sleep(150);
        key = atomic_exchange(&last_key, 0);
        if (key == KEY_Q) {
            for (int i=0; i<HEIGHT-pos_y+3; i++) printf("\033[B");
            return 1;
        } else if (key == KEY_R) {
            break;
        } else if (key == KEY_ENTER) {
            if (*(field+pos_y*WIDTH+pos_x) == ALIVE) *(field+pos_y*WIDTH+pos_x) = DEAD;
            else *(field+pos_y*WIDTH+pos_x) = ALIVE;
            if (*(field+pos_y*WIDTH+pos_x) == ALIVE) setColor(COLOR_LIGHT_MAGENTA*16);
            else setColor(COLOR_LIGHT_GREEN*16);
            printf("\033[D");
            printf(" %c", *(field+pos_y*WIDTH+pos_x));
            printf("\033[D");
        } else if (key == ARROW_RIGHT) {
            if (pos_x+1 < WIDTH) {
                pos_x += 1;
                printf("\033[2C");
            } else {
                pos_x = 0;
                for (int i=0; i<WIDTH-1; i++) printf("\033[2D");
            }
        } else if (key == ARROW_LEFT) {
            if (pos_x-1 >= 0) {
                pos_x -= 1;
                printf("\033[2D");
            } else {
                pos_x = WIDTH-1;
                for (int i=0; i<WIDTH-1; i++) printf("\033[2C");
            }
        } else if (key == ARROW_UP) {
            if (pos_y-1 >= 0) {
                pos_y -= 1;
                printf("\033[A");
            } else {
                pos_y = HEIGHT-1;
                for (int i=0; i<HEIGHT-1; i++) printf("\033[B");
            }
        } else if (key == ARROW_DOWN) {
            if (pos_y+1 < HEIGHT) {
                pos_y += 1;
                printf("\033[B");
            } else {
                pos_y = 0;
                for (int i=0; i<HEIGHT-1; i++) printf("\033[A");
            }
        }
    }
    return 0;
}

void copy_field_from(char *dest, char *origin) {
    for (int i=0; i < HEIGHT; i++) {
        for (int j=0; j < WIDTH; j++) {
            *(dest+i*WIDTH+j) = *(origin+i*WIDTH+j);
        }
    }
}

int do_iteration(char *field) {
    char *old_field;
    old_field = (char *) malloc(WIDTH*HEIGHT * sizeof(char));
    copy_field_from(old_field, field);
    
    // print_field(old_field);
    for (int i=0; i < HEIGHT; i++) {
        for (int j=0; j < WIDTH; j++) {
            int sum_around = 0;
            int points[8][2] = {
                {i-1, j-1},
                {i-1, j},
                {i-1, j+1},
                {i, j-1},
                {i, j+1},
                {i+1, j-1},
                {i+1, j},
                {i+1, j+1},
            };
            for (int p=0; p<8; p++) {
                if (points[p][0] < 0) points[p][0] += HEIGHT;
                if (points[p][0] >= HEIGHT) points[p][0] -= HEIGHT;
                if (points[p][1] < 0) points[p][1] += WIDTH;
                if (points[p][1] >= WIDTH) points[p][1] -= WIDTH;

                int y = points[p][0];
                int x = points[p][1];
                if (*(old_field+y*WIDTH+x) == ALIVE) sum_around++;
            }
            if (sum_around == 3 || sum_around == 2) {
                if (sum_around == 3) *(field+i*WIDTH+j) = ALIVE;
            } else {
                *(field+i*WIDTH+j) = DEAD;
            }
        }
    }
    int must_return = 2;
    for (int i=0; i < HEIGHT; i++) {
        for (int j=0; j < WIDTH; j++) {
            if (*(old_field+i*WIDTH+j) != *(field+i*WIDTH+j)) {
                must_return = 0;
                break;
            }
        }
    }

    free(old_field);

    if (must_return) return must_return;

    for (int i=0; i < HEIGHT; i++) {
        for (int j=0; j < WIDTH; j++) {
            if (*(field+i*WIDTH+j) == ALIVE) return 0;
        }
    }
    return 1;
}

void print_lines() {
    for (int i=0; i<2; i++){
        setColor(COLOR_WHITE);
        printf("\n    "); 
        setColor(COLOR_MAGENTA*16);
        for (int i=0; i<MENU_WIDTH; i++) printf(" ");
    }
    setColor(COLOR_WHITE);
}

void color_wrap(int add_tab, char text[]) {
    setColor(COLOR_WHITE);
    printf("\n    "); 
    setColor(COLOR_MAGENTA*16);
    for (int i=0; i<5; i++) printf(" ");
    setColor(COLOR_WHITE);
    int size = strlen(text), tab = (MENU_WIDTH-5*2-size)/2+add_tab;
    for (int i=0; i<tab; i++) printf(" ");
    printf("%s", text);
    if (size%2 == 1) printf(" ");
    if (add_tab) printf(" ");
    for (int i=0; i<tab; i++) printf(" ");
    setColor(COLOR_MAGENTA*16);
    for (int i=0; i<5; i++) printf(" ");
    setColor(COLOR_WHITE);
}

void print_bg() {
    color_wrap(0, "");
    color_wrap(4, "\033[35m   _____              __  __   ______      ____    ______     _        _____   ______   ______  \033[0m");
    color_wrap(4, "\033[35m  / ____|     /\\     |  \\/  | |  ____|    / __ \\  |  ____|   | |      |_   _| |  ____| |  ____| \033[0m");
    color_wrap(4, "\033[35m | |  __     /  \\    | \\  / | | |__      | |  | | | |__      | |        | |   | |__    | |__    \033[0m");
    color_wrap(4, "\033[35m | | |_ |   / /\\ \\   | |\\/| | |  __|     | |  | | |  __|     | |        | |   |  __|   |  __|   \033[0m");
    color_wrap(4, "\033[35m | |__| |  / ____ \\  | |  | | | |____    | |__| | | |        | |____   _| |_  | |      | |____  \033[0m");
    color_wrap(4, "\033[35m  \\_____| /_/    \\_\\ |_|  |_| |______|    \\____/  |_|        |______| |_____| |_|      |______| \033[0m");
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, ""); // !
    color_wrap(0, ""); // !
    color_wrap(0, ""); // !
    color_wrap(0, ""); // !
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "Press ENTER to edit selected field");
    color_wrap(0, "Press N for next step");
    color_wrap(0, "Press Q for exit");
    color_wrap(0, "");
}

int* print_button(
    int color1, char text1[], 
    int color2, char text2[], 
    int color3, char text3[], 
    int color4, char text4[]
) {
    int distance = 18;
    int len1 = strlen(text1);
    int len2 = strlen(text2);
    int len3 = strlen(text3);
    int len4 = strlen(text4);
    int tab = 4 + (MENU_WIDTH-distance*3-len1-len2-len3-len4)/2;
    printf("\r");
    for (int i=0; i<tab; i++) printf("\033[C");
    setColor(color1);
    printf(text1);
    // printf("%d", len1);
    setColor(COLOR_WHITE);
    for (int i=0; i<distance; i++) printf("\033[C");
    setColor(color2);
    printf(text2);
    // printf("%d", len2);
    setColor(COLOR_WHITE);
    for (int i=0; i<distance; i++) printf("\033[C");
    setColor(color3);
    printf(text3);
    // printf("%d", len3);
    setColor(COLOR_WHITE);
    for (int i=0; i<distance; i++) printf("\033[C");
    setColor(color4);
    printf(text4);
    // printf("%d", len4);
    setColor(COLOR_WHITE);
    printf("\r\033[B");
    int* result;
    result = malloc(8 * sizeof(int));
    result[0] = tab;
    result[1] = len1;
    result[2] = distance;
    result[3] = len2;
    result[4] = distance;
    result[5] = len3;
    result[6] = distance;
    result[7] = len4;
    return result;
}

int* print_buttons(
    int color1, char text1[], 
    int color2, char text2[], 
    int color3, char text3[], 
    int color4, char text4[]
) {
    printf("\r\033[13A");

    int spaces = 6;
    int len1 = strlen(text1);
    int len2 = strlen(text2);
    int len3 = strlen(text3);
    int len4 = strlen(text4);

    char *empty1 = malloc(spaces*2+len1+1);
    char *empty2 = malloc(spaces*2+len2+1);
    char *empty3 = malloc(spaces*2+len3+1);
    char *empty4 = malloc(spaces*2+len4+1);
    memset(empty1, ' ', spaces*2+len1);
    memset(empty2, ' ', spaces*2+len2);
    memset(empty3, ' ', spaces*2+len3);
    memset(empty4, ' ', spaces*2+len4);
    empty1[spaces*2+len1] = '\0';
    empty2[spaces*2+len2] = '\0';
    empty3[spaces*2+len3] = '\0';
    empty4[spaces*2+len4] = '\0';

    char *txt1 = malloc(spaces*2+len1+1);
    char *txt2 = malloc(spaces*2+len2+1);
    char *txt3 = malloc(spaces*2+len3+1);
    char *txt4 = malloc(spaces*2+len4+1);
    memset(txt1, ' ', spaces);
    memset(txt2, ' ', spaces);
    memset(txt3, ' ', spaces);
    memset(txt4, ' ', spaces);
    memcpy(txt1+spaces, text1, len1);
    memcpy(txt2+spaces, text2, len2);
    memcpy(txt3+spaces, text3, len3);
    memcpy(txt4+spaces, text4, len4);
    memset(txt1+spaces+len1, ' ', spaces);
    memset(txt2+spaces+len2, ' ', spaces);
    memset(txt3+spaces+len3, ' ', spaces);
    memset(txt4+spaces+len4, ' ', spaces);
    txt1[spaces*2+len1] = '\0';
    txt2[spaces*2+len2] = '\0';
    txt3[spaces*2+len3] = '\0';
    txt4[spaces*2+len4] = '\0';

    print_button(color1, empty1, color2, empty2, color3, empty3, color4, empty4);
    print_button(color1, txt1, color2, txt2, color3, txt3, color4, txt4);
    print_button(color1, empty1, color2, empty2, color3, empty3, color4, empty4);
    print_button(color1, empty1, color2, empty2, color3, empty3, color4, empty4);
    int* res = print_button(color1, empty1, color2, empty2, color3, empty3, color4, empty4);

    free(empty1);
    free(empty2);
    free(empty3);
    free(empty4);
    free(txt1);
    free(txt2);
    free(txt3);
    free(txt4);

    return res;
}

void print_values(int *cords) {
    for (int i=0; i<cords[0]; i++) printf("\033[C");

    int str_length = 0;
    for (int tmp = WIDTH; tmp != 0; str_length++) tmp /= 10;
    int tab = (cords[1]-str_length)/2;
    for (int i=0; i<tab; i++) printf("\033[C");
    setColor(COLOR_LIGHT_CYAN*16);
    printf("%d", WIDTH);
    setColor(COLOR_WHITE);
    for (int i=tab+str_length; i<cords[1]; i++) printf("\033[C");
    
    for (int i=0; i<cords[2]; i++) printf("\033[C");
    
    str_length = 0;
    for (int tmp = HEIGHT; tmp != 0; str_length++) tmp /= 10;
    tab = (cords[3]-str_length)/2;
    for (int i=0; i<tab; i++) printf("\033[C");
    setColor(COLOR_LIGHT_CYAN*16);
    printf("%d", HEIGHT);
    setColor(COLOR_WHITE);
    for (int i=tab+str_length; i<cords[3]; i++) printf("\033[C");
    
    for (int i=0; i<cords[4]; i++) printf("\033[C");
    
    str_length = 0;
    for (int tmp = UPDATE_INTERVAL; tmp != 0; str_length++) tmp /= 10;
    tab = (cords[5]-str_length)/2;
    for (int i=0; i<tab; i++) printf("\033[C");
    setColor(COLOR_LIGHT_CYAN*16);
    printf("%d", UPDATE_INTERVAL);
    setColor(COLOR_WHITE);
    for (int i=tab+str_length; i<cords[5]; i++) printf("\033[C");
    
    for (int i=0; i<cords[6]; i++) printf("\033[C");
    
    str_length = 0;
    char state = 'X';
    if (USE_RAND) state = 'V';
    tab = (cords[5]-str_length)/2;
    for (int i=0; i<tab; i++) printf("\033[C");
    int random_color = COLOR_RED*16;
    if (USE_RAND) random_color = COLOR_GREEN*16;
    setColor(random_color);
    printf(" %c", state);
    setColor(COLOR_WHITE);
}

void print_menu(int button, int clear) {
    if (clear) printf("\e[1;1H\e[2J");
    else printf("\033[H");

    setColor(COLOR_WHITE);
    for (int i=0; i<3; i++) printf("\n");

    if (clear) {
        print_lines();
        print_bg();
        print_lines();
    } else {
        printf("\033[28B");
    }

    int random_color = COLOR_RED*16;
    if (USE_RAND) random_color = COLOR_GREEN*16;
    
    int* buttons_cords = print_buttons(
        COLOR_LIGHT_CYAN*16, "WIDTH", 
        COLOR_LIGHT_CYAN*16, "HEIGHT", 
        COLOR_LIGHT_CYAN*16, "INTERVAL", 
        random_color, "RANDOM FIELD"
    );

    printf("\r\033[2A");
    print_values(buttons_cords);
    printf("\r\033[A");

    int indent = 0;
    for (int i=0; i<=button; i++) indent += buttons_cords[i*2] + buttons_cords[i*2+1];
    indent -= buttons_cords[button*2+1] / 2;
    if (button < 2) indent--;
    for (int i=0; i<indent; i++) printf("\033[C");
}

int ask_value(char text[]) {
    printf("\e[1;1H\e[2J");
    setColor(COLOR_WHITE);
    for (int i=0; i<3; i++) printf("\n");

    print_lines();

    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, text);
    color_wrap(0, "");
    color_wrap(0, ""); // !
    color_wrap(0, "");
    color_wrap(0, "");
    color_wrap(0, "");

    print_lines();

    printf("\r\033[5A");
    for (int i=0; i<MENU_WIDTH/2; i++) printf("\033[C");
    
    int result;
    clear_input_buffer();
    scanf("%d", &result);
    return result;
}

int start_menu() {
    print_menu(0, 1);

    int key = 0, button = 0;
    while (1) {
        Sleep(100);
        key = atomic_exchange(&last_key, 0);
        // printf("\r%d", key);
        // continue;
        if (key == KEY_N) return 0;
        else if (key == KEY_Q) {
            printf("\e[1;1H\e[2J");
            return 1;
        } else if (key == ARROW_RIGHT) {
            button++;
            if (button > 3) button = 0;
            print_menu(button, 0);
        } else if (key == ARROW_LEFT) {
            button--;
            if (button < 0) button = 3;
            print_menu(button, 0);
        } else if (key == KEY_ENTER) {
            if (button == 0) {
                WIDTH = ask_value("ENTER WIDTH (DEFAULT 50)");
                if (WIDTH < LIMIT_WIDTH_MIN) WIDTH = LIMIT_WIDTH_MIN;
                else if (WIDTH > LIMIT_WIDTH_MAX) WIDTH = LIMIT_WIDTH_MAX;
                print_menu(button, 1);
            } else if (button == 1) {
                HEIGHT = ask_value("ENTER HEIGHT (DEFAULT 20)");
                if (HEIGHT < LIMIT_HEIGHT_MIN) HEIGHT = LIMIT_HEIGHT_MIN;
                else if (HEIGHT > LIMIT_HEIGHT_MAX) HEIGHT = LIMIT_HEIGHT_MAX;
                print_menu(button, 1);
            } else if (button == 2) {
                UPDATE_INTERVAL = ask_value("ENTER GAME UPDATE INTERVAL (DEFAULT 200, -1 TO USE ENTER INSTEAD)");
                if (UPDATE_INTERVAL < LIMIT_INTERVAL_MIN) UPDATE_INTERVAL = LIMIT_INTERVAL_MIN;
                else if (UPDATE_INTERVAL > LIMIT_INTERVAL_MAX) UPDATE_INTERVAL = LIMIT_INTERVAL_MAX;
                print_menu(button, 1);
            } else if (button == 3) {
                if (USE_RAND) USE_RAND = 0;
                else USE_RAND = 1;
                print_menu(button, 0);
            }
        }
    }
}

int main_cycle(char *field) {
    int result = 0, pause = 0;
    int key;
    for (int iter=0; !result;) {
        if (pause) {
            Sleep(50);
            key = atomic_exchange(&last_key, 0);
            if (key == KEY_Q) {
                printf("\n [COMMAND] User exit...\n");
                return 0;
            } else if (key == KEY_R) {
                printf("\n [COMMAND] Resuming...\n");
                pause = 0;
                printf("\e[1;1H\e[2J");
                continue;
            }
        } else {
            iter++;
            result = do_iteration(field);
            if (UPDATE_INTERVAL >= 0) {
                key = atomic_exchange(&last_key, 0);
                if (key == KEY_Q) {
                    printf("\n [COMMAND] User exit...\n");
                    clear_input_buffer();
                    return 0;
                } else if (key == KEY_P) {
                    printf("\n [COMMAND] Paused         Q - quit    R - resume\n");
                    pause = 1;
                    continue;
                }
            }
            printf("\033[H"); 
            print_field(field);
            setColor(COLOR_WHITE);
            printf("    ITERATION #%d         Q - quit    P - pause    ", iter);
            if (UPDATE_INTERVAL < 0) printf("ENTER - next iteration");
            printf("\n");
            if (UPDATE_INTERVAL < 0) {
                int must_continue = 0;
                while (1) {
                    key = atomic_exchange(&last_key, 0);
                    if (key == KEY_ENTER) break;
                    else if (key == KEY_Q) {
                        printf("\n [COMMAND] User exit...\n");
                        clear_input_buffer();
                        return 0;
                    } else if (key == KEY_P) {
                        printf("\n [COMMAND] Paused         Q - quit    R - resume\n");
                        pause = 1;
                        must_continue = 1;
                        break;
                    }
                    Sleep(50);
                }
                if (must_continue) continue;
            }
            else Sleep(UPDATE_INTERVAL);
        }
    }
    if (result == 1) printf("\n [GAME] ALL CELLS DEAD \n");
    else if (result == 2) printf("\n [GAME] REPEAT DETECTED \n");
    return 0;
}

int main() {
    srand(time(NULL));
    HANDLE keyboardThread;
    DWORD threadID;
    keyboardThread = CreateThread(
        NULL,
        0,
        keyboard_handler,
        NULL,
        0,
        &threadID
    );
    if (keyboardThread == NULL) {
        printf("Keyboard thread is dead!\n");
        return 1;
    }
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        printf("Ошибка получения информации о консоли\n");
        return 1;
    }
    int con_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int con_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    LIMIT_WIDTH_MAX = con_width - 3;
    LIMIT_HEIGHT_MAX = con_height - 3;
    
    int result;
    result = start_menu();
    if (result) {
        clear_input_buffer();
        return 1;
    }
    char *field;
    field = (char *) malloc(WIDTH*HEIGHT * sizeof(char));
    printf("\e[1;1H\e[2J");
    result = init_field(field);
    if (result) {
        clear_input_buffer();
        return 1;
    }
    result = main_cycle(field);
    clear_input_buffer();
    return result;
}
