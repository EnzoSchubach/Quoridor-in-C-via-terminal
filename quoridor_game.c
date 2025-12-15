#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>


//enums
typedef enum {LOAD = 0, TWOP = 1, THREEP = 2, FOURP = 3, TWOVTWO = 4, PVE = 5, QUIT = 6} option;
typedef enum {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, ENTER = 4, BACK = 5, SPACE = 6, SLASH = 7} input;
typedef enum {P1 = 0, P2 = 1, NONE = 2} winner;
typedef enum {INVALID = 0, VALID = 1, CANCEL = 2, REPEAT = 3} validation;
typedef enum {TOP = 0, BOT = 1, WEST = 2, EAST = 3} spawn;
typedef enum {INSERT = 0, DELETE = 1} instruction;
typedef enum {VERTICAL = 0, HORIZONTAL = 1} orientation;
typedef enum {MOVE = 0, WALL = 1}action;

//structs
struct termios orig_termios;

typedef struct{
    int wall_x;
    int wall_y;
    orientation state;
} wall;

typedef struct {
    char name[10];
    char icon;
    int x[2];
    int y[2];
    wall walls[7];
    int wc;
    int win_pos;
    int wall_breaks;
    action last_action;
} player;


//macros
#define BASE_SIZE 9
#define WALL_SIZE 2
#define N_OPTIONS 6
#define NAME_SIZE 10
#define WALL_CAP 5


//function pre-calls (just so we doesnt need to organize every function in order)
void construct_board(int b_size, char **board);
void print_board(int b_size, char **board);
int select_gamemode(int b_size, char **board);
void player_names(player p[], int n_players);
void setup_players(int b_size, char **board, player p[], int n_players);
void pvp_mode(int b_size, char **board, int n_players);
validation player_actions(int b_size, char **board, player p[], input p_input, int *turn_count, int n_players);
validation wall_actions(int b_size, char **board, int *x, int *y, input in, orientation *state, char **overlay, player *p);
void place_wall(int b_size, char **board, int *x, int *y, instruction mode, orientation *state, bool temporary);
void update_overlay(int b_size, char **board, char **overlay);
input get_input();


void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void enable_raw_mode() {
    struct termios raw;

    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;

    raw.c_lflag &= ~ICANON;
    raw.c_lflag &= ~ECHO;

    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void flush_input(){
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char dump;
    while (read(STDIN_FILENO, &dump, 1) > 0);

    fcntl(STDIN_FILENO, F_SETFL, flags);
}


char **create_board(int b_size) {
    char **board = malloc(b_size * sizeof(char*));
    if (!board) return NULL;

    for (int i = 0; i < b_size; i++) {
        board[i] = malloc(b_size * sizeof(char));
        if (!board[i]) {
            // Se falhar, liberar o que já foi alocado
            for (int j = 0; j < i; j++) {
                free(board[j]);
            }
            free(board);
            return NULL;
        }
    }

    return board;
}

void free_board(int b_size, char **board) {
    if (board == NULL) return;
    
    for (int i = 0; i < b_size; i++) {
        free(board[i]);
    }
    free(board);
}


int main(){
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    enable_raw_mode();
    int b_size = BASE_SIZE*2+1; 
    
    char **board = create_board(b_size);
    if (board == NULL) {
        printf("Erro ao alocar memória para o tabuleiro.\n");
        return 1;
    }

    construct_board(b_size, board); 
    
    option gamemode = select_gamemode(b_size, board);
    switch(gamemode){
        case LOAD: break;

        case TWOP: 
            pvp_mode(b_size, board, 2);
            break;

        case THREEP: 
            pvp_mode(b_size, board, 3);
            break;

        case FOURP:
            pvp_mode(b_size, board, 4);
            break;

        case QUIT: 
            print_board(b_size, board);
            printf("\nThanks for playing!\n\n"); 
            free_board(b_size, board);
            return 0; 
            break;
        case TWOVTWO:
        case PVE:
            break; 
    }
    
    free_board(b_size, board);
    return 0;
}

input get_input(){
    char c = 0;
    read(0, &c, 1);

    if (c == '\033'){ 
        char seq[2];
        read(0, &seq[0], 1);
        read(0, &seq[1], 1);

        if (seq[0] == '['){
            switch (seq[1]){
                case 'A': return UP; break;
                case 'B': return DOWN; break;
                case 'C': return RIGHT; break;
                case 'D': return LEFT; break;
            }
        }
    }
    else{
        switch (c){
            case '\n': return ENTER; break;
            case 127: return BACK; break;
            case ' ': return SPACE; break;
            case '/': return SLASH; break;
        }
    }
    return -1;
}


void construct_board(int b_size, char **board) {
    for (int i = 0; i < b_size; i++) {
        for (int j = 0; j < b_size; j++) {
            
            if (i % 2 == 0 && j % 2 == 0)
                board[i][j] = '+';

            else if (i%2 == 0)
                if (i == 0 || i == b_size-1) board[i][j] = '=';
                else board[i][j] = '-';

            else if (j%2 == 0)
                if (j == 0 || j == b_size-1) board[i][j] = 'I';
                else board[i][j] = '|';

            else
                board[i][j] = ' ';
        }
    }
}

void print_board(int n, char **b){
    system("clear");
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            char c = b[i][j];
            switch(c){
                case '=':
                case 'I':
                    printf(j%2==0 ? " \033[31m%c\033[0m " : "\033[31m%c\033[0m", c);
                    break;
                case '+':
                case '-':
                case '|':
                    printf(j%2==0 ? " \033[33m%c\033[0m " : "\033[33m%c\033[0m", c);
                    break;

                case '~':
                case ';':
                    printf(j%2==0 ? " \033[1;32m%c\033[0m " : "\033[1;32m%c\033[0m", c);
                    break;
                
                default:
                    printf(j%2==0 ? " %c " : "%c", c);
            }
        }
        printf("\n");
    }
}



