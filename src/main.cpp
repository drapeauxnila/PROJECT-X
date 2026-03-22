#include "raylib.h"
#include "network_client.hpp"
#include "common.hpp"

float playerX = TILE_SIZE * 5;
float playerY = TILE_SIZE * 5;
float playerVelocityY = 0.0f;
bool isGrounded = false;

const float playerSpeed = 200.0f; 
const float GRAVITY = 1000.0f;
const float JUMP_SPEED = -400.0f;

bool CheckTileCollision(float px, float py, const uint8_t* worldMap) {
    if (px < 0 || py < 0 || px + PLAYER_SIZE > WORLD_WIDTH * TILE_SIZE || py + PLAYER_SIZE > WORLD_HEIGHT * TILE_SIZE) return true;
    
    int startX = (int)(px / TILE_SIZE);
    int endX = (int)((px + PLAYER_SIZE - 0.1f) / TILE_SIZE);
    int startY = (int)(py / TILE_SIZE);
    int endY = (int)((py + PLAYER_SIZE - 0.1f) / TILE_SIZE);

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            if (worldMap[y * WORLD_WIDTH + x] == 1) { 
                return true;
            }
        }
    }
    return false;
}

int main() {
    if (!InitNetwork()) return -1;
    ConnectToServer("127.0.0.1");

    const int screenWidth = 800;
    const int screenHeight = 600;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "MyGrowtopiaClone - Raylib C++ Client");
    SetTargetFPS(60);

    Camera2D camera = { 0 };
    camera.target = (Vector2){ playerX, playerY };
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    float syncTimer = 0.0f;

    while (!WindowShouldClose()) {
        ProcessNetwork();
        float dt = GetFrameTime();

        if (isWorldLoaded && myNetId != 0) {
            bool moved = false;
            float oldX = playerX;
            float oldY = playerY;

            if (IsKeyDown(KEY_A)) { playerX -= playerSpeed * dt; moved = true; }
            if (IsKeyDown(KEY_D)) { playerX += playerSpeed * dt; moved = true; }

            // X-Axis Collision
            if (CheckTileCollision(playerX, oldY, localWorld)) {
                playerX = oldX;
            }

            // Y-Axis Physics
            playerVelocityY += GRAVITY * dt;
            playerY += playerVelocityY * dt;

            // Y-Axis Collision
            isGrounded = false;
            if (CheckTileCollision(playerX, playerY, localWorld)) {
                if (playerVelocityY > 0) { 
                    playerY = (float)(((int)(playerY + PLAYER_SIZE) / TILE_SIZE) * TILE_SIZE) - PLAYER_SIZE;
                    isGrounded = true;
                } else if (playerVelocityY < 0) {
                    playerY = (float)(((int)playerY / TILE_SIZE + 1) * TILE_SIZE);
                }
                playerVelocityY = 0;
            }
            
            // Jumping
            if (isGrounded && IsKeyPressed(KEY_W)) {
                playerVelocityY = JUMP_SPEED;
            }

            if (oldY != playerY) moved = true;
            
            syncTimer += dt;
            if (moved || syncTimer > 0.1f) {
                if (oldX != playerX || oldY != playerY || syncTimer > 0.1f) {
                    SendMovePacket(playerX, playerY);
                    syncTimer = 0.0f;
                }
            }
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                int tx = (int)(mouseWorldPos.x / TILE_SIZE);
                int ty = (int)(mouseWorldPos.y / TILE_SIZE);
                
                if (tx >= 0 && tx < WORLD_WIDTH && ty >= 0 && ty < WORLD_HEIGHT) {
                    float dx = (tx * TILE_SIZE + TILE_SIZE/2) - (playerX + PLAYER_SIZE/2);
                    float dy = (ty * TILE_SIZE + TILE_SIZE/2) - (playerY + PLAYER_SIZE/2);
                    if ((dx*dx + dy*dy) < (150.0f * 150.0f)) { 
                        uint8_t newBlock = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ? 0 : 1; 
                        if (localWorld[ty * WORLD_WIDTH + tx] != newBlock) {
                            localWorld[ty * WORLD_WIDTH + tx] = newBlock; 
                            SendBlockEditPacket(tx, ty, newBlock);
                        }
                    }
                }
            }
        }

        camera.target = (Vector2){ playerX, playerY };

        BeginDrawing();
        ClearBackground(SKYBLUE);

        if (isWorldLoaded) {
            BeginMode2D(camera);
            
            // Draw Tiles
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                for (int x = 0; x < WORLD_WIDTH; x++) {
                    if (localWorld[y * WORLD_WIDTH + x] == 1) { 
                        DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, DARKBROWN);
                        DrawRectangleLines(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, BLACK);
                    }
                }
            }

            // Draw Other Players globally
            for (auto const& [nid, p] : otherPlayers) {
                DrawRectangle(p.x, p.y, PLAYER_SIZE, PLAYER_SIZE, RED);
                DrawRectangleLines(p.x, p.y, PLAYER_SIZE, PLAYER_SIZE, BLACK);
                DrawText(TextFormat("Player %d", nid), p.x - 10, p.y - 15, 10, WHITE);
            }

            // Draw Local Player
            if (myNetId != 0) {
                DrawRectangle(playerX, playerY, PLAYER_SIZE, PLAYER_SIZE, GREEN);
                DrawRectangleLines(playerX, playerY, PLAYER_SIZE, PLAYER_SIZE, BLACK);
                DrawText("YOU", playerX, playerY - 15, 10, GREEN);
            }

            EndMode2D();
            
            DrawText("Client: Online & Fully Synced. WASD to Move.", 10, 10, 20, RAYWHITE);
        } else {
            DrawText("Connecting to Game Server...", 10, 10, 20, RAYWHITE);
        }

        EndDrawing();
    }

    DisconnectNetwork();
    CloseWindow();
    return 0;
}
