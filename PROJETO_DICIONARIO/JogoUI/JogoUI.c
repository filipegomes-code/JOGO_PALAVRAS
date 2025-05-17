// JOGOUI.C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Windows.h>
#include "../SharedMem/mensagens.h"

char* player;
volatile int terminar = 0;

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

    char resposta_recebida[600]; //ReadFile precisa de memória editável, não pode ser constante (como TIPO_LIM_PLAYERS) que estava a tentar usar
    DWORD bytesread;
    BOOL success = ReadFile(hPipeArbitro, resposta_recebida, sizeof(resposta_recebida), &bytesread, NULL);
    
    if (success && strcmp(resposta_recebida, TIPO_LIM_PLAYERS) == 0){
        printf("N consegue entrar\n");
        CloseHandle(hPipeArbitro);
        return 1;
    }

    if(success && strcmp(msg.tipo, TIPO_LISTA)== 0){
        printf("%s\n", resposta_recebida);
    }

    if(success && strcmp(msg.tipo, TIPO_PONT)== 0){
        printf("[TABELA PONTUACAO]: ");
        printf("%s\n", resposta_recebida);
    }

    CloseHandle(hPipeArbitro);
    return 0;
}

DWORD WINAPI Thread_JogoUI(LPVOID lpParam){
    char pipename[50];
    snprintf(pipename, sizeof(pipename), "\\\\.\\pipe\\Pipe_Comando_%s", player);

    HANDLE hPipe;
    while (!terminar) {
        hPipe = CreateNamedPipeA(pipename, PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            printf("Erro ao criar pipe de comando\n");
            return 1;
        }

        BOOL connected = ConnectNamedPipe(hPipe, NULL);
        if (!connected) {
            CloseHandle(hPipe);
            continue;
        } 

        char comando[100];
        DWORD read;
        BOOL success;

        while ( !terminar && (success = ReadFile(hPipe, comando, sizeof(comando), &read, NULL)) && read > 0) {
            comando[read] = '\0';

            if (strcmp(comando, TIPO_EXCLUIR) == 0) {
                printf("Jogador excluido pelo arbitro.\n");
                terminar = 1;
                CloseHandle(hPipe);
                exit(0);
            } else if (strcmp(comando, TIPO_ENCERRAR) == 0) {
                printf("Jogo encerrado pelo arbitro.\n");
                terminar = 1;
                CloseHandle(hPipe);
                exit(0);
            } else {
                printf("\r\033[K%s\n", comando);  // imprime a linha de letras e limpa o input antigo
                printf(">");                      // repõe o prompt , por n ter esta linha de codigo, deu ganda problema no crl do terminal que n voltava para o prompt
                fflush(stdout);
            }
        }

    }
    CloseHandle(hPipe);
    return 0;
}

int main(int argc, char* argv[]){   

    mensagem msg;

    if(argc < 2 ){ 
        printf("uso incorreto de %s <username>", argv[0]);
        return 1;
    }

    player = argv[1];
    printf("jogador %s entrou no jogo\n", player);

    strcpy(msg.tipo , TIPO_ENTRAR); // usei strcpy pq eu controlo o tamanho da msg.tipo
    strncpy(msg.username, player, sizeof(msg.username)); // uso strncpy aqui pq o tamanho do username pode passar o tamanho do buffer e pode dar overflow. 

    if(Pipe_jogoUI(msg) == 1){
        return 1;
    }

    Info_comandos();

    HANDLE hThreadComando = CreateThread(NULL, 0, Thread_JogoUI, NULL, 0, NULL);
    if(hThreadComando == NULL){
        printf("Erro a criar thread\n");
        return 1;
    }

    while(!terminar){
        char input[30];

        putchar('>');
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        if(input[0] == ':'){
            if(strcmp(input, TIPO_SAIR) == 0){
                printf("Saiste do Jogo\n");
                terminar =1 ;
                strcpy(msg.tipo, TIPO_SAIR);
                strncpy(msg.username, player, sizeof(msg.username));
                Pipe_jogoUI(msg);
                exit(0);
            }else if(strcmp(input, TIPO_LISTA)== 0){
                printf("digitou o comando para ver a lista de jogadores\n");
                strcpy(msg.tipo, TIPO_LISTA);
                strncpy(msg.username, player, sizeof(msg.username));
                Pipe_jogoUI(msg);
                continue;
            }else if(strcmp(input, TIPO_PONT)== 0){
                printf("comando para ver a pontuacao\n");
                strcpy(msg.tipo, TIPO_PONT);
                strncpy(msg.username, player, sizeof(msg.username));
                Pipe_jogoUI(msg);
                continue;
            }
        }else{
            strcpy(msg.tipo, "palavra");
            strncpy(msg.username, player, sizeof(msg.username));
            strncpy(msg.palavra , input, sizeof(msg.palavra));
            Pipe_jogoUI(msg);
        }

    } 
    WaitForSingleObject(hThreadComando, INFINITE);
    CloseHandle(hThreadComando);
    return 0;
}





