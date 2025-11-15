#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_SIZE 9
#define WALL_SIZE 2

void construct_board(int b_size, char board[b_size][b_size]);
//void print_board(int b_size, char board[b_size][b_size]);
int select_gamemode(int b_size, char board[b_size][b_size]);
//8x8 map, matrix has to be 9*2 x 9*2. 18x18, so the walls can be placed inside the matrix
int main(){
    int b_size = BASE_SIZE*2+1; //control variable
    char board[b_size][b_size]; //creates the board

    construct_board(b_size, board); // calls the function to prepare the board for play
    print_board(b_size, board);
    typedef enum {load = 0, pvp = 1, pve = 2} option;
    option gamemode = select_gamemode(b_size, board);
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

int select_gamemode(int b_size, char board[b_size][b_size]){
    print_board(b_size, board);
    printf("\n\nWelcome to Quoridor made by Enzo and Joao Cleber\nPlease select an option\n\nLOAD SAVE         PLAYER VS PLAYER         PLAYER VS MACHINE\n");
    //scanf("")to do: make a system to control what option to choose via arrows and confirm with enter
    return 0;
}