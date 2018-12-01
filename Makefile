LIB='-Linclude'
INC='-Iinclude'

default: timage

timage: main.c
	gcc -std=c99 -Wall $(LIB) $(INC) -o timage main.c -lm -lncurses

clean:
	rm -r *.o timage
