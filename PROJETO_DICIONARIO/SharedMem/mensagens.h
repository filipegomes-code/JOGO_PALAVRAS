#ifndef MENSAGENS_H
#define MENSAGENS_H

#define TIPO_ENTRAR "entrar"
#define TIPO_SAIR ":sair"
#define TIPO_LIM_PLAYERS "recusado"
#define TIPO_ACEITE "aceite"

typedef struct
{
    char tipo[20];
    char username[30];
    char palavra[30];
} mensagem;

typedef struct 
{
    HANDLE hpipe;
    mensagem msg_thread;
} infoThread;


#endif