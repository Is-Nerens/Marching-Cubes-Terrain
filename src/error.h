#ifndef ERROR_H
#define ERROR_H

#include <GL/glew.h>
#include <iostream>
#include <chrono>

void ClearGLErrors()
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {}
}

void PrintGLErrors()
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL Error: " << error << std::endl;
    }
}

namespace Debug
{
    double processTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    void StartTimer()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    void EndTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Duration: " << duration.count() << " ms" << std::endl;
    }





    double accumulatedTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> startAccum;

    void ResetAccumulation()
    {
        accumulatedTime = 0;
    }

    void ResumeAccum()
    {
        startAccum = std::chrono::high_resolution_clock::now();
    }

    void StopAccum()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - startAccum);
        accumulatedTime += duration.count();
    }

    void PrintAccum()
    {
        std::cout << "Accumulated time: " << accumulatedTime / 1e6 << "ms" << std::endl;
    }
};

#endif
