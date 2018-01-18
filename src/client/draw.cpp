#include <draw.hpp>

static sf::Font font;

GameWindow::GameWindow(uint8_t id, sf::ContextSettings &settings) :
        sf::RenderWindow(sf::VideoMode(1024, 768), "Bomberman", sf::Style::Default, settings),
        id(id) {

    if (!font.loadFromFile("assets/OpenSans-Regular.ttf")) {
        throw std::runtime_error("Failed to load font");
    }
    if (!playerTexture.loadFromFile("assets/player.png")) {
        throw std::runtime_error("Failed to load player textures");
    }
    if (!objectTexture.loadFromFile("assets/object.png")) {
        throw std::runtime_error("Failed to load object textures");
    }
}

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
    lastInput = input;
    return input;
}

static void setTextPosition(sf::Text &text, float x, float y) {
    text.setPosition((int) x, (int) (y - text.getCharacterSize() / 6));
}

void GameWindow::drawLobby() {
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(16);
    text.setColor(sf::Color::Black);
    for (int i = 0; i < player_cnt; ++i) {
        if (players[i].id == id) {
            text.setStyle(sf::Text::Style::Bold);
        } else {
            text.setStyle(sf::Text::Style::Regular);
        }
        text.setString(player_infos[players[i].id].name);
        setTextPosition(text, 50, 60 + 40 * i);
        draw(text);

        sf::CircleShape readyCircle(10);
        readyCircle.setPosition(20, 60 + 40 * i);
        readyCircle.setOutlineThickness(1);
        readyCircle.setOutlineColor(sf::Color::Black);
        if (players[i].ready) {
            readyCircle.setFillColor(sf::Color(50, 205, 50));
        } else {
            readyCircle.setFillColor(sf::Color::White);
        }
        draw(readyCircle);
    }

    text.setStyle(sf::Text::Style::Regular);
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
    rect.setSize(sf::Vector2f(400, 180));
    rect.setFillColor(sf::Color::White);
    draw(rect);

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(16);
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
    sf::Sprite wall(objectTexture, sf::IntRect(0, 0, 16, 16));
    wall.setScale(2, 2);
    sf::Sprite box(objectTexture, sf::IntRect(16, 0, 16, 16));
    box.setScale(2, 2);
    sf::Sprite empty(objectTexture, sf::IntRect(32, 0, 16, 16));
    empty.setScale(2, 2);
    sf::Sprite empty2(objectTexture, sf::IntRect(48, 0, 16, 16));
    empty2.setScale(2, 2);
    for (int y = 0; y < field_height; ++y) {
        for (int x = 0; x < field_width; ++x) {
            if (field_get(x, y) == BLOCK_WALL) {
                wall.setPosition(32 * x, 32 * y);
                draw(wall);
            } else if (field_get(x, y) == BLOCK_BOX) {
                box.setPosition(32 * x, 32 * y);
                draw(box);
            } else if (field_get(x, y) == BLOCK_EMPTY) {
                if (y > 0 && field_get(x, y - 1) != BLOCK_EMPTY) {
                    empty2.setPosition(32 * x, 32 * y);
                    draw(empty2);
                } else {
                    empty.setPosition(32 * x, 32 * y);
                    draw(empty);
                }
            }
        }
    }
}

void GameWindow::drawPlayers() {
    sf::Sprite player(playerTexture);
    player.setScale(2, 2);
    sf::Text name;
    name.setFont(font);
    name.setCharacterSize(16);
    name.setStyle(sf::Text::Style::Bold);
    for (int i = 0; i < player_cnt; ++i) {
        if (!players[i].dead) {
            int offset;
            if (player_infos[players[i].id].last_x != players[i].x
                || player_infos[players[i].id].last_y != players[i].y) {

                player_infos[players[i].id].last_x = players[i].x;
                player_infos[players[i].id].last_y = players[i].y;
                player_infos[players[i].id].movement_last_detected = get_milliseconds();
            }
            if (get_milliseconds() - player_infos[players[i].id].movement_last_detected <= 100) {
                if (keyframe % 4 < 3) {
                    offset = 16 * (keyframe % 4);
                } else {
                    offset = 16 * (4 - keyframe % 4);
                }
            } else {
                offset = 16;
            }
            sf::Vector2f pos((int) (players[i].x * 3.2 - 16), (int) (players[i].y * (float) 3.2 - 32));
            player.setPosition(pos);
            switch (players[i].direction) {
                case DIRECTION_LEFT:
                    player.setTextureRect(sf::IntRect(offset, 0, 16, 24));
                    break;
                case DIRECTION_UP:
                    player.setTextureRect(sf::IntRect(48 + offset, 0, 16, 24));
                    break;
                case DIRECTION_RIGHT:
                    player.setTextureRect(sf::IntRect(96 + offset, 0, 16, 24));
                    break;
                case DIRECTION_DOWN:
                    player.setTextureRect(sf::IntRect(144 + offset, 0, 16, 24));
                    break;
                default:
                    break;
            }
            draw(player);
            if (players[i].id == id) {
                name.setString("You");
                name.setColor(sf::Color::Cyan);
            } else {
                name.setString(player_infos[players[i].id].name);
                name.setColor(sf::Color::White);
            }

            name.setPosition((int) (players[i].x * 3.2 - name.getLocalBounds().width / 2), pos.y - 28);
            draw(name);
        }
    }
}

