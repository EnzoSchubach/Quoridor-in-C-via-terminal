#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>


//enums
typedef enum {LOAD = 0, TWOP = 1, THREEP = 2, FOURP = 3, QUIT = 4} option;
typedef enum {UP = 10, DOWN = 11, LEFT = 12, RIGHT = 13, ENTER = 14, BACK = 15, SPACE = 16, SLASH = 17} input;
typedef enum {P1 = 20, P2 = 21, NONE = 22} winner;
typedef enum {INVALID = 30, VALID = 31, CANCEL = 32, REPEAT = 33, AGAIN = 34} validation;
typedef enum {TOP = 40, BOT = 41, WEST = 42, EAST = 43} spawn;
typedef enum {INSERT = 50, DELETE = 51} instruction;
typedef enum {VERTICAL = 60, HORIZONTAL = 61} orientation;
typedef enum {MOVE = 70, WALL = 71} action;

//macros
#define BASE_SIZE 9
#define WALL_SIZE 2
#define N_OPTIONS 5
#define NAME_SIZE 10
#define WALL_CAP 5
#define BREAK_CAP 3

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
    wall walls[WALL_CAP];
    int wc;
    int win_pos;
    int wall_breaks;
    action last_action;
} player;


//chamadas de função (pra não ter que resolver conflito de ordem de funções)
void construct_board(int b_size, char **board);
void print_board(int b_size, char **board);
int select_gamemode(int b_size, char **board);
void player_names(player p[], int n_players);
void setup_players(int b_size, char **board, player p[], int n_players);
void pvp_mode(int b_size, char **board, int n_players, int start_turn_count, player p[]);
validation player_actions(int b_size, char **board, player p[], input p_input, int *turn_count, int n_players);
validation wall_actions(int b_size, char **board, int *x, int *y, input in, orientation *state, char **overlay, player *p, int n_players);
void place_wall(int b_size, char **board, int *x, int *y, instruction mode, orientation *state, bool temporary);
void update_overlay(int b_size, char **board, char **overlay);
input get_input();
void save_game(int b_size, player p[], int n_players, int turn_count); 
bool load_game(int *b_size, char ***board_new_ptr, player p_loaded[], int *n_players, int *turn_count);


//funções de ativar e desativar o raw mode (não mostrar o que está sendo digitado e pegar input sem apertar enter)
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

//limpar buffer
void flush_input(){
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char c;
    while (read(STDIN_FILENO, &c, 1) > 0);

    fcntl(STDIN_FILENO, F_SETFL, flags);
}

//criar tabuleiro com alocação dinamica
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

//liberar a memoria da heap
void free_board(int b_size, char **board) {
    if (board == NULL) return; //checar se o ponteiro tá NULL (já foi free)
    
    for (int i = 0; i < b_size; i++) {
        free(board[i]);
    }
    free(board);
}

