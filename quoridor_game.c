#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>


//structs
struct termios orig_termios;
typedef struct {
    char name[10];
    char icon;
    int x[2];
    int y[2];
} player;

//enums
typedef enum {LOAD = 0, PVP = 1, PVE = 2, QUIT = 3} option;
typedef enum {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, ENTER = 4, BACK = 5, SPACE = 6} input;
typedef enum {P1 = 0, P2 = 1, NONE = 2} winner;
typedef enum {INVALID = 0, VALID = 1} validation;
typedef enum {TOP = 0, BOT = 1, WEST = 2, EAST = 3} spawn;

//macros
#define BASE_SIZE 9 //24 é o tamanho máximo onde dá pra ver o mata todo + texto no VScode
#define WALL_SIZE 2
#define N_OPTIONS 4
#define MAX_BOTS 3
#define NAME_SIZE 10
#define N_PLAYERS 3

//robot names
const char name_list[MAX_BOTS][NAME_SIZE] = {
    "BILL",
    "PULGA",
    "JUBA"
};

//function pre-calls (just so we doesnt need to organize every function in order)
void construct_board(int b_size, char board[b_size][b_size]);
void print_board(int b_size, char board[b_size][b_size]);
int select_gamemode(int b_size, char board[b_size][b_size]);
void player_names(option mode, player p[]);
void setup_players(int b_size, char board[b_size][b_size], player p[]);
void pvp_mode(int b_size, char board[b_size][b_size]);
validation player_actions(int b_size, char board[b_size][b_size], player p[], input p_input, int *turn_count);


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
    // torna o stdin não-bloqueante
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char dump;
    while (read(STDIN_FILENO, &dump, 1) > 0);

    // volta pro modo bloqueante normal
    fcntl(STDIN_FILENO, F_SETFL, flags);
}



//9x9 map, matrix has to be 9*2 x 9*2. 18x18, so the walls can be placed inside the matrix
int main(){
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    enable_raw_mode();
    int b_size = BASE_SIZE*2+1; //control variable
    char board[b_size][b_size]; //creates the board

    construct_board(b_size, board); // calls the function to prepare the board for play
    print_board(b_size, board);
    
    option gamemode = select_gamemode(b_size, board);
    switch(gamemode){
        case LOAD: break;
            //load save

        case PVP: 
            pvp_mode(b_size, board);
            break;

        case PVE: 
            // player_names(gamemode);
            break;

        case QUIT: 
            print_board(b_size, board);
            printf("\nThanks for playing!\n\n"); 
            return 0; 
            break;
    }
}

input get_input(){
    char c = 0; //intializes as 0
    read(0, &c, 1); //reads the first byte of what the user typed

    if (c == '\033'){ //checks if the first byte is ESC (arrows are a combination of ESC + [ + A/B/C/D)
        char seq[2];
        read(0, &seq[0], 1); //stores the other 2 bytes in an array
        read(0, &seq[1], 1);

        if (seq[0] == '['){
            switch (seq[1]){ //checks what arrow is
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
        }
    }
    return -1;
}


void construct_board(int b_size, char board[b_size][b_size]) {
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

void print_board(int n, char b[n][n]){
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
                default:
                    printf(j%2==0 ? " %c " : "%c", c);
            }
        }
        printf("\n");
    }
}



int select_gamemode(int b_size, char board[b_size][b_size]){ 
    int aux = -1, i = 0;
    char arr[] = {'<', ' ', ' ', ' '};

    while(aux != ENTER){

        print_board(b_size, board);
        printf("\n\n\033[34mWelcome to Quoridor made by Enzo and Joao Cleber\nPlease select an option\033[0m\n\n");
        printf("\033[34mLOAD SAVE\033[0m %c         \033[34mPLAYER VS PLAYER\033[0m %c         \033[34mPLAYER VS BOT\033[0m %c         \033[34mLEAVE GAME\033[0m %c\n", arr[0], arr[1], arr[2], arr[3]); //selection screen
        
        aux = get_input();
        while(aux != LEFT && aux != RIGHT && aux != ENTER){
            aux = get_input(); //gets character until its a valid one for the menu
        }

        switch(aux){
            case LEFT:
                arr[i] = ' ';
                i = (i - 1 + N_OPTIONS) % N_OPTIONS; //prevents going above or under 0/2
                arr[i] = '<';
                break;

            case RIGHT:
                arr[i] = ' ';
                i = (i + 1) % N_OPTIONS; //prevents going above or under 0/2
                arr[i] = '<';
                break;
        }
    }
    //undone: needs an if to check if the user has a save, if not, calls recursively the function 
    return i;
}

