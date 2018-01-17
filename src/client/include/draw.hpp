#ifndef BOMBERMAN_DRAW_H
#define BOMBERMAN_DRAW_H

#include <SFML/Graphics.hpp>

extern "C" {
    #include "protocol.h"
    #include "state.h"
};

class GameWindow : public sf::RenderWindow {
public:
    explicit GameWindow(uint8_t id, sf::ContextSettings &settings) :
            sf::RenderWindow(sf::VideoMode(800, 600), "Bomberman", sf::Style::Titlebar | sf::Style::Close, settings),
            id(id) {

        if (!font.loadFromFile("assets/OpenSans-Regular.ttf")) {
            throw std::runtime_error("Failed to load font from file");
        }
    }

    uint16_t getInput();

    void drawLobby();

    void drawField();

    bool controlsOpen = false;

private:
    sf::Font font;
    uint8_t id;

    void drawControls();
};

#endif //BOMBERMAN_DRAW_H
