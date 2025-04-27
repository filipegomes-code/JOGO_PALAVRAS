// JOGOUI.C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Windows.h>
#include "../SharedMem/mensagens.h"

void Pipe_jogoUI(mensagem msg){

    HANDLE hPipeArbitro = CreateFileA("\\\\.\\pipe\\Pipe", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); // este pipe entra em ligaçao com o pipe do arbitro para saber qual é o tipo de mensagem

    if(hPipeArbitro == INVALID_HANDLE_VALUE){
        printf(" erro na ligacao com arbitro ");
        return;
    }

    DWORD byteswritten;
    WriteFile(hPipeArbitro, &msg , sizeof(msg), &byteswritten, NULL);


    CloseHandle(hPipeArbitro);
}


int main(int argc, char* argv[]){   

    mensagem msg;

    if(argc < 2 ){ 
        printf("uso incorreto de de %s <username>", argv[0]);
        return 1;
    }

    char* player = argv[1];
    printf("jogador %s entrou no jogo\n", player);

    strcpy(msg.tipo , TIPO_ENTRAR); // usei strcpy pq eu controlo o tamanho da msg.tipo
    strncpy(msg.username, player, sizeof(msg.username)); // uso strncpy aqui pq o tamanho do username pode passar o tamanho do buffer e pode dar overflow. 

    Pipe_jogoUI(msg);

    return 0;
}





