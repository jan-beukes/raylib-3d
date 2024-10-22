#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <rcamera.h>
#include "perlin.h"

#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)
#define MIN(X, Y) (X) < (Y) ? (X) : (Y) 
#define MAX(X, Y) (X) > (Y) ? (X) : (Y)

#define GRAVITY 100.0
#define JUMP 60

typedef struct Player {
    float height;
    Vector3 *position; // center y
    Camera3D *camera;
    float vel_y;
} Player;

typedef struct Terrain {
    Model *model;
    Mesh *mesh;
    int width;
    int length;

} Terrain;

typedef struct Block {
    Vector3 pos;
    Vector3 size;

} Block;

char *debug;
int jump_force = JUMP;

Image darken_image_from_noise(Image noise_image, Image image) {
    ImageResize(&image, noise_image.width, noise_image.height);

    Color *noise_pixels = LoadImageColors(noise_image);
    Color *image_pixels = LoadImageColors(image);
    Color *data = calloc(image.width*image.height, sizeof(Color));
    int min_color_value = 30;

    for (int y = 0; y < image.height; y++){
        for (int x = 0; x < image.width; x++){
            Color image_color = image_pixels[y*image.width + x];
            float noise_value = noise_pixels[y*image.width + x].r / 255.0;
            Color new_color = {MIN(min_color_value + image_color.r*noise_value, 255),
                               MIN(min_color_value + image_color.g*noise_value, 255),
                               MIN(min_color_value + image_color.b*noise_value, 255),
                               255};
            data[y*image.width + x] = new_color;
        }
    }
    Image new = {
        .data = data, 
        .width=image.width, 
        .height=image.height,
        .mipmaps=1,
        .format=PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    return new;

}

void player_move_position(Player *player, Vector3 move) {
    *player->position = Vector3Add(*player->position, move);
    player->camera->target = Vector3Add(player->camera->target, move);
}

float get_terrain_height(float x, float z, const float* vertices, int rows, int cols, float cell_size) {
    // Find grid cell indices from position
    int cell_x = floor(x / cell_size);
    int cell_z = floor(z / cell_size);

    // handle out of bounds
    if (cell_x >= cols - 1 || cell_z >= rows - 1 || cell_x < 0 || cell_z < 0) {
        return -1;
    }

    // Indiced for corners of the grid cell
    int vertex_index_top_left = (cell_z * (cols - 1) + cell_x) * 18;  // Step of 18 floats for each cell
    int vertex_index_bottom_left = vertex_index_top_left + 3;          // Offset to the bottom-left vertex
    int vertex_index_top_right = vertex_index_top_left + 6;            // Offset to the top-right vertex
    int vertex_index_bottom_right = vertex_index_top_left + 15;        // Offset to the bottom-right vertex

    // Corner heights of the cell
    float h1 = vertices[vertex_index_top_left + 1];     // Height at top-left
    float h2 = vertices[vertex_index_bottom_left + 1];  // Height at bottom-left
    float h3 = vertices[vertex_index_top_right + 1];    // Height at top-right
    float h4 = vertices[vertex_index_bottom_right + 1]; // Height at bottom-right

    // average height
    float height = (h1 + h2 + h3 + h4) / 4.0f;

    return height;
}

void move_player(Player *player, Terrain terrain, float dt, float dude_speed) {
    float speed;
    bool floor = false;
    if (IsKeyDown(KEY_LEFT_SHIFT)){
        speed = dude_speed*4;
    }else {
        speed = dude_speed;
    }

    Vector3 rot = {GetMouseDelta().x*0.1, GetMouseDelta().y*0.1, 0};
    
    Vector3 dr = {(IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * speed * dt,
                (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * speed * dt, 0};

    UpdateCameraPro(player->camera, dr, rot, 0); 

    // Handle Terrain collision
    Vector3 player_pos = Vector3Transform(*player->position, MatrixInvert(terrain.model->transform));
    float mesh_height = get_terrain_height(player_pos.x, player_pos.z, terrain.mesh->vertices,
                                    terrain.length, terrain.width, 1);
    
    // subtract epsilon for more lenient jumping
    float epsilon = 1;
    floor = mesh_height != -1 && player_pos.y - player->height - epsilon <= mesh_height; 
    char *text = floor == 1 ? "true" : "false";
    sprintf(debug, "%s", text);

    if (mesh_height != -1 && player_pos.y - player->height <= mesh_height) {
        float y_move = mesh_height + player->height - player->position->y; 
        player_move_position(player,(Vector3){0, y_move, 0});
        player->vel_y = 0;
    } else {
        player->vel_y -= GRAVITY*dt;
    }

    // Jump
    if (IsKeyPressed(KEY_J)) {
        jump_force = jump_force == JUMP ? JUMP*3 : JUMP;
    }
    if (floor && IsKeyPressed(KEY_SPACE)) {
        player->vel_y = jump_force;
    }


    player_move_position(player, (Vector3){0,player->vel_y*dt, 0});
    
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    debug = malloc(255*sizeof(char));

    InitWindow(screenWidth, screenHeight, "EPIC MAN");
    //SetTargetFPS(120);
    rlDisableBackfaceCulling();

    // Create the mesh
    int width = 400, length = 400;
    int max_height = 75;
    float scale = 1;
    Image image;
    Mesh plane;
    Model model; 

    image = my_perlin_image(width, length, GetRandomValue(0, 10000), GetRandomValue(0, 10000), 2, 2, 0.5, 6);
    plane = GenMeshHeightmap(image, (Vector3){width, max_height, length});
    model = LoadModelFromMesh(plane);
    
    Image mario = LoadImage("res/mario.png");
    mario = darken_image_from_noise(image, mario);
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(mario);    
    model.transform = MatrixTranslate(-width*scale/2, 0, -length*scale/2);

    Terrain terrain = {
        .model = &model,
        .mesh = &plane,
        .width = width,
        .length = length
    };

    Block blocks[255];
    int count = 0;

    Texture color_map = LoadTextureFromImage(image);

    BoundingBox terrain_box = GetModelBoundingBox(model);

    Camera camera = {0};
    camera.position = (Vector3){0.0f, max_height, -1.0f}; // Forward vector is zero if target (x,z) is same
    camera.target = (Vector3){0.0f,  max_height/2, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    // camera.type = CAMERA_PERSPECTIVE;

    Player player = {0};
    player.camera = &camera;
    player.height = 8;
    player.position = &camera.position;

    DisableCursor();
    const float dude_speed = 10;
    float speed;
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Player
        move_player(&player, terrain, dt, dude_speed);

        Ray ray;
        ray.position = camera.position;
        ray.direction = GetCameraForward(&camera);

        RayCollision collision = GetRayCollisionMesh(ray, *terrain.mesh, terrain.model->transform);

        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && collision.hit){
            blocks[count++] = (Block){collision.point, (Vector3){4, 4, 4}};
        }

        BeginDrawing();
        {
            ClearBackground(SKYBLUE);
            BeginMode3D(camera);
            DrawModel(model, Vector3Zero(), 1, WHITE);

            DrawCube(collision.point, 4, 4, 4, RED);
            for (int i = 0; i < count; i++) {
                DrawCubeV(blocks[i].pos, blocks[i].size, RED);
            }
            EndMode3D();
            
            DrawText(TextFormat("Position (%.1f, %.1f)", player.position->x, player.position->z), 10, 40, 20, BLUE);
            DrawText(TextFormat("ON FLOOR: %s", debug), 10, 70, 20, BLUE);
            DrawText(TextFormat("Raycast: (%f, %f, %f)", collision.point.x, collision.point.y, collision.point.z),
                   10, 100, 20, BLUE);
            DrawFPS(10, 10);
            float texture_scale = 200.0/width;
            DrawTextureEx(color_map, (Vector2){screenWidth-color_map.width*texture_scale, 0}, 0, texture_scale, WHITE);
        }
        EndDrawing();
    }

    UnloadModel(model); 
    CloseWindow();

    return 0;
}
