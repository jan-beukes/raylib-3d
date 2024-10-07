#include <stdio.h>
#include "custom_plane.h"
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>
#include <rcamera.h>

int main() {

    InitWindow(1280, 720, "Gaming");
    SetTargetFPS(120);
    
    Camera3D camera = {
        .position = {0.0, 2.0, 2},
        .target = {0, 2, 1},
        .up = {0.0, 1.0, 0.0},
        .projection = CAMERA_PERSPECTIVE,
        .fovy =  60.0
    };

    // Textured floor 
    Texture2D floor_texture = LoadTexture("1.png");
    Mesh plane_mesh = GenMeshPlaneTiled(100, 100, 10, 10);
    Model plane_model = LoadModelFromMesh(plane_mesh);
    plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = floor_texture;
    
    Texture mario = LoadTexture("mario.png");

    DisableCursor();
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Vector2 dir = {IsKeyDown(KEY_D) - IsKeyDown(KEY_A), IsKeyDown(KEY_W) - IsKeyDown(KEY_S)};
        dir = Vector2Normalize(dir);
        float rx = 10 * dir.x * dt;
        float ry = 10 * dir.y * dt;
        float mouse_delta_x = GetMouseDelta().x * 0.1;
        UpdateCameraPro(&camera,(Vector3){ry, rx, 0}, (Vector3){mouse_delta_x, 0 , 0}, 0);

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);

            // Objects
            DrawCube((Vector3){0, 0.5, 0}, 1, 1, 1, RED); 
            DrawCubeWires((Vector3){0, 0.5, 0}, 1, 1, 1, BLACK); 
            for (float i = 0; i < 2*PI; i+=PI/6) {
                DrawBillboard(camera, mario, (Vector3) {cos(i)*10,2,sin(i)*10}, 4, WHITE);
            }

            // Wall and floor
            DrawModel(plane_model, (Vector3){0, 0, 0}, 1, WHITE);
            DrawModelEx(plane_model, (Vector3){0, 4, 0},(Vector3){0,0,1}, 180, Vector3One(),WHITE);
            EndMode3D();
        }
        EndDrawing();
    }

    UnloadModel(plane_model); 
    UnloadTexture(floor_texture); 
    CloseWindow();

    return 0;
}
