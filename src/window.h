#ifndef WINDOW_H
#define WINDOW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <stdexcept> 

void ApplyWindowMinSize(sf::RenderWindow &window, int minWidth, int minHeight)
{
    window.setSize(sf::Vector2u(
        static_cast<unsigned int>(std::max(static_cast<int>(window.getSize().x), minWidth)),
        static_cast<unsigned int>(std::max(static_cast<int>(window.getSize().y), minHeight))
    ));
}

#endif // WINDOW_HPP