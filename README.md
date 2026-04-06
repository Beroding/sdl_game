1. install mysys64

2. open in windows MYSYS2 UCRT64

3. run "pacman -Syu" //to update, if it tells you to close it just close and reopen and run these command again until you see "there is nothing to do"

4. run "pacman -Sy" //refresh package database explicitly

5. run "pacman -Ss sdl3"
you should see something like:
- mingw-w64-ucrt-x86_64-sdl3
- mingw-w64-ucrt-x86_64-sdl3-image
- mingw-w64-ucrt-x86_64-sdl3-ttf

6. install the package above
run "pacman -S mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-sdl3-image mingw-w64-ucrt-x86_64-sdl3-ttf"
note: might have slight name difference so dont just copy and paste

7. add some file from "msys64/ucrt64/bin" (if you download the zip file it will be in the bin folder)
- SDL3.dll
- SDL3_image.dll
- SDL3_ttf.dll

8. install make
run "pacman -S make"

9. add it to new terminal profile in vscode (until i know how to write it this part will remain like this, i dont know how to explain it)

10. open ucrt64 terminal and run "make"
