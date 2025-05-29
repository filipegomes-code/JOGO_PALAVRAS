// ARBITRO.C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>
#include <time.h>
#include "../SharedMem/mensagens.h"

#define MAX_PLAYER 20
#define TAM_NOME 20
#define MAXLETRAS_ECRAN 5
#define MAX_PALAVRAS 47038

int MAX_LETRAS = 6;
int ritmo = 3; // ritmo em segundos (mutável)

mensagem msg;
HANDLE hmutex;
HANDLE Pipe_player_threads[MAX_PLAYER]; //um handle de arrays para guardar as threads criadas para cada jogador

char dicionario[MAX_PALAVRAS][13]; // armazena as palavras do dicionario (dicionario_pt_eng.txt), uso 13, pq é 12+\0
int total_pal = 0;

char vetorletras[MAXLETRAS_ECRAN]; // array para cada letra
int letrascont = 0; // numeros de letras em uso
int comecou = 0; //flag 

char PLAYER_NAMES[MAX_PLAYER][TAM_NOME];
int cont_players = 0; 
int pontuacao[MAX_PLAYER] = {0}; // inicializa todas as pontuaçoes a zero
int pontuacao_lider = -1;
char nome_lider[TAM_NOME] = "";
// memoria partilhada
HANDLE hMapFile;
char* letras_partilhadas;

void print_comandos(){

    printf("COMANDOS:\n");
    printf("listar - mostra jogadores e a sua pontuacao\n");
    printf("excluir - Permite expulsar jogador do jogo\n");
    printf("acelerar - Permite aumentar o ritmo\n");
    printf("travar - Permite desacelerar o ritmo\n");
    printf("iniciarbot <nome> - inicia um jogador bot automatico\n");
    printf("encerrar - encerra todos os programas ativos\n\n");
}

void armazenar_ritmo_registry() {
    HKEY hKey;
    DWORD disp;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\TP_SO2"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, &disp) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, TEXT("Ritmo"), 0, REG_DWORD, (const BYTE*)&ritmo, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void armazenar_maxletras_registry() {
    HKEY hKey;
    DWORD disp;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\TP_SO2"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, &disp) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, TEXT("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&MAX_LETRAS, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void ler_ritmo_registry() {
    HKEY hKey;
    DWORD tamanho = sizeof(DWORD);
    DWORD valor = 3;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\TP_SO2"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, TEXT("Ritmo"), NULL, NULL, (LPBYTE)&valor, &tamanho) == ERROR_SUCCESS) {
            ritmo = (int)valor;
        }
        RegCloseKey(hKey);
    }
}

void ler_maxletras_registry() {
    HKEY hKey;
    DWORD tamanho = sizeof(DWORD);
    DWORD valor = 6;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\TP_SO2"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, TEXT("MAXLETRAS"), NULL, NULL, (LPBYTE)&valor, &tamanho) == ERROR_SUCCESS) {
            MAX_LETRAS = (int)valor > 12 ? 12 : (int)valor;
        }
        RegCloseKey(hKey);
    }
}

