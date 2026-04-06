compile: build run

build:
	gcc src/main.c src/menu_screen.c src/loading_screen.c src/collision_system.c src/game_screen.c src/battle_system.c src/opening_animation.c src/video_player.c  src/dialogue_loader.c src/pause_menu.c src/save_menu.c src/music_system.c src/sfx_system.c src/save_system.c -o main.exe \
	-I /ucrt64/include \
	-L /ucrt64/lib \
	-lSDL3_image -lSDL3_ttf -lSDL3_mixer -lSDL3 -lm
	
run:
	./main.exe

# gcc -static -static-libgcc -static-libstdc++ -o game.exe \
    src/*.c \
    -Wl,-Bstatic -lSDL3_image -lSDL3_ttf -lSDL3 -Wl,-Bdynamic -lSDL3_mixer \
    -Wl,-Bstatic -lfreetype -lharfbuzz -lglib-2.0 -lpcre2-8 -lintl -liconv \
    -lbz2 -lpng16 -lz -lbrotlidec -lbrotlicommon -lgraphite2 \
    -Wl,-Bdynamic \
    -lsetupapi -ldxguid -lwinmm -limm32 -lole32 -loleaut32 \
    -luuid -lversion -lcomctl32 -lgdi32 -luser32 -lshell32 -ladvapi32 \
    -lrpcrt4 -ldwrite -lm -lws2_32