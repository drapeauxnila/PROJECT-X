#pragma once
#include <cstdint>

constexpr int WORLD_WIDTH = 100;
constexpr int WORLD_HEIGHT = 60;
constexpr int TILE_SIZE = 32;
constexpr int PLAYER_SIZE = 24;

enum PacketType : uint8_t {
    PACKET_WORLD_DATA = 1,
    PACKET_PLAYER_JOIN,
    PACKET_PLAYER_MOVE,
    PACKET_PLAYER_DISCONNECT,
    PACKET_BLOCK_EDIT
};

struct BlockEditPacket {
    uint8_t type = PACKET_BLOCK_EDIT;
    uint32_t netId;
    int tileX, tileY;
    uint8_t blockId;
};

struct WorldDataPacket {
    uint8_t type = PACKET_WORLD_DATA;
    uint8_t tiles[WORLD_WIDTH * WORLD_HEIGHT];
};

struct PlayerJoinPacket {
    uint8_t type = PACKET_PLAYER_JOIN;
    uint32_t netId;
    float x, y;
    bool isLocal;
};

struct PlayerMovePacket {
    uint8_t type = PACKET_PLAYER_MOVE;
    uint32_t netId;
    float x, y;
};

struct PlayerDisconnectPacket {
    uint8_t type = PACKET_PLAYER_DISCONNECT;
    uint32_t netId;
};
