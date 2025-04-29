// JOGOUI.C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Windows.h>
#include "../SharedMem/mensagens.h"


void Info_comandos(){

    printf("COMANDOS:\n");
    printf(":sair - Permite Jogador sair do jogo\n");
    printf(":pont - Permite ver pontuacao de outros jogadores\n");
    printf(":jogs - Permite obter a lista de Jogadores\n\n");

}

int Pipe_jogoUI(mensagem msg){

    HANDLE hPipeArbitro = CreateFileA("\\\\.\\pipe\\Pipe", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); // este pipe entra em ligaçao com o pipe do arbitro para saber qual é o tipo de mensagem

    if(hPipeArbitro == INVALID_HANDLE_VALUE){
        printf(" erro na ligacao com arbitro ");
        return 1;
    }
    
    DWORD byteswritten;
    WriteFile(hPipeArbitro, &msg , sizeof(msg), &byteswritten, NULL);


    char resposta_recusado[10]; //ReadFile precisa de memória editável, não pode ser constante (como TIPO_LIM_PLAYERS) que estava a tentar usar
    DWORD bytesread;
    BOOL success = ReadFile(hPipeArbitro, resposta_recusado, sizeof(resposta_recusado), &bytesread, NULL);

    if(success && strcmp(resposta_recusado, TIPO_LIM_PLAYERS) == 0){
        printf("Numero máximo de jogadores atingidos, n consegue entrar\n");
        CloseHandle(hPipeArbitro);
        return 1;
    }

    CloseHandle(hPipeArbitro);
    return 0;
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

    if(Pipe_jogoUI(msg) == 1){
        return 1;
    }

    Info_comandos();

    while(1){
        char input[30];

        putchar('>');
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        if(strcmp(input, TIPO_SAIR) == 0){
            printf("Saiste do Jogo\n");
            strcpy(msg.tipo, TIPO_SAIR);
            strncpy(msg.username, player, sizeof(msg.username));
            Pipe_jogoUI(msg);
            break;
        }

    } 

    return 0;
}





