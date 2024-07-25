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
    double accumulatedTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> startAccum;


    void StartTimer()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    void EndTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Duration: " << duration.count() << " microseconds" << std::endl;
    }

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
        std::cout << "Accumulated time: " << accumulatedTime / 1e6 << "ms" << std::endl;
    }
};

void LinePlot(sf::RenderWindow& window, const std::vector<glm::vec2>& points, int startIndex = 0, float thickness = 3.0f)
{
    sf::VertexArray lines(sf::Quads);

    for (int i = 1; i < points.size(); ++i)
    {
        int index = (i + startIndex) % points.size();
        int lastIndex = (i - 1 + startIndex) % points.size();

        // Calculate direction vector
        sf::Vector2f dir(points[index].x - points[lastIndex].x, points[index].y - points[lastIndex].y);
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir /= length; // Normalize

        // Perpendicular vector
        sf::Vector2f perp(-dir.y, dir.x);

        // Half thickness vectors
        sf::Vector2f halfThickness = perp * (thickness / 2.0f);

        // Define vertices for the thick line
        sf::Vertex v1(sf::Vector2f(points[lastIndex].x + halfThickness.x, points[lastIndex].y + halfThickness.y), sf::Color::Red);
        sf::Vertex v2(sf::Vector2f(points[lastIndex].x - halfThickness.x, points[lastIndex].y - halfThickness.y), sf::Color::Red);
        sf::Vertex v3(sf::Vector2f(points[index].x - halfThickness.x, points[index].y - halfThickness.y), sf::Color::Red);
        sf::Vertex v4(sf::Vector2f(points[index].x + halfThickness.x, points[index].y + halfThickness.y), sf::Color::Red);

        lines.append(v1);
        lines.append(v2);
        lines.append(v3);
        lines.append(v4);
    }

    window.draw(lines);
}


void LineGraph(sf::RenderWindow& window, std::vector<float> values, int x, int y, int w, int h, int min, int max)
{
    std::vector<glm::vec2> points;
    for (int i=0; i<values.size(); ++i)
    {
        float yNorm = std::min((values[i] - min) / (max - min), 1.0f);
        float pointY = y + h - (yNorm * h);
        float pointX = x + (static_cast<float>(i) / (values.size()-1)) * w;
        
        glm::vec2 point(pointX, pointY);
        points.push_back(point);
    }

    LinePlot(window, points, 0);

    // RECTANGLE BORDER
    sf::RectangleShape border(sf::Vector2f(static_cast<float>(w), static_cast<float>(h)));
    border.setPosition(static_cast<float>(x), static_cast<float>(y));
    border.setOutlineColor(sf::Color::White);
    border.setOutlineThickness(2.0f); // Thickness of the border
    border.setFillColor(sf::Color::Transparent); // No fill color
    window.draw(border);
}

#endif
