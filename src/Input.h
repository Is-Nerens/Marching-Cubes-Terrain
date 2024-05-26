#ifndef INPUT_H
#define INPUT_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <iostream>

enum class KeyCode {
    Num0 = 0,
    Num1 = 1,
    Num2 = 2,
    Num3 = 3,
    Num4 = 4,
    Num5 = 5,
    Num6 = 6,
    Num7 = 7,
    Num8 = 8,
    Num9 = 9,
    A = 10,
    B = 11,
    C = 12,
    D = 13,
    E = 14,
    F = 15,
    G = 16,
    H = 17,
    I = 18,
    J = 19,
    K = 20,
    L = 21,
    M = 22,
    N = 23,
    O = 24,
    P = 25,
    Q = 26,
    R = 27,
    S = 28,
    T = 29,
    U = 30,
    V = 31,
    W = 32,
    X = 33,
    Y = 34,
    Z = 35,
    Apostrophe = 36,
    Semicolon = 37,
    Comma = 38,
    Period = 39,
    Plus = 40,
    Minus = 41,
    Equal = 42,
    Slash = 43,
    Space = 44,
    LeftShift = 45,
    RightShift = 46,
    LeftControl = 47,
    RightControl = 48,
    LeftAlt = 49,
    RightAlt = 50,
    Tab = 51,
    Escape = 52,
    Unknown = 53,
};

class InputSystem {
public:

	// Constructor
    InputSystem (sf::RenderWindow* _windowPtr) : windowPtr(_windowPtr)
    {
        InitKeyMap();
    }

    bool GetKeyDown(KeyCode keyCode)
    {
    	return keyStates[static_cast<int>(keyCode)] == 1;
    }

    bool GetKeyUp(KeyCode keyCode)
    {
    	return keyStates[static_cast<int>(keyCode)] == 3;
    }

    bool GetKey(KeyCode keyCode)
    {
    	return keyStates[static_cast<int>(keyCode)] == 1 || keyStates[static_cast<int>(keyCode)] == 2;
    }

    bool MousePressed() { return (mouseButtonState == 1); }
    bool MouseDown() { return (mouseButtonState == 1); }
    bool MouseHidden() { return mouseHidden; }
    float MouseDeltaX() { return mouseDeltaX; }
    float MouseDeltaY() { return mouseDeltaY; }

    void HandleEvent(sf::Event &event)
    {
        // key pressed
        if (event.type == sf::Event::KeyPressed) {
            pressedKeys[GetKeyIndex(event.key.code)] = 1; // flag key as pressed-down
        }

        // key released
        if (event.type == sf::Event::KeyReleased) {
            releasedKeys[GetKeyIndex(event.key.code)] = 1; // flag key as released
        }


        // mouse button
        bool mouseLeftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
        {
            PressMouse();
        }

        // update Mouse
        if (mouseHidden)
        {
            sf::Vector2i mousePosition = sf::Mouse::getPosition(*windowPtr);
            sf::Vector2i center(windowPtr->getSize().x / 2, windowPtr->getSize().y / 2);
            sf::Vector2i delta = sf::Mouse::getPosition(*windowPtr) - center;
            sf::Mouse::setPosition(center, *windowPtr);
            UpdateMouseDelta(delta.x, delta.y);
        }
        else
        {   
            sf::Vector2i mousePosition = sf::Mouse::getPosition(*windowPtr);
            UpdateMousePosition();
        }
    }

    // called after all the "HandleEvent" function calls
    void UpdateKeyStates()
    {
        for (int i=0; i<53; ++i)
        {
            if (pressedKeys[i] == 1 && keyStates[i] == 0)
            {
                keyStates[i] = 1; // on down triggered
            }

            if (pressedKeys[i] == 0 && releasedKeys[i] == 0 && keyStates[i] == 1)
            {
                keyStates[i] = 2; // pressed (ehld)
            }

            if (releasedKeys[i] == 1)
            {
                keyStates[i] = 3; // key released
            }
        }
    }

    void NewFrame()
    {
        // set mouse state to 0 (unpressed)
        mouseButtonState = 0;

        for (int i=0; i<53; ++i)
        {
            if (keyStates[i] == 3) keyStates[i] = 0;
            pressedKeys[i] = 0;
            releasedKeys[i] = 0;
        }
    }

    void LockMouse(float posX, float posY)
    {
        // set variables
        prevMouseX = posX;
        prevMouseY = posY;
        mouseDeltaX = 0.0f;
        mouseDeltaY = 0.0f;
        mouseHidden = true;
        
        // set sf mouse
        sf::Vector2i center(windowPtr->getSize().x / 2, windowPtr->getSize().y / 2);
        sf::Vector2i delta = sf::Mouse::getPosition(*windowPtr) - center;
        sf::Mouse::setPosition(center, *windowPtr);
    }

