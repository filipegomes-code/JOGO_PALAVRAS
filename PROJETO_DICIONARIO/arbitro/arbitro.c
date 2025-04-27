// ARBITRO.C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include "../SharedMem/mensagens.h"

void warnsAll(char* tipo, char* username){
    if( strcmp(tipo , TIPO_ENTRAR) == 0){
        printf("jogador %s entrou no Jogo\n", username);
    }

    if( strcmp(tipo , TIPO_SAIR) == 0){
        printf("Jogador %s saiu do Jogo\n", username);
    }
}

//int remove(){
//    return 0;
//}

//int addPlayer(){
//    return 0;
//}
            
void Comandos(mensagem msg){
    if(strcmp(msg.tipo,TIPO_ENTRAR) == 0){
        //addPlayer();
        //response();
        warnsAll(TIPO_ENTRAR, msg.username);
    }

    if(strcmp(msg.tipo, TIPO_SAIR) == 0){
        //removePlayer();
        //response();
        warnsAll(TIPO_SAIR, msg.username);
    }
}

DWORD WINAPI Thread_player(LPVOID lpParam){
    HANDLE hpipe = (HANDLE)lpParam; //fazemos um cast para HANDLE

    mensagem msg;
    DWORD bytes_read;
    
    BOOL success = ReadFile(hpipe, &msg, sizeof(msg), &bytes_read, NULL);

    if(success){
        Comandos(msg);
    }

    CloseHandle(hpipe);
    return 0;
}

int main(){
    //mensagem msg;
    HANDLE hpipe;

    printf("[AGUARDANDO PLAYERS.....]");

    while(1){
        hpipe = CreateNamedPipeA(("\\\\.\\pipe\\Pipe"), PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);

        if(hpipe == INVALID_HANDLE_VALUE){
            printf("erro a criar pipe");
            return 1;
        }

        BOOL connected = ConnectNamedPipe(hpipe, NULL);

        if(connected){

            HANDLE thread = CreateThread(NULL, 0, Thread_player, (LPVOID)hpipe /*cast para LPVOID */, 0, NULL);

            if(thread == NULL){
                printf("erro na criacao da thread");
                CloseHandle(hpipe);
            }else
                CloseHandle(thread);
        }else
            CloseHandle(hpipe);
    }

    return 0;
}