// suposto enviar a todos os JogoUIs oq esta a acontecer
void warnsAll(const char* mensagem){
    char pipename[50];

    WaitForSingleObject(hmutex, INFINITE);
    for (int i = 0; i < cont_players; i++) {
        snprintf(pipename, sizeof(pipename), "\\\\.\\pipe\\Pipe_Comando_%s", PLAYER_NAMES[i]);

        HANDLE hPipeComando = CreateFileA(pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if(hPipeComando != INVALID_HANDLE_VALUE) {
            DWORD byteswrite;
            BOOL wrote = WriteFile(hPipeComando, mensagem, strlen(mensagem) + 1, &byteswrite, NULL);
            if(!wrote) {
                printf("[WARNsAll] Falha ao escrever no pipe do jogador %s\n", PLAYER_NAMES[i]);
            }
            CloseHandle(hPipeComando);
        } else {
           // printf("[WARNsAll] Não conseguiu abrir pipe do jogador %s\n", PLAYER_NAMES[i]);
        }
    }
    ReleaseMutex(hmutex);
}

//Fazer um dicionario com um file .txt c/ palavras e usar fopen
void armazenar_dicionario(const char* nomefile){

    FILE* file = fopen(nomefile, "r");
    if(!file){
        perror("erro ao abrir file");
        exit(1);
    }

    while(fgets(dicionario[total_pal], MAX_LETRAS, file)){
        dicionario[total_pal][strcspn(dicionario[total_pal], "\r\n")] = 0; // remove \r\n
        total_pal++;
        if(total_pal >= MAX_PALAVRAS) break;
    }

    fclose(file);
    printf("dicionario armazenado com %d palavras\n", total_pal);
}

int name_repeated(const char* nome){

    for(int i = 0; i < cont_players; i++){
        if( strcmp(PLAYER_NAMES[i] , nome) == 0)
            return 1;
    }
    return 0;
}

void jogo_start(){
    if(cont_players >= 2 && comecou == 0){
        comecou = 1;
        HANDLE hthreadletras = CreateThread(NULL, 0, Thread_letras, NULL, 0, NULL);
        if(hthreadletras !=NULL){
            CloseHandle(hthreadletras);
        }
    }
}

int addPlayer(const char* nome, HANDLE hpipe){
    
    WaitForSingleObject(hmutex, INFINITE);
    printf("[ARBITRO] Player %s adicionado.", nome);
    strcpy(PLAYER_NAMES[cont_players], nome);
    Pipe_player_threads[cont_players] = hpipe;
    pontuacao[cont_players] = 0;
    cont_players++;
    //mensagem enviada para os outros jogoui, é como se tovesse TIPO_ENTRAR
    char msg[100];
    snprintf(msg, sizeof(msg), "player ENTROU: %s", nome);
    // define a partir de quantos players chamamos a thread para gerar as letras, ou seja, pode-se dizer que é quando o jogo começa
    jogo_start();
    Sleep(100);
    warnsAll(msg);

    ReleaseMutex(hmutex);

    return 0;
}

void removePlayer(const char* nome){
    WaitForSingleObject(hmutex, INFINITE);

    for(int i = 0; i < cont_players ; i++){
        if(strcmp(PLAYER_NAMES[i], nome) == 0){
            //msg enviada para os outros jogoui
            char msg[100];
            snprintf(msg, sizeof(msg), "player SAIU: %s", nome);
            warnsAll(msg);


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

    printf("Jogador nao aceite, cheio ou nome repetido");

    DWORD byteswrite;
    WriteFile(hpipe, TIPO_LIM_PLAYERS, strlen(TIPO_LIM_PLAYERS)+1, &byteswrite, NULL);
                
    DisconnectNamedPipe(hpipe); // se limite for atingido, "guarda" o pipe criado e é usado no proximo jogador que entrar, em vez, de se criar um novo pipe . n era preciso fazer , mas é interessante.
    CloseHandle(hpipe);
    
}

void Comandos(mensagem msg, HANDLE hpipe){
    if(strcmp(msg.tipo, TIPO_ENTRAR)== 0){
        WaitForSingleObject(hmutex,INFINITE);

        int cheio = cont_players >= MAX_PLAYER;
        int repetido = name_repeated(msg.username);
        ReleaseMutex(hmutex);

        if(cheio || repetido){
            // posso usar um operador ternário para dar printf quando é recusado por nome repetido ou limite maximo, mas n pus ainda e ns se vou por
            player_rejeitado(hpipe, TIPO_LIM_PLAYERS);
            return;
        }
        DWORD bytesWritten;
        WriteFile(hpipe, "aceite" , strlen("aceite") + 1, &bytesWritten, NULL);
        addPlayer(msg.username, hpipe);
        // warnsall (opcional)
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
        // warnsall (opcional)
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
        // warnsall (opcional)
    }
    else if(strcmp(msg.tipo, TIPO_SAIR) == 0){
        removePlayer(msg.username);
        printf("jogador %s saiu do jogo\n", msg.username);
        CloseHandle(hpipe);
        return;
    }
}

int pal_existe(const char* palavra){

    for(int i=0; i < total_pal; i++){
        if(strcmp(dicionario[i], palavra) == 0){
            return 1;
        }
    }
    return 0;
}

int letras_visivel(const char* palavra){

    int conta_vetor[26] = {0};
    int conta_pal[26] = {0};

    // vetorletras -> array para cada letra
    WaitForSingleObject(hmutex, INFINITE);
    for(int i = 0; i < MAXLETRAS_ECRAN; i++){
        char c = vetorletras[i];
        if( c>= 'a' && c<= 'z'){
            conta_vetor[c-'a']++;
        }
    }
    ReleaseMutex(hmutex);

    // conta se quantidade de letras na palavra é igual às visiveis
    for(int i = 0; palavra[i] != '\0'; i++){
        char c = palavra[i];
        if( c>= 'a' && c<='z')
            conta_pal[c - 'a']++;
        else
            return 0;
    }

    for(int i =0; i < 26; i++){
        if(conta_pal[i] > conta_vetor[i]){
            return 0;
        }
    }

    return 1; // palavra pode ser formada com letras visiveis
}
// imprime qual pessoa tem a maior pontuacao e passou à frente, sempre que alguem passa à frente, pode aparecer várias vezes
void pontuacao_maior(){
    int max_pontos = -1;
    char novo_lider[TAM_NOME] = "";

    for (int i = 0; i < cont_players; i++) {
        if (pontuacao[i] > max_pontos) {
            max_pontos = pontuacao[i];
            strcpy(novo_lider, PLAYER_NAMES[i]);
        }
    }

    // Só avisa se o líder mudou
    if (strcmp(nome_lider, novo_lider) != 0) {
        strcpy(nome_lider, novo_lider);
        pontuacao_lider = max_pontos;

        char msg[150];
        snprintf(msg, sizeof(msg), "Jogador %s passou para a frente com %d pontos", novo_lider, pontuacao_lider);
        warnsAll(msg);
    }
}

void anunciar_vencedor_encerrar(){
    char vencedor[TAM_NOME] = "";
    int max_pontos = -1;

    for (int i = 0; i < cont_players; i++) {
        if (pontuacao[i] > max_pontos) {
            max_pontos = pontuacao[i];
            strcpy(vencedor, PLAYER_NAMES[i]);
        }
    }

    char msg_final[150];
    snprintf(msg_final, sizeof(msg_final), "VENCEDOR: %s com %d pontos", vencedor, max_pontos);
    printf("[FIM DO JOGO] %s\n", msg_final);

    // envia mensagem do vencedor para todos os jogadores
    warnsAll(msg_final);
    // espera um pouco para garantir que a mensagem é lida antes de encerrar
    Sleep(100);

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
}

int valida_pal(const char* username, const char* palavra, HANDLE hpipe){

    // ver se a palavra existe if(pal_existe);
    if(!pal_existe(palavra)){
        printf("[ARBITRO] pal n existe no dicionario\n");
        WaitForSingleObject(hmutex, INFINITE);
        for(int i =0; i < cont_players; i++){
            if(strcmp(PLAYER_NAMES[i], username)==0){
                pontuacao[i]-= (int)(strlen(palavra)/2);
                break;
            }
        }
        ReleaseMutex(hmutex);

        const char* resposta = "Palavra n existe";
        DWORD write;
        WriteFile(hpipe, resposta, strlen(resposta)+1, &write, NULL);

        return 0;
    }

    // ver se as letras da palavra estao visiveis e na msm quantidade if(letras_visiveis);
    if(!letras_visivel(palavra)){
        printf("erro letra já desapareceu ou funcao n funciona");
        WaitForSingleObject(hmutex, INFINITE);
        for(int i =0; i < cont_players; i++){
            if(strcmp(PLAYER_NAMES[i], username) == 0){
                pontuacao[i]-= (int)(strlen(palavra) / 2);
                break;
            }
        }
        ReleaseMutex(hmutex);

        const char* resposta = "Palavra invalida, n tem letras todas";
        DWORD write;
        WriteFile(hpipe, resposta, strlen(resposta)+1, &write, NULL);

        return 0;
    }

    //dar 1 ponto por letra, acho que diz isso no enunciado
    WaitForSingleObject(hmutex, INFINITE); // só uso este mutex aqui, pq vou enviar uma resposta para o jogoui
    for(int i = 0; i < cont_players; i++){
        if(strcmp(PLAYER_NAMES[i], username)== 0){
            pontuacao[i]+= (int)strlen(palavra);
            break;
        }
    }
    
    char msg[150];
    snprintf(msg, sizeof(msg), "Jogador %s adivinhou a palavra '%s'", username, palavra);
    warnsAll(msg);
    // funcao para mostrar oq tem pontuacao mais alta
    pontuacao_maior();

    // remover letras que estao no vetorletras[]
    for(int i=0; i < strlen(palavra); i++ ){
        for(int j =0; j < MAXLETRAS_ECRAN; j++){
            if(vetorletras[j] == palavra[i]){
                vetorletras[j] = '_';
                break;
            }
        }
    }
    ReleaseMutex(hmutex);

    const char* resposta = "Palavra valida";
    DWORD write;
    WriteFile(hpipe, resposta, strlen(resposta)+1, &write, NULL);

    return 1;
}

DWORD WINAPI Thread_player(LPVOID lpParam){
    infoThread* info = (infoThread*)lpParam; //fazemos um cast para HANDLE
    HANDLE hpipe = info->hpipe;
    mensagem msg = info->msg_thread;
    
    if(strcmp(msg.tipo, "palavra") == 0){
        // funcao para ver se a palavra esta no dicionario
        valida_pal(msg.username, msg.palavra, hpipe);
    }else
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
            // enviar mensagem final a todos e encerrar
            anunciar_vencedor_encerrar();
            ReleaseMutex(hmutex);
            printf("Jogo encerrado pelo arbitro\n");
            exit(0);
        }
        else if (strncmp(input, "iniciarbot ",11)== 0){
            char* nomebot = input + 11;

            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
        
            char cmd[100];
            snprintf(cmd, sizeof(cmd), "bot.exe %s", nomebot); // ou ".\\bot.exe %s" se estiveres com problemas no caminho

            BOOL success = CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0 , NULL, NULL, &si, &pi);

            if (success) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                printf("BOT %s iniciado com sucesso.\n", nomebot);
            } else {
                printf("Erro ao iniciar BOT %s.\n", nomebot);
            }
        }else if( strcmp(input,"acelerar")==0){
            ritmo++;
            armazenar_ritmo_registry();
            printf("[ARBITRO] Ritmo aumentado para %d segundos", ritmo);
        }else if( strcmp(input, "travar")==0){
            if(ritmo > 1) ritmo--;
            armazenar_ritmo_registry();
            printf("[ARBITRO] Ritmo diminuido para %d segundos", ritmo);
        }else
            printf("comando desconhecido\n");
    }

    return 0;
}

