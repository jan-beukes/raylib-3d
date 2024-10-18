#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdint.h>

#include "custom_draw.h"

#define MAX(X, Y) (X) > (Y) ? (X) : (Y)

#define MOVE_SPEED 20
#define TURN_SPEED 250
#define MOUSE_SENS 0.1
#define TILE_SIZE 10
#define PLAYER_RADIUS 2

typedef struct Map {
    int **walls;
    int wall_count;
    Rectangle *colliders;
    
    Vector2 origin; // TOP LEFT!!!!!!! in x,z plane!!!!!
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

char *debug_msg = "Chill";

void init_map(Map *map, Image image, Vector2 origin) {
    map->width = image.width;
    map->height = image.height;
    map->origin = origin;

    // Walls array
    map->walls = calloc(sizeof(int*), image.height);
    for (int i=0; i < image.width; i++){
        map->walls[i] = calloc(sizeof(int), image.width);
    }
    
    Color *colors= LoadImageColors(image);
    int wall_count = 0;
    for (int i=0; i < image.height; i++){
        for (int j=0; j < image.width; j++){
            int value = texture_from_blue(colors[i*image.height + j].b);
            if (value != EMPTY) {
                map->walls[i][j] = texture_from_blue(colors[i*image.height + j].b);
                wall_count++;
            }
        }
    }
    map->wall_count = wall_count;
    map->colliders = calloc(wall_count, sizeof(Rectangle));
    int collider_index = 0;
    for (int i=0; i < image.height; i++){
        for (int j=0; j < image.width; j++){
            if (map->walls[i][j] != 0) {
                map->colliders[collider_index++] = (Rectangle){j*TILE_SIZE, i*TILE_SIZE, TILE_SIZE, TILE_SIZE};
            }
        }
    }
}

void player_movement(Camera *camera, Map map, float dt) {

    // Vector3 looking = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    // float angle = RAD2DEG*atan2(-looking.z, looking.x);
    // printf("rot: %f\n", angle);

    // Movement
    Vector2 dir = {IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
                   IsKeyDown(KEY_W) - IsKeyDown(KEY_S)};
    dir = Vector2Normalize(dir);
    float rx = MOVE_SPEED * dir.x * dt;
    float ry = MOVE_SPEED * dir.y * dt;

    // Collision???
    Vector2 forward = {GetCameraForward(camera).x, -GetCameraForward(camera).z};
    Vector2 right = {GetCameraRight(camera).x, -GetCameraRight(camera).z};
    Vector2 world_displacement = Vector2Add(Vector2Scale(forward, ry) , Vector2Scale(right, rx));

    for (int i =0; i < map.wall_count; i++) {
        Vector2 pos = {camera->position.x, -camera->position.z};
        if (CheckCollisionCircleRec(pos, PLAYER_RADIUS, map.colliders[i])){
            debug_msg = "collision";
            break;
        } else {
            debug_msg = "Chill";
        }
    }

    float mouse_move_x = GetMouseDelta().x * MOUSE_SENS;
    float key_move_x = (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * TURN_SPEED * dt;
    float rotation = fabs(mouse_move_x) > fabs(key_move_x) ? mouse_move_x : key_move_x;
    UpdateCameraPro(camera, (Vector3){ry, rx, 0}, (Vector3){rotation, 0, 0}, 0);
}

void draw_map(Model floor, Map map, Texture *wall_textures) {
    // floor/ceiling
    int width = map.width, length = map.height;
    Vector2 top_left_pos = map.origin;

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
    // Load Mario
    FILE *mario_file = fopen("res/mario.png", "r");
    if (mario_file == NULL) perror("Dude");
    if (fseek(mario_file, 0, SEEK_END)) perror("Dude");
    long size = ftell(mario_file);
    fseek(mario_file, 0, SEEK_SET);
    unsigned char *mario_data = malloc(sizeof(unsigned char) * size);
    fread(mario_data, 1, size, mario_file);


    InitWindow(1280, 720, "Gaming");

    Camera3D camera = {.position = {0, 2.0, 0},
                       .target = {0, 2, -1},
                       .up = {0.0, 1.0, 0.0},
                       .projection = CAMERA_PERSPECTIVE,
                       .fovy = 60.0};

    // Texture and stuff
    Texture2D floor_texture = LoadTexture("res/floor.png");
    Texture wall1_texture = LoadTexture("res/wall1.png");
    Texture wall2_texture = LoadTexture("res/wall2.png");
    Texture mario = LoadTextureFromImage(LoadImageFromMemory(".png", mario_data, size));
    
    Texture wall_textures[2] = {wall1_texture, mario};
    Image map_image = LoadImage("res/map.png");
    Map map;
    init_map(&map, map_image, (Vector2){-map.width*TILE_SIZE/2,-map.height*TILE_SIZE/2});

    BoundingBox box = {(Vector3){0,0,0},{2, 2, 2}};

    Mesh plane_mesh = gen_mesh_plane_tiled(map.width*TILE_SIZE, map.height*TILE_SIZE, map.width, map.height);
    Model plane_model = LoadModelFromMesh(plane_mesh);
    plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = floor_texture;

    DisableCursor();
    while (!WindowShouldClose()) {
        SetWindowTitle(TextFormat("%dfps",GetFPS()));
        float dt = GetFrameTime();
        player_movement(&camera, map, dt);

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);


            // Objects
            DrawBoundingBox(box, BLUE);

            // Map
            draw_map(plane_model, map, wall_textures);

            EndMode3D();
            DrawText(debug_msg, 10, 10, 32, RAYWHITE);

        }
        EndDrawing();
    }

    UnloadModel(plane_model);
    UnloadTexture(floor_texture);
    CloseWindow();

    return 0;
}