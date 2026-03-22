compile: build run

build:
# 	gcc ./src/main.c -o ./out/sdl_game.exe -lSDL3
	gcc ./src/main.c -o main.exe -I "C:\SDL\x86_64-w64-mingw32\include" -L "C:\SDL\x86_64-w64-mingw32\lib" -lSDL3 -lSDL3_image
# 	gcc ./src/main.c -o ./out/sdl_game.exe -lSDL3 -lSDL3main
	
run: # build # ← 'run' depends on 'build' being fully complete
# 	./out/sdl_game.exe
	./main.exe

# .PHONY: all build run clean

# all: run

# build:
# 	gcc ./src/main.c -o ./out/sdl_game.exe -lSDL3
# 	cp /ucrt64/bin/SDL3.dll ./out/ 2>/dev/null || true

# run: build
# 	./out/sdl_game.exe

# clean:
# 	rm -f ./out/sdl_game.exe ./out/SDL3.dll