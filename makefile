all: Megamaniac

LIBS=-lzdk04 -lncurses -lm -lrt
DIRS=-I../Topic05/ZDK04 -L../Topic05/ZDK04
FLAGS=-std=gnu99 -D_XOPEN_SOURCE=500 -Wall -Werror

clean:
	rm *.exe

rebuild: clean all

SRC=Megamaniac.c
Megamaniac: $(SRC) $(HDR) ../Topic05/ZDK04/libzdk04.a
	gcc $(FLAGS) $(SRC) -o $@ $(DIRS) $(LIBS) 

