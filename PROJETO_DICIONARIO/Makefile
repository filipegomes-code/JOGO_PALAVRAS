# Variáveis
CC = gcc
CFLAGS = -Wall
INCLUDES = -I./SharedMem

all: arbitro.exe JogoUI.exe bot.exe

arbitro.exe: arbitro/arbitro.c
	$(CC) $(CFLAGS) $(INCLUDES) -o arbitro.exe arbitro/arbitro.c

JogoUI.exe: JogoUI/JogoUI.c
	$(CC) $(CFLAGS) $(INCLUDES) -o JogoUI.exe JogoUI/JogoUI.c

bot.exe: bot/bot.c
	$(CC) $(CFLAGS) $(INCLUDES) -o bot.exe bot/bot.c

clean:
	del /Q *.exe