void GameWindow::drawDynamites() {
    int offset;
    if (keyframe % 4 < 3) {
        offset = 16 * (keyframe % 4);
    } else {
        offset = 16 * (4 - keyframe % 4);
    }
    sf::Sprite dyn(objectTexture, sf::IntRect(64 + offset, 64, 16, 16));
    dyn.setScale(2, 2);
    for (int i = 0; i < dyn_cnt; ++i) {
        dyn.setPosition((dynamites[i].x - 5) * 3.2f, (dynamites[i].y - 5) * 3.2f);
        draw(dyn);
    }
}

void GameWindow::drawFlames() {
    sf::Sprite flame(objectTexture);
    flame.setScale(2, 2);
    int offset;
    if (keyframe % 6 < 4) {
        offset = 16 * (keyframe % 6);
    } else {
        offset = 16 * (6 - keyframe % 6);
    }
    for (int i = 0; i < flame_cnt; ++i) {
        if ((flame_map_get(flames[i].x - 1, flames[i].y) || flame_map_get(flames[i].x + 1, flames[i].y))
            && (flame_map_get(flames[i].x, flames[i].y - 1) || flame_map_get(flames[i].x, flames[i].y + 1))) {

            flame.setTextureRect(sf::IntRect(offset, 64, 16, 16));
        } else if (flame_map_get(flames[i].x - 1, flames[i].y) && flame_map_get(flames[i].x + 1, flames[i].y)) {
            flame.setTextureRect(sf::IntRect(offset, 48, 16, 16));
        } else if (flame_map_get(flames[i].x, flames[i].y - 1) && flame_map_get(flames[i].x, flames[i].y + 1)) {
            flame.setTextureRect(sf::IntRect(64 + offset, 48, 16, 16));
        } else if (flame_map_get(flames[i].x + 1, flames[i].y)) {
            flame.setTextureRect(sf::IntRect(offset, 16, 16, 16));
        } else if (flame_map_get(flames[i].x, flames[i].y + 1)) {
            flame.setTextureRect(sf::IntRect(64 + offset, 16, 16, 16));
        } else if (flame_map_get(flames[i].x - 1, flames[i].y)) {
            flame.setTextureRect(sf::IntRect(offset, 32, 16, 16));
        } else if (flame_map_get(flames[i].x, flames[i].y - 1)) {
            flame.setTextureRect(sf::IntRect(64 + offset, 32, 16, 16));
        } else {
            flame.setTextureRect(sf::IntRect(offset, 64, 16, 16));
        }
        flame.setPosition(flames[i].x * 32, flames[i].y * 32);
        draw(flame);
    }
}

void GameWindow::drawPwrups() {
    sf::Sprite pwrup(objectTexture);
    pwrup.setScale(2, 2);
    for (int i = 0; i < pwrup_cnt; ++i) {
        pwrup.setPosition(pwrups[i].x * 32, pwrups[i].y * 32);
        switch (pwrups[i].type) {
            case PWRUP_POWER:
                pwrup.setTextureRect(sf::IntRect(0, 80, 16, 16));
                break;
            case PWRUP_SPEED:
                pwrup.setTextureRect(sf::IntRect(16, 80, 16, 16));
                break;
            case PWRUP_COUNT:
                pwrup.setTextureRect(sf::IntRect(32, 80, 16, 16));
                break;
            case PWRUP_KICK:
                pwrup.setTextureRect(sf::IntRect(48, 80, 16, 16));
                break;
            case PWRUP_REMOTE:
                pwrup.setTextureRect(sf::IntRect(64, 80, 16, 16));
                break;
            default:
                break;
        }
        draw(pwrup);
    }
}

void GameWindow::drawGame() {
    if (get_milliseconds() - lastKeyframe >= 200) {
        ++keyframe;
        lastKeyframe = get_milliseconds();
    }
    drawField();
    drawDynamites();
    drawFlames();
    drawPwrups();
    drawPlayers();
}
