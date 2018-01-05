#ifndef BOMBERMAN_PROTOCOL_H
#define BOMBERMAN_PROTOCOL_H

/*
 * client >> server (TCP)
 *
 * non null-terminated player name (23)
 */
#define JOIN_REQUEST 0x01u

/*
 * client << server (TCP)
 *
 * JOIN_RESPONSE_* (1)
 * if status == JOIN_RESPONSE_SUCCESS:
 *   player id (1)
 */
#define JOIN_RESPONSE 0x02u

#define JOIN_RESPONSE_SUCCESS 0x00u
#define JOIN_RESPONSE_BUSY 0x01u
#define JOIN_RESPONSE_FULL 0x02u

/*
 * client << server (TCP)
 *
 * player count (1)
 * for n = 0 to player count:
 *   player id (1)
 *   non null-terminated player name (23)
 * readiness (one bit for each player) (1)
 */
#define LOBBY_STATUS 0x07u

/*
 * client >> server (TCP)
 *
 * player id (1)
 */
#define KEEP_ALIVE 0x03u

/*
 * client >> server (TCP)
 *
 * player id (1)
 */
#define READY 0x04u

/*
 * client >> server (TCP)
 *
 * player id (1)
 * OR of INPUT_* (2)
 */
#define INPUT 0x05u

#define INPUT_UP 0x0001u
#define INPUT_DOWN 0x0002u
#define INPUT_LEFT 0x0004u
#define INPUT_RIGHT 0x0008u
#define INPUT_PLANT 0x0010u
#define INPUT_DETONATE 0x0020u
#define INPUT_PICK_UP 0x0040u
#define INPUT_THROW 0x0080u

#define INPUT_TAUNT 0x0200u
#define INPUT_DANCE 0x0400u
#define INPUT_LAUGH 0x0800u
#define INPUT_BONUS1 0x1000u
#define INPUT_BONUS2 0x2000u
#define INPUT_BONUS3 0x4000u
#define INPUT_BONUS4 0x8000u

/*
 * client >> server (TCP)
 *
 * player id (1)
 */
#define DISCONNECT 0x06u

/*
 * client << server (TCP)
 *
 * player count (1)
 * for n = 0 to player count:
 *   player id (1)
 *   non null-terminated player name (23)
 *   x position / 10 (2)
 *   y position / 10 (2)
 *   DIRECTION_* (1)
 * field width [0;255] (1)
 * field height [0;255] (1)
 * array of BLOCK_* (field width * field height)
 * (optional) dynamite timer in s, default 5 (1)
 * TODO: define default dynamite slide velocity
 * (optional) dynamite slide velocity / 10 in u/s [0.1;25.5], default . (1)
 */
#define GAME_START 0x08u

#define DIRECTION_UP 0x00u
#define DIRECTION_RIGHT 0x01u
#define DIRECTION_DOWN 0x02u
#define DIRECTION_LEFT 0x03u

#define BLOCK_EMPTY 0x00u
#define BLOCK_WALL 0x01u
#define BLOCK_BOX 0x02u

/*
 * client << server (TCP)
 *
 * update count (2)
 * for n = 0 to update count:
 *   x position (1)
 *   y position (1)
 *   BLOCK_* (1)
 */
#define MAP_UPDATE 0x09u

/*
 * client << server (TCP)
 *
 * timer in s (2)
 * dynamite count (1)
 * for n = 0 to dynamite count:
 *   x position (1)
 *   y position (1)
 * flame count (1)
 * for n = 0 to flame count:
 *   x position (1)
 *   y position (1)
 * powerup count (1)
 * for n = 0 to powerup count:
 *   x position (1)
 *   y position (1)
 *   PWRUP_* (1)
 * player count (1)
 * for n = 0 to player count:
 *   player id (1)
 *   dead? (1)
 *   if !dead:
 *     x position / 10 (2)
 *     y position / 10 (2)
 *     DIRECTION_* (1)
 *     OR of ACTIVE_PWRUP_* (1)
 *     power [1;255] (1)
 *     speed in u/s [1;255] (1)
 *     count [1;255] (1)
 */
#define OBJECTS 0x0Au

#define PWRUP_POWER 0x00u
#define PWRUP_SPEED 0x01u
#define PWRUP_REMOTE 0x02u
#define PWRUP_COUNT 0x03u
#define PWRUP_KICK 0x04u

#define ACTIVE_PWRUP_REMOTE 0x01u
#define ACTIVE_PWRUP_KICK 0x02u

/*
 * client << server (TCP)
 *
 * player count (1)
 * for n = 0 to player count:
 *   player id #n (1)
 */
#define GAME_OVER 0x0Bu

#endif //BOMBERMAN_PROTOCOL_H
