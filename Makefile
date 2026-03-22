compile: build run

build:
	gcc src/main.c -o main.exe \
	-I /ucrt64/include \
	-L /ucrt64/lib \
	-lSDL3_image -lSDL3
	
run:
	./main.exe