// ARBITRO.C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include "../SharedMem/mensagens.h"

#define MAX_PLAYER 2

HANDLE hmutex;
int cont_players = 0;

void warnsAll(char* tipo, char* username){
    if( strcmp(tipo , TIPO_ENTRAR) == 0){
        printf("jogador %s entrou no Jogo\n", username);
    }

    if( strcmp(tipo , TIPO_SAIR) == 0){
        printf("Jogador %s saiu do Jogo\n", username);
    }
}

int addPlayer(){
    
    WaitForSingleObject(hmutex, INFINITE);
    cont_players++;
    ReleaseMutex(hmutex);

    return cont_players;
}

int removePlayer(){
    
    WaitForSingleObject(hmutex, INFINITE);
    cont_players--;
    ReleaseMutex(hmutex);

    return cont_players;
}

            
void Comandos(mensagem msg){
    if(strcmp(msg.tipo,TIPO_ENTRAR) == 0){
        //response();
        warnsAll(TIPO_ENTRAR, msg.username);
    }

    if(strcmp(msg.tipo, TIPO_SAIR) == 0){
        removePlayer();
        //response();
        warnsAll(TIPO_SAIR, msg.username);
    }
}

DWORD WINAPI Thread_player(LPVOID lpParam){
    infoThread* info = (infoThread*)lpParam; //fazemos um cast para HANDLE
    HANDLE hpipe = info->hpipe;
    mensagem msg = info->msg_thread;
    
    Comandos(msg);

    free(info);
    CloseHandle(hpipe);
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

            WaitForSingleObject(hmutex,INFINITE);

            if(cont_players >= MAX_PLAYER){
                printf("quantidade maxima de players atingida");
                
                DWORD byteswrite;
                WriteFile(hpipe, TIPO_LIM_PLAYERS, strlen(TIPO_LIM_PLAYERS)+1, &byteswrite, NULL);
                
                DisconnectNamedPipe(hpipe); // se limite for atingido, "guarda" o pipe criado e é usado no proximo jogador que entrar, em vez, de se criar um novo pipe . n era preciso fazer , mas é interessante.
                CloseHandle(hpipe);
                continue;
            }
            ReleaseMutex(hmutex);

            DWORD bytesWritten;
            WriteFile(hpipe, "aceite" , strlen("aceite") + 1, &bytesWritten, NULL);

            addPlayer();
            
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