int gerarletras(){
    for(int i =0; i < MAXLETRAS_ECRAN; i++) vetorletras[i] = '_';

    return 0;
}

char sortearletras(){ return 'a' + (rand() % 26); }

// permite sortear as letras sem entrar em race conditions 
DWORD WINAPI Thread_letras(LPVOID lpParam){

    srand((unsigned)time(NULL)); // srand espera sempre um num positivo

    int tempovida[MAXLETRAS_ECRAN] = {0};
    gerarletras(); // é preciso por isto aqui, senao o array nunca é inicializado com '_' e a verificaçao dentro de threads falha

    while(1){
        Sleep(ritmo * 1000); // passa de ms para s

        WaitForSingleObject(hmutex, INFINITE);

        int livre = -1;
        // insere letra na 1ª posiçao livre 
        for(int i = 0; i < MAXLETRAS_ECRAN; i++){
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
            for(int i = 1; i < MAXLETRAS_ECRAN; i++){
                if(tempovida[i] > maxtempo){
                    maxtempo = tempovida[i];
                    posantiga = i;
                }
            }

            vetorletras[posantiga] = sortearletras();
            tempovida[posantiga] = 1;
        }

        // incrementa segundos a cada elemento que é uma letra no vetor
        for(int i = 0; i < MAXLETRAS_ECRAN; i++)
            if(vetorletras[i] != '_') tempovida[i]++; 

        // atualiza mem partilhada
        memcpy(letras_partilhadas, vetorletras, MAXLETRAS_ECRAN);
        ReleaseMutex(hmutex);

        printf("linha das letras ");
        for(int i = 0; i < MAXLETRAS_ECRAN; i++){
            printf("%c ", vetorletras[i]);
        }
        putchar('\n');
    }

    return 0;
}

int main(){ 
    //mensagem msg;
    HANDLE hpipe;
    ler_ritmo_registry();
    ler_maxletras_registry();
    armazenar_maxletras_registry();
    armazenar_dicionario("arbitro/dicionario_pt_eng.txt");

    print_comandos();
    printf("[AGUARDANDO PLAYERS.....]");

    hmutex = CreateMutexA(NULL, FALSE, "Global\\geral");

    if(hmutex == NULL){
        printf("erro ao criar mutex");
        return 1;
    }
    // cria o mapa, é como se fosse um quadro preto
    hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAXLETRAS_ECRAN, "Global\\LetrasPartilhadas");
    if (hMapFile == NULL) {
        printf("Erro ao criar memoria partilhada\n");
        return 1;
    }
    // aqui é como se tivessemos um projetor/lanterna que aponta para o quadro preto
    letras_partilhadas = (char*) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, MAXLETRAS_ECRAN);
    if (letras_partilhadas == NULL) {
        printf("Erro ao mapear memoria\n");
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