myshell : myshell.o lex.yy.o
	cc -o myshell myshell.o lex.yy.o -lfl 

myshell.o : myshell.c
	cc -c -std=c99 -Wall myshell.c

lex.yy.o: lex.yy.c
	cc -c -std=c99 lex.yy.c

lex.yy.c : lex.l
	flex lex.l

infer :
	make clean; infer-capture -- make; infer-analyze -- make

clean :
	rm -f myshell myshell.o lex.yy.o lex.yy.c