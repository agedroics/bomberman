#include <SFML/Window.hpp>
#include <arpa/inet.h>

extern "C" {
    #include "packet.h"
}

#define FRAMERATE 60

#define STATE_LOBBY 1
#define STATE_IN_PROGRESS 2

static int fd;
static uint8_t id;
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

static void fail_send(void) {
    fprintf(stderr, "Failed to send message to server: %s\n", strerror(errno));
    exit(-1);
}

static void *keep_alive_thread(void *arg) {
    uint8_t id = *(uint8_t *) arg;
    for (;;) {
        if (send_keep_alive(fd, id) == -1) {
            fail_send();
        }
        sleep(8);
    }
    pthread_exit(nullptr);
}

static void *data_thread(void *arg) {
    auto *reader = (reader_t *) arg;
    char *data;
    int result;

    while ((data = get_bytes(reader, 1))) {
        switch (*data) {
            case LOBBY_STATUS:
                pthread_mutex_lock(&state_lock);
                result = parse_lobby_status(reader);
                pthread_mutex_unlock(&state_lock);
                if (result == -1) {
                    fprintf(stderr, "Failed to read LOBBY_STATUS packet\n");
                    exit(-1);
                }
                break;
            case GAME_START:
                pthread_mutex_lock(&state_lock);
                result = parse_game_start(reader);
                pthread_mutex_unlock(&state_lock);
                if (result == -1) {
                    fprintf(stderr, "Failed to read GAME_START packet\n");
                    exit(-1);
                }
                if (send_ready(fd, id) == -1) {
                    fail_send();
                }
                puts("STARTING GAME");
                state = STATE_IN_PROGRESS;
                break;
            case MAP_UPDATE:
                pthread_mutex_lock(&state_lock);
                result = parse_map_update(reader);
                pthread_mutex_unlock(&state_lock);
                if (result == -1) {
                    fprintf(stderr, "Failed to read MAP_UPDATE packet\n");
                    exit(-1);
                }
                break;
            case OBJECTS:
                pthread_mutex_lock(&state_lock);
                result = parse_objects(reader);
                pthread_mutex_unlock(&state_lock);
                if (result == -1) {
                    fprintf(stderr, "Failed to read OBJECTS packet\n");
                    exit(-1);
                }
                break;
            case GAME_OVER:
                pthread_mutex_lock(&state_lock);
                result = parse_game_over(reader);
                pthread_mutex_unlock(&state_lock);
                if (result == -1) {
                    fprintf(stderr, "Failed to read GAME_OVER packet\n");
                    exit(-1);
                }
                if (field) {
                    free(field);
                    field = nullptr;
                }
                puts("LOBBY STAGE");
                state = STATE_LOBBY;
                break;
            default:
                break;
        }
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

    char *response = get_bytes(&reader, 2);
    if (response[0] == JOIN_RESPONSE) {
        switch (response[1]) {
            case JOIN_RESPONSE_SUCCESS:
                id = (uint8_t) get_bytes(&reader, 1)[0];
                pthread_t thread;
                if ((errno = pthread_create(&thread, nullptr, keep_alive_thread, &id))) {
                    fprintf(stderr, "Failed to create keepalive thread: %s\n", strerror(errno));
                    return -1;
                }
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

    pthread_t thread;
    if ((errno = pthread_create(&thread, nullptr, data_thread, &reader))) {
        fprintf(stderr, "Failed to create data thread: %s\n", strerror(errno));
        return -1;
    }

    puts("LOBBY STAGE");

    sf::Window window(sf::VideoMode(800, 600), "Bomberman");
    window.setVerticalSyncEnabled(true);
    sf::Event event;

    uint16_t prev_input = 0;

    while (window.isOpen()) {
        time_t timestamp = time(nullptr);
        uint16_t input = 0;
        if (window.hasFocus()) {
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
        }
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                default:
                    break;
            }
        }
        if (input != prev_input) {
            if (state == STATE_IN_PROGRESS) {
                if (send_input(fd, id, input) == -1) {
                    fail_send();
                }
            } else if (state == STATE_LOBBY && input & INPUT_PLANT) {
                if (send_ready(fd, id) == -1) {
                    fail_send();
                }
            }
            prev_input = input;
        }
        usleep((useconds_t) MAX((timestamp + 1000 / FRAMERATE - time(nullptr)) * 1000, 0));
    }

    if (send_disconnect(fd, id) == -1) {
        fail_send();
    }
    close(fd);

    return 0;
}
