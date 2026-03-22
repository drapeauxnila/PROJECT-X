#include <enet/enet.h>
#include <iostream>
#include "network_client.hpp"
#include "common.hpp"

uint8_t localWorld[WORLD_WIDTH * WORLD_HEIGHT];
bool isWorldLoaded = false;
uint32_t myNetId = 0;
std::map<uint32_t, ClientPlayer> otherPlayers;

static ENetHost* client = nullptr;
static ENetPeer* peer = nullptr;

bool InitNetwork() {
    if (enet_initialize() != 0) return false;
    client = enet_host_create(NULL, 1, 2, 0, 0);
    return client != nullptr;
}

void ConnectToServer(const char* ip) {
    ENetAddress address;
    enet_address_set_host(&address, ip);
    address.port = 17092;
    peer = enet_host_connect(client, &address, 2, 0);
}

void SendMovePacket(float x, float y) {
    if (!peer || peer->state != ENET_PEER_STATE_CONNECTED) return;
    PlayerMovePacket mp = { PACKET_PLAYER_MOVE, myNetId, x, y };
    ENetPacket* packet = enet_packet_create(&mp, sizeof(PlayerMovePacket), ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 0, packet);
}

void SendBlockEditPacket(int tileX, int tileY, uint8_t blockId) {
    if (!peer || peer->state != ENET_PEER_STATE_CONNECTED) return;
    BlockEditPacket bp = { PACKET_BLOCK_EDIT, myNetId, tileX, tileY, blockId };
    ENetPacket* packet = enet_packet_create(&bp, sizeof(BlockEditPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

void ProcessNetwork() {
    if (!client) return;
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "[CLIENT] Connected to server.\n";
                break;
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength > 0) {
                    uint8_t type = event.packet->data[0];
                    if (type == PACKET_WORLD_DATA && event.packet->dataLength >= sizeof(WorldDataPacket)) {
                        WorldDataPacket* wp = reinterpret_cast<WorldDataPacket*>(event.packet->data);
                        for (int i = 0; i < WORLD_WIDTH * WORLD_HEIGHT; i++) localWorld[i] = wp->tiles[i];
                        isWorldLoaded = true;
                    } else if (type == PACKET_PLAYER_JOIN && event.packet->dataLength >= sizeof(PlayerJoinPacket)) {
                        PlayerJoinPacket* jp = reinterpret_cast<PlayerJoinPacket*>(event.packet->data);
                        if (jp->isLocal) {
                            myNetId = jp->netId;
                        } else {
                            otherPlayers[jp->netId] = { jp->x, jp->y };
                        }
                    } else if (type == PACKET_PLAYER_MOVE && event.packet->dataLength >= sizeof(PlayerMovePacket)) {
                        PlayerMovePacket* mp = reinterpret_cast<PlayerMovePacket*>(event.packet->data);
                        if (mp->netId != myNetId) {
                            otherPlayers[mp->netId].x = mp->x;
                            otherPlayers[mp->netId].y = mp->y;
                        }
                    } else if (type == PACKET_PLAYER_DISCONNECT && event.packet->dataLength >= sizeof(PlayerDisconnectPacket)) {
                        PlayerDisconnectPacket* dp = reinterpret_cast<PlayerDisconnectPacket*>(event.packet->data);
                        otherPlayers.erase(dp->netId);
                    } else if (type == PACKET_BLOCK_EDIT && event.packet->dataLength >= sizeof(BlockEditPacket)) {
                        BlockEditPacket* bp = reinterpret_cast<BlockEditPacket*>(event.packet->data);
                        if (bp->tileX >= 0 && bp->tileX < WORLD_WIDTH && bp->tileY >= 0 && bp->tileY < WORLD_HEIGHT) {
                            localWorld[bp->tileY * WORLD_WIDTH + bp->tileX] = bp->blockId;
                        }
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                isWorldLoaded = false;
                otherPlayers.clear();
                myNetId = 0;
                break;
            case ENET_EVENT_TYPE_NONE: break;
        }
    }
}

void DisconnectNetwork() {
    if (peer) enet_peer_disconnect(peer, 0);
    if (client) {
        ENetEvent event;
        while (enet_host_service(client, &event, 1000) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
        }
        enet_host_destroy(client);
    }
    enet_deinitialize();
}