void player_names(option mode, player p[]){
    system("clear");
    disable_raw_mode();

    if(mode == PVP){
        for(int i = 0; i < N_PLAYERS; i++){
            printf("Player %d, insert your name: ", i+1);
            fflush(stdout);

            fgets(p[i].name, NAME_SIZE, stdin);
            p[i].name[strcspn(p[i].name, "\n")] = '\0';
        }
    }
    else if(mode == PVE){
        printf("Please, insert your name: ");
        fflush(stdout);

        fgets(p[0].name, NAME_SIZE, stdin);
        p[0].name[strcspn(p[0].name, "\n")] = '\0';

        for(int i = 1; i < N_PLAYERS; i++){
            strcpy(p[i].name, name_list[i-1]);
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

void setup_players(int b_size, char board[b_size][b_size], player p[]) {

    spawn sides[4] = {TOP, BOT, EAST, WEST};

    for (int i = 0; i < N_PLAYERS; i++) {
        p[i].icon = p[i].name[0] ? p[i].name[0] : '?';

        player_spawn(b_size, sides[i], &p[i].x[0], &p[i].y[0]);

        board[p[i].x[0]][p[i].y[0]] = p[i].icon;

        p[i].x[1] = -1;
        p[i].y[1] = -1;
    }
}


void pvp_mode(int b_size, char board[b_size][b_size]){
    player p[N_PLAYERS];

    player_names(PVP, p);
    setup_players(b_size, board, p);
    print_board(b_size, board);

    winner winner = NONE;
    int turn_count = 0;
    validation check = INVALID;

    while(winner == NONE){
        turn_count+=1;
        flush_input();

        while(check == INVALID){
            print_board(b_size, board);
            printf("\n\033[34mTURN %d\nIt's your turn %s\nMake your move!\n\033[0m", turn_count, p[turn_count%N_PLAYERS].name);
            input a = get_input();
            check = player_actions(b_size, board, p, a, &turn_count);
            flush_input();
            print_board(b_size, board);
        }
        
        check = INVALID;
        sleep(1);
        
    }
    //sistema de turnos
}

void realloc_history(player *p){
    p->x[1] = p->x[0];
    p->y[1] = p->y[0];
}

validation player_actions(int b_size, char board[b_size][b_size], player p[], input p_input, int *turn_count){
    
    int i = *turn_count%N_PLAYERS; //player index

    switch(p_input){
        case UP:
            if(board[(p[i].x[0])-1][(p[i].y[0])] == '='){
                return INVALID;
            }
            for(int j = 0; j < N_PLAYERS; j++){
                if(board[(p[i].x[0])-2][(p[i].y[0])] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].x[0] -= 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            break;

        case DOWN:
            if(board[(p[i].x[0])+1][(p[i].y[0])] == '='){
                return INVALID;
            }
            for(int j = 0; j < N_PLAYERS; j++){
                if(board[(p[i].x[0])+2][(p[i].y[0])] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].x[0] += 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            break;

        case LEFT:
            if(board[(p[i].x[0])][(p[i].y[0])-1] == 'I'){
                return INVALID;
            }
            for(int j = 0; j < N_PLAYERS; j++){
                if(board[(p[i].x[0])][(p[i].y[0])-2] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].y[0] -= 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            break;

        case RIGHT:
            if(board[(p[i].x[0])][(p[i].y[0])+1] == 'I'){
                return INVALID;
            }
            for(int j = 0; j < N_PLAYERS; j++){
                if(board[(p[i].x[0])][(p[i].y[0])+2] == p[j].icon) return INVALID;
            }
            board[p[i].x[0]][p[i].y[0]] = ' ';
            realloc_history(&p[i]);
            p[i].y[0] += 2;
            board[p[i].x[0]][p[i].y[0]] = p[i].icon;

            break;

        case BACK:
            int aux = (i-1+N_PLAYERS)%N_PLAYERS;
            if(p[aux].x[1] == -1){
                printf("\n\033[31mWARNING: Invalid action, you can't go back an action you never made\033[0m\n");
                sleep(2);
                return INVALID;
            }

            board[p[aux].x[0]][p[aux].y[0]] = ' ';
            p[aux].x[0] = p[aux].x[1];
            p[aux].y[0] = p[aux].y[1];

            board[p[aux].x[0]][p[aux].y[0]] = p[aux].icon;

            p[aux].x[1] = -1;
            p[aux].y[1] = -1;

            *turn_count-=2;
            return VALID;

        case SPACE:
            //funcoes das paredes
    }
    return VALID;
}

//to do: trocar matriz por uma gerada por malloc, corrigir o back porque ele só tá voltando a ultima jogada feita e não as ultimas 2 jogadas de cada jogador na ordem de turno