    void ShowMouse()
    {
        mouseDeltaX = 0.0f;
        mouseDeltaY = 0.0f;
        mouseHidden = false;
    }

private:
    sf::RenderWindow* windowPtr;
    float prevMouseX;
    float prevMouseY;
    float mouseDeltaX;
    float mouseDeltaY;
    bool mouseHidden = true;
    int mouseButtonState = 0;

    void UpdateMouseDelta(float dX, float dY)
    {
        mouseDeltaX = dX;
        mouseDeltaY = dY;
    }

    void UpdateMousePosition()
    {
        mouseDeltaX = sf::Mouse::getPosition(*windowPtr).x - prevMouseX;
        mouseDeltaY = sf::Mouse::getPosition(*windowPtr).x - prevMouseY;
        prevMouseX = sf::Mouse::getPosition(*windowPtr).x;
        prevMouseY = sf::Mouse::getPosition(*windowPtr).y;
    }

    void PressMouse()
    {
        mouseButtonState = 1;
    }


    // 0 - nothing
    // 1 - on down
    // 2 - pressed
    // 3 - on up
    int keyStates[53] = {0};
    int pressedKeys[53] = {0};
    int releasedKeys[53] = {0};

    std::unordered_map<sf::Keyboard::Key, int> keyMap;

    int GetKeyIndex(sf::Keyboard::Key& key)
    {
        return keyMap[key];
    }

    void InitKeyMap()
    {
        keyMap[sf::Keyboard::Num0] = 0;
        keyMap[sf::Keyboard::Num1] = 1;
        keyMap[sf::Keyboard::Num2] = 2;
        keyMap[sf::Keyboard::Num3] = 3;
        keyMap[sf::Keyboard::Num4] = 4;
        keyMap[sf::Keyboard::Num5] = 5;
        keyMap[sf::Keyboard::Num6] = 6;
        keyMap[sf::Keyboard::Num7] = 7;
        keyMap[sf::Keyboard::Num8] = 8;
        keyMap[sf::Keyboard::Num9] = 9;
        keyMap[sf::Keyboard::A] = 10;
        keyMap[sf::Keyboard::B] = 11;
        keyMap[sf::Keyboard::C] = 12;
        keyMap[sf::Keyboard::D] = 13;
        keyMap[sf::Keyboard::E] = 14;
        keyMap[sf::Keyboard::F] = 15;
        keyMap[sf::Keyboard::G] = 16;
        keyMap[sf::Keyboard::H] = 17;
        keyMap[sf::Keyboard::I] = 18;
        keyMap[sf::Keyboard::J] = 19;
        keyMap[sf::Keyboard::K] = 20;
        keyMap[sf::Keyboard::L] = 21;
        keyMap[sf::Keyboard::M] = 22;
        keyMap[sf::Keyboard::N] = 23;
        keyMap[sf::Keyboard::O] = 24;
        keyMap[sf::Keyboard::P] = 25;
        keyMap[sf::Keyboard::Q] = 26;
        keyMap[sf::Keyboard::R] = 27;
        keyMap[sf::Keyboard::S] = 28;
        keyMap[sf::Keyboard::T] = 29;
        keyMap[sf::Keyboard::U] = 30;
        keyMap[sf::Keyboard::V] = 31;
        keyMap[sf::Keyboard::W] = 32;
        keyMap[sf::Keyboard::X] = 33;
        keyMap[sf::Keyboard::Y] = 34;
        keyMap[sf::Keyboard::Z] = 35;
        keyMap[sf::Keyboard::Quote] = 36;
        keyMap[sf::Keyboard::SemiColon] = 37;
        keyMap[sf::Keyboard::Comma] = 38;
        keyMap[sf::Keyboard::Period] = 39;
        keyMap[sf::Keyboard::Add] = 40;
        keyMap[sf::Keyboard::Subtract] = 41;
        keyMap[sf::Keyboard::Equal] = 42;
        keyMap[sf::Keyboard::Slash] = 43;
        keyMap[sf::Keyboard::Space] = 44;
        keyMap[sf::Keyboard::LShift] = 45;
        keyMap[sf::Keyboard::RShift] = 46;
        keyMap[sf::Keyboard::LControl] = 47;
        keyMap[sf::Keyboard::RControl] = 48;
        keyMap[sf::Keyboard::LAlt] = 49;
        keyMap[sf::Keyboard::RAlt] = 50;
        keyMap[sf::Keyboard::Tab] = 51;
        keyMap[sf::Keyboard::Escape] = 52;
    }
};
	
#endif

