// ARBITRO.C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <time.h>
#include "../SharedMem/mensagens.h"

#define MAX_PLAYER 2
#define TAM_NOME 20
#define RITMO 1
#define MAXLETRAS 5

mensagem msg;
HANDLE hmutex;
HANDLE Pipe_player_threads[MAX_PLAYER]; //um handle de arrays para guardar as threads criadas para cada jogador

char vetorletras[MAXLETRAS]; // array para cada letra
int letrascont = 0; // numeros de letras em uso
int comecou = 0; //flag 

char PLAYER_NAMES[MAX_PLAYER][TAM_NOME];
int cont_players = 0; 
int pontuacao[MAX_PLAYER] = {0}; // inicializa todas as pontuaçoes a zero

// suposto enviar a todos os JogoUIs oq esta a acontecer
void warnsAll(char* tipo, char* username){
    if( strcmp(tipo , TIPO_ENTRAR) == 0){
        printf("jogador %s entrou no Jogo\n", username);
    }

    if( strcmp(tipo , TIPO_SAIR) == 0){
        printf("Jogador %s saiu do Jogo\n", username);
    }
}

int name_repeated(const char* nome){

    for(int i = 0; i < cont_players; i++){
        if( strcmp(PLAYER_NAMES[i] , nome) == 0)
            return 1;
    }
    return 0;
}

int addPlayer(const char* nome, HANDLE hpipe){
    
    WaitForSingleObject(hmutex, INFINITE);
    strcpy(PLAYER_NAMES[cont_players], nome);
    Pipe_player_threads[cont_players] = hpipe;
    pontuacao[cont_players] = 0;
    cont_players++;
    // if temporario para testar as letras
    if(cont_players == 1 && comecou == 0){
        comecou = 1;
        HANDLE hthreadletras = CreateThread(NULL, 0, Thread_letras, NULL, 0, NULL);
        if(hthreadletras !=NULL){
            CloseHandle(hthreadletras);
        }
    }
    ReleaseMutex(hmutex);



    return 0;
}

