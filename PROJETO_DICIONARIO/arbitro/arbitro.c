#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Windows.h"
#include "../SharedMem/mensagens.h"


void main(){
    mensagem msg;
    HANDLE hpipe;

    hpipe = CreateNamedPipeA(("\\\\.\\pipe\\Pipe"), PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);

    if(hpipe == NULL){
        printf("erro a criar pipe");
        return 1;
    }

    BOOL connected = ConnectNamedPipe(hpipe, NULL);
    if(connected){
        printf("n existe pipe");
    }



}