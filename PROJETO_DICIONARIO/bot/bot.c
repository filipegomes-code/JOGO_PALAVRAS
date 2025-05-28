// BOT.C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <Windows.h>
#include <time.h>
#include <ctype.h>
#include "../SharedMem/mensagens.h"

#define MAX_PALAVRA 50
#define MAX_PALAVRAS 47038
#define MAXLETRAS_ECRAN 5
#define MAX_LETRAS 13

char player[50];
HANDLE hmutex;
volatile int terminar = 0;

char letras_visiveis[MAXLETRAS_ECRAN+1];
char dicionario[MAX_PALAVRAS][MAX_LETRAS];

int total_palavras = 0;

HANDLE hMapFile;
char* letras_partilhadas;

bool usado[MAXLETRAS_ECRAN];
char tentativa[MAXLETRAS_ECRAN + 1];
char melhor_palavra[MAXLETRAS_ECRAN + 1] = "";


void carregar_dicionario(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir dicionario");
        exit(1);
    }

    char palavra[MAX_LETRAS];

    while (fgets(palavra, MAX_LETRAS, file)) {
        palavra[strcspn(palavra, "\r\n")] = '\0';  // remove \n ou \r\n

        if (strlen(palavra) <= MAXLETRAS_ECRAN) {
            strcpy(dicionario[total_palavras], palavra);
            total_palavras++;
        }

        if (total_palavras >= MAX_PALAVRAS) break;
    }
    fclose(file);
    printf("[BOT] Total de palavras carregadas: %d\n", total_palavras);
}

int letras_suficientes(const char* palavra) {
    char letras_temp[MAXLETRAS_ECRAN];
    memcpy(letras_temp, letras_visiveis, MAXLETRAS_ECRAN); // faz uma cópia das letras visíveis

    for (int i = 0; palavra[i]; i++) {
        char c = tolower(palavra[i]);
        int found = 0;

        // tenta encontrar a letra na cópia das letras visíveis
        for (int j = 0; j < MAXLETRAS_ECRAN; j++) {
            if (letras_temp[j] == c) {
                letras_temp[j] = '_'; // marca como usada
                found = 1;
                break;
            }
        }

        if (!found) return 0; // letra não encontrada nas disponíveis
    }

    return 1; // conseguiu formar a palavra com as letras disponíveis
}

bool palavra_existe_no_dicionario(const char* palavra) {
    for (int i = 0; i < total_palavras; i++) {
        if (strcmp(dicionario[i], palavra) == 0)
            return true;
    }
    return false;
}

void gerar_permutacoes_mais_longa(int pos, int max_len) {
    tentativa[pos] = '\0';

    if (pos > 0 && palavra_existe_no_dicionario(tentativa)) {
        if (strlen(tentativa) > strlen(melhor_palavra)) {
            strcpy(melhor_palavra, tentativa);
        }
    }

    if (pos == max_len) return;

    for (int i = 0; i < MAXLETRAS_ECRAN; i++) {
        if (!usado[i] && letras_visiveis[i] != '_') {
            usado[i] = true;
            tentativa[pos] = letras_visiveis[i];
            gerar_permutacoes_mais_longa(pos + 1, max_len);
            usado[i] = false;
        }
    }
}

const char* escolher_palavra() {
    melhor_palavra[0] = '\0';
    memset(usado, 0, sizeof(usado));

    for (int len = 1; len <= MAXLETRAS_ECRAN; len++) {
        gerar_permutacoes_mais_longa(0, len);
    }

    if (strlen(melhor_palavra) > 0) {
        return melhor_palavra;
    }

    return NULL;
}

int Pipe_bot(mensagem msg) {
    HANDLE hPipe = CreateFileA("\\\\.\\pipe\\Pipe", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("Erro ao ligar ao arbitro\n");
        return 1;
    }

    DWORD written;
    WriteFile(hPipe, &msg, sizeof(msg), &written, NULL);
    CloseHandle(hPipe);

    return 0;
}

DWORD WINAPI Thread_Comando(LPVOID lpParam) {
    char pipename[50];
    snprintf(pipename, sizeof(pipename), "\\\\.\\pipe\\Pipe_Comando_%s", player);

    HANDLE hPipe;
    while (!terminar) {
        hPipe = CreateNamedPipeA(pipename, PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);
        if (hPipe == INVALID_HANDLE_VALUE) return 1;

        BOOL connected = ConnectNamedPipe(hPipe, NULL);
        if (!connected) {
            CloseHandle(hPipe);
            continue;
        }

        char comando[100];
        DWORD read;
        BOOL success;

        while ((success = ReadFile(hPipe, comando, sizeof(comando), &read, NULL)) && read > 0) {
            comando[read] = '\0';

            if (strcmp(comando, TIPO_ENCERRAR) == 0 || strcmp(comando, TIPO_EXCLUIR) == 0) {
                printf("[BOT] Terminando por comando do arbitro.\n");
                terminar = 1;
                CloseHandle(hPipe);
                exit(0);
            }
        }

        CloseHandle(hPipe);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    srand((unsigned)time(NULL));
    if(argc !=2){
        printf("uso incorreto do bot");
        return 1;
    }

    strncpy(player, argv[1], sizeof(player));

    mensagem msg;
    carregar_dicionario("bot/dicionario_pt_eng.txt"); // ver se posso mudar oq está antes do dicionario

    hmutex = CreateMutexA(NULL, FALSE, "Global\\arbitro");

    // conecta à mem partilhada (mapa) , foi criado o mapa no arbitro.
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, "Global\\LetrasPartilhadas");
    if (hMapFile == NULL) {
        printf("BOT: erro ao abrir memória partilhada\n");
        return 1;
    }
    // funciona como uma lanterna a apontar para um quadro escuro escrito (revela oq esta no quadro).
    letras_partilhadas = (char*) MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, MAXLETRAS_ECRAN);
    if (letras_partilhadas == NULL) {
        printf("BOT: erro ao mapear memória partilhada\n");
        CloseHandle(hMapFile);
        return 1;
    }

    strcpy(msg.tipo, TIPO_ENTRAR);
    strncpy(msg.username, player, sizeof(msg.username));

    if (Pipe_bot(msg) == 1) return 1;

    HANDLE hThread = CreateThread(NULL, 0, Thread_Comando, NULL, 0, NULL);
    if (!hThread) return 1;

    while (!terminar) {
        WaitForSingleObject(hmutex, INFINITE);
        memcpy(letras_visiveis, letras_partilhadas, MAXLETRAS_ECRAN);

        printf("[BOT] Letras visiveis: %.*s\n", MAXLETRAS_ECRAN, letras_visiveis);

        const char* palavra = escolher_palavra();
        ReleaseMutex(hmutex);
        if (palavra && letras_suficientes(palavra)) {
            strcpy(msg.tipo, "palavra");
            strncpy(msg.username, player, sizeof(msg.username));
            strncpy(msg.palavra, palavra, sizeof(msg.palavra));
            msg.palavra[sizeof(msg.palavra) - 1] = '\0';  // segurança extra
            Pipe_bot(msg);
        }
        Sleep(5000 + rand()%29000);
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    return 0;
}
