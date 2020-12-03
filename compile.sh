echo Compiling  the program
gcc -c game.c
gcc -pthread -c server.c
gcc -pthread -o server game.o server.o