//main, unindo as coisas
int main(){
    //ativando raw mode 
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    enable_raw_mode();
    //criando tamanho base do mapa baseado em 
    int b_size = BASE_SIZE*2+1; 
    
    //contagem de turno e de players base (pra mandar pro load e save)
    int n_players = 0;
    int turn_count = 0;
    player p[4]; //inicializando o array de players (máximo OBRIGATORIO de até 4 players)
    
    char **board = create_board(b_size); // cria o tabuleiro usando MALLOC e guarda em board
    if (board == NULL) { //if para caso não tenha espaço na HEAP
        printf("O espaço da sua Heap está cheio, não foi possível inicializar\n");
        return 1;
    }

    construct_board(b_size, board); //cria as paredes e a estrutura do mapa
    
    option gamemode = select_gamemode(b_size, board); // chama 
    
    // A variável 'current_board' agora rastreia o tabuleiro ativo (inicial ou carregado)
    char **current_board = board; 
    
    switch(gamemode){
        case LOAD: 
            //LOAD libera o tabuleiro antigo e inicializa o novo se tiver arquivo
            if (load_game(&b_size, &current_board, p, &n_players, &turn_count)) {
                free_board(BASE_SIZE*2+1, board); 
                pvp_mode(b_size, current_board, n_players, turn_count, p);//chama o modo de gameplay com o novo tabuleira
            } else {
                //caso não tenha save
                free_board(b_size, board);
                return 0;
            }
            break;
            
        case TWOP: 
            //lógica pra 2 jogadores
            turn_count = 0;
            n_players = 2;
            pvp_mode(b_size, current_board, n_players, turn_count, p);
            break;

        case THREEP: 
            //lógica pra 3 jogadores
            turn_count = 0;
            n_players = 3;
            pvp_mode(b_size, current_board, n_players, turn_count, p);
            break;

        case FOURP: 
            //lógica pra 4 jogadores
            turn_count = 0;
            n_players = 4;
            pvp_mode(b_size, current_board, n_players, turn_count, p);
            break;

        case QUIT: 
            //sair do jogo no menu
            print_board(b_size, current_board);
            printf("\nThanks for playing!\n\n"); 
            free_board(b_size, current_board);
            return 0; 
    }
    //lógica p liberar o tabuleiro no fim, usa current_board pq é o ponteiro final. Se o load deu bom, ele aponta para o tabuleiro mais novo.
    if (current_board != NULL && current_board != board) {
        //se o jogo foi carregado, o 'board' original já foi liberado, e liberamos o 'current_board'.
        free_board(b_size, current_board);
    } else if (current_board != NULL) {
        // Se for jogo novo ou load falhou, liberamos o 'board' original (que é current_board).
        free_board(b_size, current_board);
    }
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

void save_game(int b_size, player p[], int n_players, int turn_count) {
    FILE *file = fopen("quoridor_save.bin", "wb");
    if (file == NULL) {
        printf("\n\033[31mErro ao abrir arquivo para salvar.\033[0m\n");
        return;
    }

    // 1. Salva metadados
    fwrite(&b_size, sizeof(int), 1, file);
    fwrite(&n_players, sizeof(int), 1, file);
    fwrite(&turn_count, sizeof(int), 1, file);

    // 2. Salva o array de jogadores
    for (int i = 0; i < n_players; i++) {
        // Salva a estrutura player. Note que 'walls' e 'name' são arrays fixos.
        if (fwrite(&p[i], sizeof(player), 1, file) != 1) {
             printf("\n\033[31mErro ao salvar dados do jogador %d.\033[0m\n", i + 1);
             fclose(file);
             return;
        }
    }    
    fclose(file);
    printf("\n\033[32mJogo salvo com sucesso em 'quoridor_save.bin'.\033[0m\n");
}

bool load_game(int *b_size, char ***board_new_ptr, player p_loaded[], int *n_players, int *turn_count) {
    FILE *file = fopen("quoridor_save.bin", "rb");
    if (file == NULL) {
        printf("\n\033[31mErro: Arquivo 'quoridor_save.bin' não encontrado.\033[0m\n");
        return false;
    }

    // 1. Lê metadados
    if (fread(b_size, sizeof(int), 1, file) != 1 ||
        fread(n_players, sizeof(int), 1, file) != 1 ||
        fread(turn_count, sizeof(int), 1, file) != 1) 
    {
        printf("\n\033[31mErro ao ler cabeçalho do arquivo de salvamento.\033[0m\n");
        fclose(file);
        return false;
    }

    // 2. Verifica se o número de jogadores é válido
    if (*n_players > 4 || *n_players < 2) {
        printf("\n\033[31mErro: Número de jogadores (%d) inválido no arquivo de salvamento.\033[0m\n", *n_players);
        fclose(file);
        return false;
    }

    // 3. Lê o array de jogadores (diretamente para o array estático p_loaded[])
    if (fread(p_loaded, sizeof(player), *n_players, file) != *n_players) {
        printf("\n\033[31mErro ao ler dados dos jogadores.\033[0m\n");
        fclose(file);
        return false;
    }

    fclose(file);
    
    // 4. Recria e reconstrói o tabuleiro (aloca char **board)
    char **board_loaded = create_board(*b_size);
    if (board_loaded == NULL) {
        printf("\n\033[31mErro ao criar tabuleiro após carregamento.\033[0m\n");
        return false;
    }
    construct_board(*b_size, board_loaded);

    // Posiciona jogadores
    for (int i = 0; i < *n_players; i++) {
        board_loaded[p_loaded[i].x[0]][p_loaded[i].y[0]] = p_loaded[i].icon;
    }

    // Posiciona paredes
    for (int i = 0; i < *n_players; i++) {
        for (int j = 0; j < p_loaded[i].wc; j++) {
            int wx = p_loaded[i].walls[j].wall_x;
            int wy = p_loaded[i].walls[j].wall_y;
            orientation state = p_loaded[i].walls[j].state;
            
            place_wall(*b_size, board_loaded, &wx, &wy, INSERT, &state, false);
        }
    }

    // Passa o ponteiro para o novo tabuleiro alocado para a main
    *board_new_ptr = board_loaded;

    printf("\n\033[32mJogo carregado com sucesso!\033[0m\n");
    return true;
}



int select_gamemode(int b_size, char **board){ 
    int aux = -1, i = 0;
    char arr[] = {'<', ' ', ' ', ' ', ' '};

    while(aux != ENTER){

        print_board(b_size, board);
        printf("\n\n\033[34mWelcome to Quoridor made by Enzo and Joao Cleber\nPlease select an option\033[0m\n\n");
        printf("\033[34mLOAD SAVE\033[0m %c         \033[34m2P\033[0m %c         \033[34m3P\033[0m %c         \033[34m4P\033[0m %c         \033[34mLEAVE GAME\033[0m %c\n", arr[0], arr[1], arr[2], arr[3], arr[4]);
        
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

void pvp_mode(int b_size, char **board, int n_players, int start_turn_count, player p[]){
    
    // 🛑 CORREÇÃO: Removido o 'player p[n_players];'. 
    // Usamos o array 'p' que já foi passado no argumento.

    // 1. Configuração de Jogo Novo (só se for o primeiro turno)
    if (start_turn_count == 0) {
        player_names(p, n_players);
        setup_players(b_size, board, p, n_players);
    } 
    // Se start_turn_count > 0, os dados vieram do LOAD e estão em 'p'.

    print_board(b_size, board);

    winner game_winner = NONE;
    int turn_count = start_turn_count; // Inicia com o turno salvo (ou 0)
    validation check = INVALID;

    while(game_winner == NONE || check != CANCEL){
        // Aumenta o turno
        check = INVALID;
        turn_count+=1;
        
        // Ajuste do índice do jogador atual
        int current_player_index = (turn_count) % n_players; 
        
        flush_input();

        while(check == INVALID){
            print_board(b_size, board);
            printf("\n\033[34mTURNO %d\nÉ sua vez %s\nFaça sua jogada!\n%d/%d Paredes restando\n%d/%d Quebras de parede\n\033[0m", 
                   turn_count, p[current_player_index].name, p[current_player_index].wc, WALL_CAP,
                    p[current_player_index].wall_breaks, BREAK_CAP);
            printf("\n\n\033[34mSETAS = ANDAR\nESPAÇO = COLOCAR PAREDE\nBARRA = QUEBRAR PAREDE\nENTER = SALVAR E SAIR\n\033[0m");
            input a = get_input();
            
            check = player_actions(b_size, board, p, a, &turn_count, n_players);
            if (check == AGAIN) turn_count -=1;
            flush_input();
            
            //verifica se a ação foi salvar e sair do jogo
            if (check == CANCEL) {
                game_winner = NONE; 
                return; //sai da funcao pvp_mode
            }
        }
        
        if (check == VALID) {
            game_winner = check_win(p, current_player_index);
        }
    }
    
    printf("\n\033[1;32m*** END OF GAME ***\033[0m\n");
    if (game_winner != NONE) {
        printf("\033[1;32mThe winner is: %s!\033[0m\n", p[game_winner].name);
    }
}

void realloc_history(player *p){
    p->x[1] = p->x[0];
    p->y[1] = p->y[0];
}

// bool locked_in

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
                    in, &direction, overlay, &p[i], n_players
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
            
        case SLASH: {
            if(p[i].wall_breaks <= 0) {
                return INVALID;
            }

            wall all_placed_walls[WALL_CAP * n_players];
            int wall_count = 0;

            for (int k = 0; k < n_players; k++) {
                for (int l = 0; l < p[k].wc; l++) {
                    all_placed_walls[wall_count++] = p[k].walls[l];
                }
            }

            if (wall_count == 0) {
                printf("\n\033[31mNão há paredes no mapa!\n\n\033[0m");
                get_input();
                return INVALID;
            }

            input selection = SPACE;
            int current_selection = 0; 
            
            char **overlay = create_board(b_size);
            if (overlay == NULL) return INVALID;
            
            update_overlay(b_size, board, overlay);

            while(selection != ENTER && selection != CANCEL){ 
                update_overlay(b_size, board, overlay);
                
                wall *selected_wall = &all_placed_walls[current_selection];
                
                place_wall(b_size, overlay, 
                           &selected_wall->wall_x,
                           &selected_wall->wall_y, 
                           INSERT,
                           (orientation *)&selected_wall->state, true); 

                print_board(b_size, overlay);
                printf("\n\033[34mBreaking Wall: Use right arrow to cycle (%d/%d walls)\n\n\033[0m", current_selection + 1, wall_count);


                selection = get_input();

                switch(selection){
                    case UP:
                    case LEFT:
                        current_selection = (current_selection - 1 + wall_count) % wall_count;
                        break;
                    case DOWN:
                    case RIGHT:
                        current_selection = (current_selection + 1) % wall_count;
                        break;
                    case BACK:
                        selection = CANCEL;
                        break;
                    case ENTER:
                        break;
                }
            }
            
            free_board(b_size, overlay);

            if (selection == ENTER) {
                wall *to_break = &all_placed_walls[current_selection];
                
                place_wall(b_size, board, 
                           &to_break->wall_x, 
                           &to_break->wall_y, 
                           DELETE, 
                           (orientation *)&to_break->state, 
                           false); 

                bool found = false;
                for (int k = 0; k < n_players; k++) {
                    for (int l = 0; l < p[k].wc; l++) {
                        
                        if (p[k].walls[l].wall_x == to_break->wall_x && p[k].walls[l].wall_y == to_break->wall_y) 
                        {
                            
                            for (int m = l; m < p[k].wc - 1; m++) {
                                p[k].walls[m] = p[k].walls[m + 1];
                            }

                            p[k].wc--;
                            found = true;
                            break; 
                        }
                    }
                    if (found) break; // Sai do loop 'k'
                }
                
                // 3. Decrementa o contador de quebras do jogador atual
                p[i].wall_breaks--;
                p[i].last_action = WALL; 
                *turn_count -= 1;
                validation player_actions(int b_size, char **board, player p[], input p_input, int *turn_count, int n_players);
                return REPEAT;
            } 
            
            return INVALID;
        }

        case ENTER:
            save_game(b_size, p, n_players, *turn_count);
            
            return CANCEL;

        default:
            return INVALID;
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



validation wall_actions(int b_size, char **board, int *x, int *y, input in, orientation *state, char **overlay, player *p, int n_players){
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
            return INVALID;
    }
    return INVALID;
}