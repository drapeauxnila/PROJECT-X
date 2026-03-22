#pragma once
#include <cstdint>
#include <map>

struct ClientPlayer {
    float x, y;
};

bool InitNetwork();
void ConnectToServer(const char* ip);
void ProcessNetwork();
void SendMovePacket(float x, float y);
void SendBlockEditPacket(int tileX, int tileY, uint8_t blockId);
void DisconnectNetwork();

extern uint8_t localWorld[];
extern bool isWorldLoaded;
extern uint32_t myNetId;
extern std::map<uint32_t, ClientPlayer> otherPlayers;
