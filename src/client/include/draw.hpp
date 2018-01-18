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
    explicit GameWindow(uint8_t id);

    uint16_t getInput();

    void drawLobby();

    void drawGame();

    void drawGameEnding();

    bool controlsOpen = false;

private:
    sf::Texture spriteSheet;
    sf::Texture tileset;
    sf::Texture medals;
    sf::Texture clock;
    sf::Font font;
    uint8_t id;
    int keyframe = 0;
    millis_t lastKeyframe = get_milliseconds();

    void drawControls();

    void drawField();

    void drawFadingBoxes();

    void drawPlayers();

    void drawDynamites();

    void drawFlames();

    void drawPwrups();

    void drawHud();
};

#endif //BOMBERMAN_DRAW_H
