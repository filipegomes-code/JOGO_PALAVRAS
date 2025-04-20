#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Windows.h"
#include "../SharedMem/mensagens.h"

int main(int argc, char* argv[]){   

    mensagem msg;

    if(argc < 2 ){ 
        printf("uso incorreto de de %s <username>", argv[0]);
        return 1;
    }

    char* player = argv[1];
    printf("jogador %s entrou no jogo", player);

    strcpy(msg.tipo , TIPO_ENTRAR); // usei strcpy pq eu controlo o tamanho da msg.tipo
    strncpy(msg.username, player, sizeof(msg.username)); // uso strncpy aqui pq o tamanho do username pode passar o tamanho do buffer e pode dar overflow. 



    return 0;
}





