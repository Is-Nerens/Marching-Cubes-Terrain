# Set the base directory of your project
$baseDir = "$(Split-Path -Parent $MyInvocation.MyCommand.Path)"

# Compile main.cpp from src/ and create main.o
& clang++ -std=c++20 -fopenmp -c "src\main.cpp" -I"$baseDir\libs\SFML\include" -I"$baseDir\libs\glew\include" 

# Link main.o and create the application executable
& clang++ -o build\application.exe main.o -I"$baseDir\libs\SFML\include" -I"$baseDir\libs\glew\include" -L"$baseDir\libs\SFML\lib" -L"$baseDir\libs\glew\lib\Release\x64" -lopengl32 -lglew32 -lsfml-graphics -lsfml-window -lsfml-system -fopenmp


# Copy the textures directory into the build directory
# Remove-Item "$baseDir\build\textures" -Recurse -Force -ErrorAction SilentlyContinue
# Copy-Item "$baseDir\textures" -Destination "$baseDir\build\textures" -Recurse

# # Copy the shaders direcory into the build directory
Remove-Item "$baseDir\build\shaders" -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "$baseDir\shaders" -Destination "$baseDir\build\shaders" -Recurse

# Delete the main.o file
Remove-Item "main.o" -Force
