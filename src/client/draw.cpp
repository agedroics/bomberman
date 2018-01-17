#include "draw.hpp"

static sf::Font font;

uint16_t GameWindow::getInput() {
    uint16_t input = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        input |= INPUT_LEFT;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        input |= INPUT_UP;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        input |= INPUT_RIGHT;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        input |= INPUT_DOWN;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        input |= INPUT_PLANT;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
        input |= INPUT_DETONATE;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
        input |= INPUT_PICK_UP;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
        input |= INPUT_BONUS1;
    }
    return input;
}

static void setTextPosition(sf::Text &text, float x, float y) {
    text.setPosition((int) x, (int) (y - text.getLocalBounds().height / 4));
}

void GameWindow::drawLobby() {
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(20);
    for (int i = 0; i < player_cnt; ++i) {
        if (players[i].id == id) {
            text.setColor(sf::Color::Blue);
        } else {
            text.setColor(sf::Color::Black);
        }
        text.setString(player_names[players[i].id]);
        setTextPosition(text, 50, 60 + 40 * i);
        draw(text);

        sf::CircleShape readyCircle(10);
        readyCircle.setPosition(20, 60 + 40 * i);
        readyCircle.setOutlineThickness(1);
        readyCircle.setOutlineColor(sf::Color::Black);
        if (players[i].ready) {
            readyCircle.setFillColor(sf::Color::Green);
        } else {
            readyCircle.setFillColor(sf::Color::White);
        }
        draw(readyCircle);
    }

    text.setColor(sf::Color::Black);
    text.setString("Press c to toggle controls");
    setTextPosition(text, getSize().x / 2 - text.getLocalBounds().width / 2, getSize().y - 20 - text.getLocalBounds().height);
    draw(text);

    text.setCharacterSize(24);
    text.setString("LOBBY");
    text.setStyle(sf::Text::Bold);
    setTextPosition(text, getSize().x / 2 - text.getLocalBounds().width / 2, 20);
    draw(text);

    if (controlsOpen) {
        drawControls();
    }
}

void GameWindow::drawControls() {
    sf::RectangleShape rect;
    rect.setOutlineColor(sf::Color::Black);
    rect.setOutlineThickness(1);
    int x = getSize().x / 2 - 200;
    int y = getSize().y - 180;
    rect.setPosition(x, y);
    sf::Vector2f size;
    size.x = 400;
    size.y = 180;
    rect.setSize(size);
    rect.setFillColor(sf::Color::White);
    draw(rect);

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(20);
    text.setColor(sf::Color::Black);
    text.setString("Ready/plant: Space");
    setTextPosition(text, x + 20, y + 20);
    draw(text);

    text.setString("Movement: WASD");
    setTextPosition(text, x + 20, y + 60);
    draw(text);

    text.setString("Pick up bomb: E");
    setTextPosition(text, x + 20, y + 100);
    draw(text);

    text.setString("Remote detonator: F");
    setTextPosition(text, x + 20, y + 140);
    draw(text);
}

void GameWindow::drawField() {

}