int select_gamemode(int b_size, char **board){ 
    int aux = -1, i = 0;
    char arr[] = {'<', ' ', ' ', ' ', ' ', ' '};

    while(aux != ENTER){

        print_board(b_size, board);
        printf("\n\n\033[34mWelcome to Quoridor made by Enzo and Joao Cleber\nPlease select an option\033[0m\n\n");
        printf("\033[34mLOAD SAVE\033[0m %c         \033[34m2P\033[0m %c         \033[34m3P\033[0m %c         \033[34m4P\033[0m %c         \033[34m2v2\033[0m %c         \033[34mLEAVE GAME\033[0m %c\n", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
        
        aux = get_input();
        while(aux != LEFT && aux != RIGHT && aux != ENTER){
            aux = get_input(); 
        }

        switch(aux){
            case LEFT:
                arr[i] = ' ';
                i = (i - 1 + N_OPTIONS) % N_OPTIONS; 
                arr[i] = '<';
                break;

            case RIGHT:
                arr[i] = ' ';
                i = (i + 1) % N_OPTIONS; 
                arr[i] = '<';
                break;
        }
    }
    return i;
}

void discard_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void player_names(player p[], int n_players){
    system("clear");
    disable_raw_mode();

    for(int i = 0; i < n_players; i++){
        printf("Player %d, insert your name: ", i+1);
        fflush(stdout);

        fgets(p[i].name, NAME_SIZE, stdin);
        if (!strchr(p[i].name, '\n')) {
            discard_line();
        } else {
            p[i].name[strcspn(p[i].name, "\n")] = '\0';
        }
    }

    enable_raw_mode();
}


void player_spawn(int b_size, spawn spw, int *x, int *y) {
    int middle = b_size / 2;

    switch (spw) {
        case TOP:
            *x = 1;
            *y = middle;
            break;

        case BOT:
            *x = b_size - 2;
            *y = middle;
            break;

        case WEST:
            *x = middle;
            *y = 1;
            break;

        case EAST:
            *x = middle;
            *y = b_size - 2;
            break;
    }
}

void setup_players(int b_size, char **board, player p[], int n_players) {

    spawn sides[4] = {TOP, BOT, EAST, WEST};

    for (int i = 0; i < n_players; i++) {
        p[i].icon = p[i].name[0] ? p[i].name[0] : '?';

        player_spawn(b_size, sides[i], &p[i].x[0], &p[i].y[0]);

        board[p[i].x[0]][p[i].y[0]] = p[i].icon;

        p[i].x[1] = -1;
        p[i].y[1] = -1;

        p[i].wc = 0;
        p[i].wall_breaks = 1;

        switch (sides[i]) {
            case TOP:
                p[i].win_pos = b_size - 2;
                break;
            case BOT:
                p[i].win_pos = 1;
                break;
            case WEST:
                p[i].win_pos = b_size - 2;
                break;
            case EAST:
                p[i].win_pos = 1;
                break;
        }
    }
}


winner check_win(player p[], int current_player_index) {
    int i = current_player_index;
    int target_pos = p[i].win_pos;
    
    if (i == 0 || i == 1) { 
        if (p[i].x[0] == target_pos) {
            return (winner)i;
        }
    } 
    else if (i == 2 || i == 3) { 
        if (p[i].y[0] == target_pos) {
            return (winner)i;
        }
    }

    return NONE;
}

void pvp_mode(int b_size, char **board, int n_players){
    player p[n_players];

    player_names(p, n_players);
    setup_players(b_size, board, p, n_players);
    print_board(b_size, board);

    winner game_winner = NONE;
    int turn_count = 0;
    validation check = INVALID;

    while(game_winner == NONE){
        turn_count+=1;
        int current_player_index = turn_count % n_players;
        flush_input();

        while(check == INVALID){
            print_board(b_size, board);
            printf("\n\033[34mTURN %d\nIt's your turn %s\nMake your move!\n%d/%d Walls left\n\033[0m", turn_count, p[current_player_index].name, p[current_player_index].wc, WALL_CAP);
            input a = get_input();
            check = player_actions(b_size, board, p, a, &turn_count, n_players);
            flush_input();
            print_board(b_size, board);
        }
        
        if (check == VALID) {
            game_winner = check_win(p, current_player_index);
        }

        check = INVALID;
        sleep(1);
        
    }
    
    printf("\n\033[1;32m*** Fim do Jogo! ***\033[0m\n");
    if (game_winner != NONE) {
        printf("\033[1;32mO vencedor é: %s!\033[0m\n", p[game_winner].name);
    }
}

void realloc_history(player *p){
    p->x[1] = p->x[0];
    p->y[1] = p->y[0];
}

validation player_actions(int b_size, char **board, player p[], input p_input, int *turn_count, int n_players){
    
    int i = *turn_count%n_players; 

    switch(p_input){
        case UP:
            if(board[(p[i].x[0])-1][(p[i].y[0])] == '='){
                return INVALID;
            }
            for(int j = 0; j < n_players; j++){
                if(board[(p[i].x[0])-2][(p[i].y[0])] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].x[0] -= 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            p[i].last_action = MOVE;

            break;

        case DOWN:
            if(board[(p[i].x[0])+1][(p[i].y[0])] == '='){
                return INVALID;
            }
            for(int j = 0; j < n_players; j++){
                if(board[(p[i].x[0])+2][(p[i].y[0])] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].x[0] += 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            p[i].last_action = MOVE;

            break;

        case LEFT:
            if(board[(p[i].x[0])][(p[i].y[0])-1] == 'I'){
                return INVALID;
            }
            for(int j = 0; j < n_players; j++){
                if(board[(p[i].x[0])][(p[i].y[0])-2] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].y[0] -= 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            p[i].last_action = MOVE;

            break;

        case RIGHT:
            if(board[(p[i].x[0])][(p[i].y[0])+1] == 'I'){
                return INVALID;
            }
            for(int j = 0; j < n_players; j++){
                if(board[(p[i].x[0])][(p[i].y[0])+2] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].y[0] += 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            p[i].last_action = MOVE;

            break;

        case BACK:
            int aux = (*turn_count - 1 + n_players) % n_players;

            if (p[aux].last_action == WALL) {
                if (p[aux].wc <= 0) {
                    return INVALID;
                }
                
                int wall_index = p[aux].wc - 1;
                
                place_wall(b_size, board, 
                           &p[aux].walls[wall_index].wall_x,
                           &p[aux].walls[wall_index].wall_y, 
                           DELETE,
                           (orientation *)&p[aux].walls[wall_index].state, 
                           false);
                
                p[aux].wc -= 1;
                p[aux].last_action = MOVE; 
            } 
            else if (p[aux].last_action == MOVE) {
                 if(p[aux].x[1] == -1){ 
                    return INVALID;
                 }
                
                board[p[aux].x[0]][p[aux].y[0]] = ' ';
                p[aux].x[0] = p[aux].x[1];
                p[aux].y[0] = p[aux].y[1];
                board[p[aux].x[0]][p[aux].y[0]] = p[aux].icon;

                p[aux].x[1] = -1; 
                p[aux].y[1] = -1;
                p[aux].last_action = MOVE;
            } else {
                return INVALID;
            }

            *turn_count -= 2;
            return VALID;

        case SPACE: {
            if(p[i].wc >= WALL_CAP){
                return INVALID;
            }

            validation verify = INVALID;
            input in = -1;
            orientation direction = HORIZONTAL;
            
            // Aloca dinamicamente o overlay para manter a consistência com char**
            char **overlay = create_board(b_size);
            if (overlay == NULL) {
                return INVALID;
            }

            p[i].walls[p[i].wc].wall_x = 4;
            p[i].walls[p[i].wc].wall_y = 1;

            update_overlay(b_size, board, overlay); 
            for(int k = 0; k < 2; k++){
                overlay[4][1 + k*2] = '~';
            }

            print_board(b_size, overlay);

            while(1){
                update_overlay(b_size, board, overlay); 
                in = get_input();

                verify = wall_actions(
                    b_size, board,
                    &p[i].walls[p[i].wc].wall_x,
                    &p[i].walls[p[i].wc].wall_y,
                    in, &direction, overlay, &p[i]
                );

                print_board(b_size, overlay);

                if(verify == VALID) {
                    free_board(b_size, overlay);
                    p[i].last_action = WALL;
                    return VALID;
                }
                if(verify == CANCEL) {
                    free_board(b_size, overlay);
                    return INVALID; 
                }
            }
            break;
        }

            

        case ENTER:
            break;
        case SLASH:
            break;
    }
    return VALID;
}

char base_tile(int i, int j){
    if(i % 2 == 0 && j % 2 == 0) return '+';
    if(i % 2 == 0) return '-';
    if(j % 2 == 0) return '|';
    return ' ';
}

void place_wall(int b_size, char **board, int *x, int *y, instruction mode, orientation *state, bool temporary) {
    char char_v = temporary ? ';' : 'I';
    char char_h = temporary ? '~' : '=';

    if (*state == VERTICAL) {
        for (int i = 0; i <= 2; i += 2) { 
            board[*x + i][*y] = (mode == INSERT) ? char_v : base_tile(*x + i, *y);
        }
    } else {
        for (int i = 0; i <= 2; i += 2) {
            board[*x][*y + i] = (mode == INSERT) ? char_h : base_tile(*x, *y + i);
        }
    }
}

void update_overlay(int b_size, char **board, char **overlay){
    for(int i = 0; i < b_size; i++){
        for(int j = 0; j < b_size; j++){
            overlay[i][j] = board[i][j];
        }
    }
}



validation wall_actions(int b_size, char **board, int *x, int *y, input in, orientation *state, char **overlay, player *p){
    orientation old_state = *state;
    int old_x = *x;
    int old_y = *y;
    
    switch(in){
        case UP: 
            if (*x - 2 < 1) return INVALID;

            place_wall(b_size, overlay, &old_x, &old_y, DELETE, &old_state, true);
            *x -= 2;
            place_wall(b_size, overlay, x, y, INSERT, state, true);
            return REPEAT;

        case DOWN: 
            int limit_down = (*state == VERTICAL) ? *x + 2 : *x;
            if (limit_down + 2 >= b_size - 1) return INVALID;

            place_wall(b_size, overlay, &old_x, &old_y, DELETE, &old_state, true);
            *x += 2;
            place_wall(b_size, overlay, x, y, INSERT, state, true);
            return REPEAT;

        case LEFT: 
            if (*y - 2 < 1) return INVALID;

            place_wall(b_size, overlay, &old_x, &old_y, DELETE, &old_state, true);
            *y -= 2;
            place_wall(b_size, overlay, x, y, INSERT, state, true);
            return REPEAT;

        case RIGHT:
            int limit_right = (*state == HORIZONTAL) ? *y + 2 : *y;
            if (limit_right + 2 >= b_size - 1) return INVALID;

            place_wall(b_size, overlay, &old_x, &old_y, DELETE, &old_state, true);
            *y += 2;
            place_wall(b_size, overlay, x, y, INSERT, state, true);
            return REPEAT;

        case ENTER:
            for(int i = 0; i < WALL_SIZE*2; i+=2){
                if((*state == VERTICAL) && board[*x + i][*y] != '|') return INVALID;
                if((*state == HORIZONTAL) && board[*x][*y + i] != '-') return INVALID;
            }
            
            int index = p->wc; 
            p->walls[index].wall_x = *x;
            p->walls[index].wall_y = *y;
            p->walls[index].state = *state;

            place_wall(b_size, board, x, y, INSERT, state, false);
            p->wc+=1;

            return VALID;

        case SPACE:
            place_wall(b_size, overlay, &old_x, &old_y, DELETE, &old_state, true);

            if (*state == HORIZONTAL) {
                *state = VERTICAL;
                *x -= 1;
                *y += 1;
            } else {
                *state = HORIZONTAL;
                *x += 1;
                *y -= 1;
            }

            if (*x < 1 || *x + (*state == VERTICAL ? 2 : 0) >= b_size - 1 ||
                *y < 1 || *y + (*state == HORIZONTAL ? 2 : 0) >= b_size - 1) {
                if (*state == VERTICAL) {
                    *state = HORIZONTAL; *x += 1; *y -= 1;
                } else {
                    *state = VERTICAL; *x -= 1; *y += 1;
                }
                return INVALID;
            }

            place_wall(b_size, overlay, x, y, INSERT, state, true);
            return REPEAT;

        case BACK:
            return CANCEL;
        case SLASH:
            break;
    }
    return INVALID;
}