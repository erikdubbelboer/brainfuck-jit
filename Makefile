
all: brainfuck

brainfuck: brainfuck.c
	gcc -O3 brainfuck.c -o brainfuck -I lightning/ -I lightning/lightning/i386/

test: brainfuck
	./brainfuck mandelbrot.bf | diff mandelbrot.out -

