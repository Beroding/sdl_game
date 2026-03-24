compile: build run

build:
	gcc src/main.c src/menu_screen.c src/game_screen.c src/battle_system.c src/opening_animation.c src/video_player.c -o main.exe \
	-I /ucrt64/include \
	-L /ucrt64/lib \
	-lSDL3_image -lSDL3_ttf -lSDL3
	
run:
	./main.exe