#ifndef BOMBERMAN_DRAW_H
#define BOMBERMAN_DRAW_H

#include <SFML/Graphics.hpp>

extern "C" {
    #include <protocol.h>
    #include <state.h>
    #include <utils.h>
};

class GameWindow : public sf::RenderWindow {
public:
    GameWindow(uint8_t id, sf::ContextSettings &settings);

    uint16_t getInput();

    void drawLobby();

    void drawGame();

    bool controlsOpen = false;

private:
    sf::Texture playerTexture;
    sf::Texture objectTexture;
    sf::Font font;
    uint8_t id;
    uint16_t lastInput = 0;
    int keyframe = 0;
    millis_t lastKeyframe = get_milliseconds();

    void drawControls();

    void drawField();

    void drawPlayers();

    void drawDynamites();

    void drawFlames();

    void drawPwrups();
};

#endif //BOMBERMAN_DRAW_H