void removePlayer(const char* nome){
    WaitForSingleObject(hmutex, INFINITE);
    for(int i = 0; i < cont_players ; i++){
        if(strcmp(PLAYER_NAMES[i], nome) == 0){
            char pipename[50];
            snprintf(pipename, sizeof(pipename), "\\\\.\\pipe\\Pipe_Comando_%s", PLAYER_NAMES[i]);

            HANDLE hPipeComando = CreateFileA(pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if(hPipeComando != INVALID_HANDLE_VALUE){
                DWORD byteswrite;
                WriteFile(hPipeComando, TIPO_EXCLUIR, strlen(TIPO_EXCLUIR)+1, &byteswrite, NULL);
                CloseHandle(hPipeComando);
            } else {
                printf("Erro ao conectar ao pipe do jogador %s para excluir.\n", PLAYER_NAMES[i]);
            }

            for(int j = i; j < cont_players-1 ; j++  ){
                strcpy(PLAYER_NAMES[j], PLAYER_NAMES[j + 1]);
                pontuacao[j] = pontuacao[j+1];
                Pipe_player_threads[j] = Pipe_player_threads[j+1];
            }
            cont_players--;
            break;
        }
    }
    ReleaseMutex(hmutex);
}

void player_rejeitado(HANDLE hpipe, const char* TIPO){

    printf("Jogador nao aceite");

    DWORD byteswrite;
    WriteFile(hpipe, TIPO_LIM_PLAYERS, strlen(TIPO_LIM_PLAYERS)+1, &byteswrite, NULL);
                
    DisconnectNamedPipe(hpipe); // se limite for atingido, "guarda" o pipe criado e é usado no proximo jogador que entrar, em vez, de se criar um novo pipe . n era preciso fazer , mas é interessante.
    CloseHandle(hpipe);
    
}

void Comandos(mensagem msg, HANDLE hpipe){
    if(strcmp(msg.tipo,TIPO_ENTRAR) == 0){
        //response();
        warnsAll(TIPO_ENTRAR, msg.username);
    }

    if(strcmp(msg.tipo, TIPO_SAIR) == 0){
        removePlayer(msg.username);
        //response();
        warnsAll(TIPO_SAIR, msg.username);
    }
}

DWORD WINAPI Thread_player(LPVOID lpParam){
    infoThread* info = (infoThread*)lpParam; //fazemos um cast para HANDLE
    HANDLE hpipe = info->hpipe;
    mensagem msg = info->msg_thread;
    
    Comandos(msg, hpipe);

    free(info);
    return 0;
}
// serve apenas para os comandos digitados pelo arbitro , precisa de ser Thread , senao entra em race conditions com outros threads
DWORD WINAPI Thread_arbitro_comandos(LPVOID lpParam){
    char input[100];
    while(1){
        printf("[COMANDOS]> ");
        fgets(input, sizeof(input) ,stdin);
        input[strcspn(input, "\n")] = 0;

        if(strcmp(input, "listar")== 0){
            WaitForSingleObject(hmutex, INFINITE);
            printf("[LISTA DE JOGADORES ATIVOS]:\n");
            for(int i = 0; i < cont_players; i++){
                printf("%s -> %d pontos\n", PLAYER_NAMES[i], pontuacao[i]);
            }
            ReleaseMutex(hmutex);
        }
        else if(strncmp(input, "excluir ", 8 ) == 0){
            char* nome = input + 8;
            int existe = 0;
            WaitForSingleObject(hmutex, INFINITE);
            for(int i = 0 ; i < cont_players; i++){
                if(strcmp(PLAYER_NAMES[i], nome) == 0){
                    existe = 1;
                    break;
                }
            }
            ReleaseMutex(hmutex);

            if(existe){
                removePlayer(nome);
                printf("Player %s foi expulso\n", nome);
            }else
                printf("jogador n existe\n");
        }
        else if(strcmp(input, "encerrar")== 0){
            WaitForSingleObject(hmutex,INFINITE);
            for(int i =0 ; i < cont_players; i++){
                char pipename[50];
                snprintf(pipename, sizeof(pipename), "\\\\.\\pipe\\Pipe_Comando_%s", PLAYER_NAMES[i]);
        
                HANDLE hPipeComando = CreateFileA(pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                if(hPipeComando != INVALID_HANDLE_VALUE){
                    DWORD byteswrite;
                    WriteFile(hPipeComando, TIPO_ENCERRAR, strlen(TIPO_ENCERRAR)+1, &byteswrite, NULL);
                    CloseHandle(hPipeComando);
                } else {
                    printf("Erro ao conectar ao pipe do jogador %s para encerrar.\n", PLAYER_NAMES[i]);
                }
        
                CloseHandle(Pipe_player_threads[i]); // Fecha o pipe original do árbitro
            }
            cont_players = 0;
            ReleaseMutex(hmutex);
            printf("Jogo encerrado pelo arbitro\n");
            exit(0);
        }
        else
            printf("comando desconhecido\n");
    }

    return 0;
}

int gerarletras(){
    for(int i =0; i < MAXLETRAS; i++) vetorletras[i] = '_';

    return 0;
}

char sortearletras(){ return 'a' + (rand() % 26); }

// permite sortear as letras sem entrar em race conditions 
DWORD WINAPI Thread_letras(LPVOID lpParam){

    srand((unsigned)time(NULL)); // srand espera sempre um num positivo

    int tempovida[MAXLETRAS] = {0};
    gerarletras(); // é preciso por isto aqui, senao o array nunca é inicializado com '_' e a verificaçao dentro de threads falha

    while(1){
        Sleep(RITMO * 1000); // passa de ms para s

        WaitForSingleObject(hmutex, INFINITE);

        int livre = -1;
        // insere letra na 1ª posiçao livre 
        for(int i = 0; i < MAXLETRAS; i++){
            if(vetorletras[i] == '_'){
                livre = i;
                break;
            }
        }

        if(livre != -1){
            vetorletras[livre] = sortearletras();
            tempovida[livre] = 1;
        }else{
            int posantiga = 0; // posicao mais antiga (mais tempo de vida), para substituir pela nova
            int maxtempo = tempovida[0];
            // checka se existe alguma letra mais antiga que a primeira
            for(int i = 1; i < MAXLETRAS; i++){
                if(tempovida[i] > maxtempo){
                    maxtempo = tempovida[i];
                    posantiga = i;
                }
            }

            vetorletras[posantiga] = sortearletras();
            tempovida[posantiga] = 1;
        }
        // incrementa segundos a cada elemento que é uma letra no vetor
        for(int i = 0; i < MAXLETRAS; i++)
            if(vetorletras[i] != '_') tempovida[i]++; 

        printf("linha das letras ");
        for(int i = 0; i < MAXLETRAS; i++){
            printf("%c ", vetorletras[i]);
        }
        putchar('\n');
        ReleaseMutex(hmutex);
    }

    return 0;
}

int main(){ 
    //mensagem msg;
    HANDLE hpipe;

    printf("[AGUARDANDO PLAYERS.....]");

    hmutex = CreateMutexA(NULL, FALSE, "Global\\arbitro");

    if(hmutex == NULL){
        printf("erro ao criar mutex");
        return 1;
    }

    HANDLE hArbitro = CreateThread(NULL, 0, Thread_arbitro_comandos, NULL, 0, NULL);

        if(hArbitro == NULL){
            printf("erro na criaçao do thread para os comandos do arbitro");
            return 1;
        }else
            CloseHandle(hArbitro);

    while(1){
        hpipe = CreateNamedPipeA(("\\\\.\\pipe\\Pipe"), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);

        if(hpipe == INVALID_HANDLE_VALUE){
            printf("erro a criar pipe");
            return 1;
        }

        BOOL connected = ConnectNamedPipe(hpipe, NULL);
        
        if(connected){

            mensagem msg;
            DWORD bytesread;
            BOOL success = ReadFile(hpipe, &msg, sizeof(msg), &bytesread, NULL);

            if(!success){
                printf("erro a ler file");
                CloseHandle(hpipe);
                continue;
            }
            
            if(strcmp(msg.tipo, TIPO_ENTRAR)== 0){
                WaitForSingleObject(hmutex,INFINITE);
    
                int cheio = cont_players >= MAX_PLAYER;
                int repetido = name_repeated(msg.username);
                ReleaseMutex(hmutex);
    
                if(cheio || repetido){
                    // posso usar um operador ternário para dar printf quando é recusado por nome repetido ou limite maximo, mas n pus ainda e ns se vou por
                    player_rejeitado(hpipe, TIPO_LIM_PLAYERS);
                    continue;
                }
                DWORD bytesWritten;
                WriteFile(hpipe, "aceite" , strlen("aceite") + 1, &bytesWritten, NULL);
                addPlayer(msg.username, hpipe);
            }
            else if(strcmp(msg.tipo, TIPO_LISTA) == 0){
                char lista[600] = "";

                WaitForSingleObject(hmutex, INFINITE);
                printf("[LISTA DE JOGADORES]:\n ");
                for(int i = 0; i < cont_players; i++){
                    strcat(lista, PLAYER_NAMES[i]);
                    strcat(lista, "\n");
                }
                ReleaseMutex(hmutex);

                DWORD byteswrite;
                WriteFile(hpipe, lista, strlen(lista)+1, &byteswrite, NULL);

                CloseHandle(hpipe);
                continue;
            }
            else if(strcmp(msg.tipo, TIPO_PONT)==0){
                char pontos[100] = "";
                
                WaitForSingleObject(hmutex, INFINITE);
                printf("[TABELA DE PONTUACAO]:\n ");
                for(int i = 0; i < cont_players; i++){
                    char linha[50];
                    snprintf(linha, sizeof(linha), "%s -> %d pontos\n", PLAYER_NAMES[i], pontuacao[i]);
                    strcat(pontos, linha);
                }
                ReleaseMutex(hmutex);
                DWORD byteswrite;
                WriteFile(hpipe, pontos, strlen(pontos)+1, &byteswrite, NULL);
                CloseHandle(hpipe);
                continue;
            }
            
            infoThread* info = malloc(sizeof(infoThread));
            info->hpipe = hpipe;
            info->msg_thread = msg;

            HANDLE thread = CreateThread(NULL, 0, Thread_player, (LPVOID)info /*cast para LPVOID */, 0, NULL);

            if(thread == NULL){
                printf("erro na criacao da thread");
                CloseHandle(hpipe);
                free(info);
            }else{
                CloseHandle(thread);
            }
        }else
            CloseHandle(hpipe);
    }



    return 0;
}