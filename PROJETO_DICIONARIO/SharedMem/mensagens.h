#ifndef MENSAGENS_H
#define MENSAGENS_H

#define TIPO_ENTRAR "entrar"
#define TIPO_SAIR ":sair"


typedef struct
{
    char tipo[20];
    char username[30];
    char palavra[30];
} mensagem;


#endif