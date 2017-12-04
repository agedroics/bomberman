#include <iostream>
#include <SFML/Window.hpp>

int main() {
    sf::Window window(sf::VideoMode(800, 600), "Main Window");
    window.setVerticalSyncEnabled(true);
    sf::Event event;

    while (window.isOpen()) {
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
    }

    return 0;
}