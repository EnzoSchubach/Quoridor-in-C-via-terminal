#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

//structs
struct termios orig_termios;

//enums
typedef enum {LOAD = 0, PVP = 1, PVE = 2} option;
typedef enum {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, ENTER = 4, BACK = 5} input;

//defines
#define BASE_SIZE 9
#define WALL_SIZE 2

//function pre-calls (just so we doesnt need to organize every function in order)
void construct_board(int b_size, char board[b_size][b_size]);
void print_board(int b_size, char board[b_size][b_size]);
int select_gamemode(int b_size, char board[b_size][b_size]);

void disable_raw_mode() { //function to deactive the raw mode
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios); 
}

void enable_raw_mode(){ //function to turn on raw mode (deactivates enter for the input, and the echo)
    struct termios raw;

    //gets current configuration
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;

    raw.c_lflag &= ~ICANON; //turns off canon mode (turns off enter confirmation for the input)

    raw.c_lflag &= ~ECHO; //turns off the echo (turns off the terminal showing off what you sent as a input)

    //byte reading
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    //applies the configuration
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    //when the code ends, calls the function to deactivates raw mode
    atexit(disable_raw_mode);
}

//9x9 map, matrix has to be 9*2 x 9*2. 18x18, so the walls can be placed inside the matrix
int main(){
    int b_size = BASE_SIZE*2+1; //control variable
    char board[b_size][b_size]; //creates the board

    construct_board(b_size, board); // calls the function to prepare the board for play
    print_board(b_size, board);
    
    option gamemode = select_gamemode(b_size, board);
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
                case 'C': return LEFT; break;
                case 'D': return RIGHT; break;
            }
        }
    }
    else{
        switch (c){
            case '\n': return ENTER; break;
            case '\b': return BACK; break;
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
                board[i][j] = '-';
            else if (j%2 == 0)
                board[i][j] = '|';
            else
                board[i][j] = ' ';
        }
    }
}

void print_board(int b_size, char board[b_size][b_size]){
    system("clear");
    for (int i = 0; i < b_size; i++){
        for (int j = 0; j < b_size; j++){
            if (j%2 == 0)
                printf(" %c ", board[i][j]);
            else
                printf("%c", board[i][j]);
        }
        printf("\n");
    }
}

int select_gamemode(int b_size, char board[b_size][b_size]){ //UNFINISHED---------------------------------------------------------------------------
    int aux = -1, i = 0;

    print_board(b_size, board);
    printf("\n\nWelcome to Quoridor made by Enzo and Joao Cleber\nPlease select an option\n\nLOAD SAVE         PLAYER VS PLAYER         PLAYER VS MACHINE\n");
    
    aux = get_input();
    while(aux != (LEFT || RIGHT || ENTER)){
        aux = get_input(); //gets character until its a valid one for the menu
    }

    switch(aux){
        case LEFT: i-=1; break;
        case RIGHT: i+=1; break;
        case ENTER: //needs to finish
    }
    
    

    

    return 0;
}

