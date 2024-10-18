#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>
#include <rlgl.h>
#include <stdio.h>

#include "custom_draw.h"

#define MAX(X, Y) (X) > (Y) ? (X) : (Y)

#define MOVE_SPEED 20
#define TURN_SPEED 250
#define MOUSE_SENS 0.1
#define TILE_SIZE 10

typedef struct Map {
    int **walls;
    Rectangle **colliders;
    int width;
    int height;
} Map;

enum TextureIndex {
    EMPTY,
    BRICK,
    OTHER
};

int texture_from_blue(int b) {
    switch (b){
        case 0: return EMPTY;
        case 255: return BRICK;
        case 100: return OTHER;
    }
}

void init_map(Map *map, Image image) {
    map->width = image.width;
    map->height = image.height;

    // Walls array
    map->walls = calloc(sizeof(int*), image.height);
    map->colliders = calloc(sizeof(Rectangle*), image.height);
    for (int i=0; i < image.width; i++){
        map->walls[i] = calloc(sizeof(int), image.width);
        map->colliders[i] = calloc(sizeof(Rectangle), image.width);

    }
    
    Color *colors= LoadImageColors(image);
    for (int i=0; i < image.height; i++){
        for (int j=0; j < image.width; j++){
            map->walls[i][j] = texture_from_blue(colors[i*image.height + j].b);
            map->colliders[i][j] = (Rectangle){i*TILE_SIZE, j*TILE_SIZE, TILE_SIZE, TILE_SIZE};
        }
    }
}

void player_movement(Camera *camera, Map map, float dt) {
    //printf("%f %f\n", camera->position.x, camera->position.z);
    Vector3 looking = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    printf("target: (%f %f)\n", looking.x, looking.z);

    Vector2 dir = {IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
                   IsKeyDown(KEY_W) - IsKeyDown(KEY_S)};
    dir = Vector2Normalize(dir);
    float rx = MOVE_SPEED * dir.x * dt;
    float ry = MOVE_SPEED * dir.y * dt;

    float mouse_move_x = GetMouseDelta().x * MOUSE_SENS;
    float key_move_x =
        (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * TURN_SPEED * dt;
    float rotation =
        fabs(mouse_move_x) > fabs(key_move_x) ? mouse_move_x : key_move_x;
    UpdateCameraPro(camera, (Vector3){ry, rx, 0}, (Vector3){rotation, 0, 0}, 0);
}

void draw_map(Model floor, Map map, Texture *wall_textures, Vector2 top_left_pos) {
    // floor/ceiling
    int width = map.width, length = map.height;

    DrawModel(floor, (Vector3){top_left_pos.x+TILE_SIZE*width/2, 0, top_left_pos.y+TILE_SIZE*length/2}, 1, WHITE);
    DrawModelEx(floor, (Vector3){top_left_pos.x+TILE_SIZE*width/2, 4, top_left_pos.y+TILE_SIZE*length/2}, (Vector3){0, 0, 1},
                180, Vector3One(), WHITE);

    // Walls
    for (int i = 1; i <= length; i++){
        for (int j = 1; j <= width; j++){
            if (map.walls[i-1][j-1] > 0){
                int wall_tex = map.walls[i-1][j-1] - 1;

                Vector2 pos = Vector2Add(top_left_pos, (Vector2){j*TILE_SIZE - TILE_SIZE/2, i*TILE_SIZE - TILE_SIZE/2});
                draw_textured_cube(wall_textures[wall_tex], (Vector3){pos.x, 2, pos.y}, TILE_SIZE, 4, TILE_SIZE, WHITE);
            }
        }
    }

}

int main() {
    InitWindow(1280, 720, "Gaming");

    Camera3D camera = {.position = {80, 2.0, 80},
                       .target = {30, 2, -1},
                       .up = {0.0, 1.0, 0.0},
                       .projection = CAMERA_PERSPECTIVE,
                       .fovy = 60.0};

    // Texture and stuff
    Texture2D floor_texture = LoadTexture("res/floor.png");
    Texture wall1_texture = LoadTexture("res/wall1.png");
    Texture wall2_texture = LoadTexture("res/wall2.png");
    Texture mario = LoadTexture("res/mario.png");
    
    Texture wall_textures[2] = {wall1_texture, wall2_texture};
    Image map_image = LoadImage("res/map.png");
    Map map;
    init_map(&map, map_image);

    Mesh plane_mesh = gen_mesh_plane_tiled(map.width*TILE_SIZE, map.height*TILE_SIZE, map.width, map.height);
    Model plane_model = LoadModelFromMesh(plane_mesh);
    plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = floor_texture;

    DisableCursor();
    while (!WindowShouldClose()) {
        SetWindowTitle(TextFormat("%dfps",GetFPS()));
        float dt = GetFrameTime();

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);

            player_movement(&camera, dt);

            // Objects
            DrawCube((Vector3){0, 0.5, 0}, 1, 1, 1, RED);
            DrawCubeWires((Vector3){0, 0.5, 0}, 1, 1, 1, BLACK);
            for (float i = 0; i < 2 * PI; i += PI / 6) {
                DrawBillboard(camera, mario,
                              (Vector3){cos(i) * 10, 2, sin(i) * 10}, 4, WHITE);
            }

            // Map
            draw_map(plane_model, map, wall_textures, (Vector2) {-map.width*TILE_SIZE/2,-map.height*TILE_SIZE/2});

            EndMode3D();
        }
        EndDrawing();
    }

    UnloadModel(plane_model);
    UnloadTexture(floor_texture);
    CloseWindow();

    return 0;
}