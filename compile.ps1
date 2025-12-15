# Compile test app
$sdlLib = "lib\SDL3\lib" 
$sdlInclude = "lib\SDL3\include"
$glewInclude = "lib\glew\include"
$glewLib = "lib\glew\lib"
$libInclude = "lib"

clang -std=c99 -O2 main.c `
-I"$glewInclude" `
-I"$sdlInclude" `
-I"$libInclude" `
-L"$glewLib" `
-L"$sdlLib" `
-lglew32 -lSDL3 -lopengl32 -lgdi32 -fopenmp `
-o build/app.exe -Wno-deprecated-declarations
