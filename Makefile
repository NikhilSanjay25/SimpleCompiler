CC     = clang
CFLAGS = $(shell llvm-config --cflags) -g
LIBS   = $(shell llvm-config --libs core analysis) $(shell llvm-config --ldflags) -lm

all: compiler

compiler: lex.yy.c y.tab.c ast.o codegen.o
	$(CC) $(CFLAGS) -o compiler lex.yy.c y.tab.c ast.o codegen.o $(LIBS)

lex.yy.c: lexer.l
	flex lexer.l

y.tab.c y.tab.h: parser.y
	bison -dy parser.y

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

codegen.o: codegen.c ast.h
	$(CC) $(CFLAGS) -c codegen.c

clean:
	rm -f lex.yy.c y.tab.c y.tab.h *.o compiler out.ll out.o myprog
