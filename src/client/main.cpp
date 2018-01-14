#include <SFML/Window.hpp>
#include <arpa/inet.h>

extern "C" {
    #include "../common/include/reader.h"
    #include "../common/include/utils.h"
    #include "include/packet.h"
}

#define POLL_RATE 60

#define STATE_LOBBY 1
#define STATE_IN_PROGRESS 2

static int fd;
static int state = STATE_LOBBY;

static void get_fd(char *ip, char *port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;

    if (!inet_pton(AF_INET, ip, &addr.sin_addr)) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        exit(-1);
    }
    addr.sin_port = htons((uint16_t) strtol(port, nullptr, 10));

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr, "Failed to connect to server: %s\n", strerror(errno));
        exit(-1);
    }
}

static void *keep_alive_thread(void *arg) {
    uint8_t id = *(uint8_t *) arg;
    for (;;) {
        if (send_keep_alive(fd, id) == -1) {
            fprintf(stderr, "Failed to send message to server: %s\n", strerror(errno));
            exit(-1);
        }
        sleep(8);
    }
    pthread_exit(nullptr);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <address> <port> <nickname>\n", argv[0]);
        return -1;
    }

    get_fd(argv[1], argv[2]);
    reader_t reader;
    reader_init(&reader, fd);

    if (send_join_request(fd, argv[3]) == -1) {
        fprintf(stderr, "Failed to send message to server: %s\n", strerror(errno));
        return -1;
    }

    uint8_t id;
    char *response = get_bytes(&reader, 2);
    if (response[0] == JOIN_RESPONSE) {
        switch (response[1]) {
            case JOIN_RESPONSE_SUCCESS:
                id = (uint8_t) get_bytes(&reader, 1)[0];
                pthread_t thread;
                pthread_create(&thread, nullptr, keep_alive_thread, &id);
                puts("Connection established");
                break;
            case JOIN_RESPONSE_BUSY:
                fprintf(stderr, "A game is already in progress\n");
                return -1;
            case JOIN_RESPONSE_FULL:
                fprintf(stderr, "Server is full\n");
                return -1;
            default:
                break;
        }
    }

    sf::Window window(sf::VideoMode(800, 600), "Bomberman");
    window.setVerticalSyncEnabled(true);
    sf::Event event;

    uint16_t input = 0;
    uint16_t prev_input = 0;

    while (window.isOpen()) {
        time_t timestamp = time(nullptr);
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    switch (event.key.code) {
                        case sf::Keyboard::A:
                            input |= INPUT_LEFT;
                            break;
                        case sf::Keyboard::W:
                            input |= INPUT_UP;
                            break;
                        case sf::Keyboard::D:
                            input |= INPUT_RIGHT;
                            break;
                        case sf::Keyboard::S:
                            input |= INPUT_DOWN;
                            break;
                        case sf::Keyboard::Space:
                            input |= INPUT_PLANT;
                            break;
                        case sf::Keyboard::F:
                            input |= INPUT_DETONATE;
                            break;
                        case sf::Keyboard::E:
                            input |= INPUT_PICK_UP;
                            break;
                        default:
                            break;
                    }
                    break;
                case sf::Event::KeyReleased:
                    switch (event.key.code) {
                        case sf::Keyboard::A:
                            input &= ~INPUT_LEFT;
                            break;
                        case sf::Keyboard::W:
                            input &= ~INPUT_UP;
                            break;
                        case sf::Keyboard::D:
                            input &= ~INPUT_RIGHT;
                            break;
                        case sf::Keyboard::S:
                            input &= ~INPUT_DOWN;
                            break;
                        case sf::Keyboard::Space:
                            input &= ~INPUT_PLANT;
                            break;
                        case sf::Keyboard::F:
                            input &= ~INPUT_DETONATE;
                            break;
                        case sf::Keyboard::E:
                            input &= ~INPUT_PICK_UP;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
        if (input != prev_input) {
            if (state == STATE_IN_PROGRESS) {
                send_input(fd, id, input);
            } else if (state == STATE_LOBBY && input & INPUT_PLANT) {
                send_ready(fd, id);
            }
            prev_input = input;
        }
        usleep((useconds_t) MAX((timestamp + 1000 / POLL_RATE - time(nullptr)) * 1000, 0));
    }

    if (send_disconnect(fd, id) == -1) {
        fprintf(stderr, "Failed to send message to server: %s\n", strerror(errno));
        return -1;
    }
    close(fd);

    return 0;
}
