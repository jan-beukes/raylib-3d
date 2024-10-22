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

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define GRAVITY 100.0
#define JUMP 60

typedef struct Player {
    float height;
    Vector3 *position;
    Camera3D *camera;
    float vel_y;
} Player;

typedef struct Terrain {
    Model *model;
    Mesh *mesh;
    int width;
    int length;
    float resolution;

} Terrain;

typedef struct Block {
    Vector3 pos;
    Vector3 size;
    BoundingBox bounds;
    Color color;

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

bool raycast_heightmap(Ray ray, Vector3 *collision, Image heightmap_image, int max_height, float terrain_res, Vector3 terrain_pos) {
    int width = heightmap_image.width;
    int height = heightmap_image.height;
    Color *heightmap_data = LoadImageColors(heightmap_image);  // Assuming grayscale heightmap

    // Ray step parameters
    float step_size = 0.1f; // The distance to move along the ray each iteration (adjust for precision)
    float max_distance = 2*width; // Maximum distance to march along the ray

    Vector3 ray_pos = ray.position;
    Vector3 ray_dir = Vector3Normalize(ray.direction); // Normalize the direction

    for (float distance = 0; distance < max_distance; distance += step_size) {
        // Move along the ray
        ray_pos = Vector3Add(ray.position, Vector3Scale(ray_dir, distance));

        // Convert current ray position to heightmap coordinates
        float terrain_x = (ray_pos.x - terrain_pos.x)*terrain_res;
        float terrain_z = (ray_pos.z - terrain_pos.z)*terrain_res;

        // Check if we're within the bounds of the heightmap
        if (terrain_x >= 0 && terrain_x < width && terrain_z >= 0 && terrain_z < height) {
            // Get heightmap value at the current terrain coordinates
            int ix = (int)terrain_x;
            int iz = (int)terrain_z;
            float terrainHeight = GRAY_VALUE(heightmap_data[iz * width + ix]) / 255.0f * max_height + terrain_pos.y;

            // If the ray's y-position is below the terrain height, we've hit the terrain
            if (ray_pos.y <= terrainHeight) {
                UnloadImageColors(heightmap_data);  // Free the heightmap colors
                *collision = ray_pos;  // Return the collision point
                return true;
            }
        }
    }

    UnloadImageColors(heightmap_data);  // Free the heightmap colors
    *collision = (Vector3){0};  // No collision found
    return false;
}

void move_player(Player *player, Terrain terrain, Block *blocks, int block_count, float dt, float dude_speed) {
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

    // Block Collsion
    for (int i = 0; i < block_count; i++) {
        // Top collision
        Vector3 feet_pos = Vector3Add(Vector3Subtract(*player->position, (Vector3){0, player->height, 0}),
                                                (Vector3){0,player->vel_y*dt, 0});
        if (CheckCollisionBoxSphere(blocks[i].bounds, feet_pos, 0.1)) {
            floor = true;
        }
        if (CheckCollisionBoxSphere(blocks[i].bounds, 
                  Vector3Add(*player->position,(Vector3){0,player->height/2, 0}), 0.1)) {
            floor = true;
        }

        Vector3 move_vec = Vector3Add(Vector3Scale(GetCameraForward(player->camera), dr.x),
                                     Vector3Scale(GetCameraRight(player->camera), dr.y));
        Vector3 player_point = Vector3Subtract(*player->position, (Vector3){0, player->height/2, 0});

        bool collision_cur = CheckCollisionBoxSphere(blocks[i].bounds, player_point, player->height/2);
        bool collision_step = CheckCollisionBoxSphere(blocks[i].bounds, 
                                                    Vector3Add(player_point, move_vec), player->height/2);
        // Collision on this frame
        if (!collision_cur && collision_step) {
            dr = Vector3Zero();
            break;
        }

        
    }


    UpdateCameraPro(player->camera, dr, rot, 0); 

    // Handle Terrain collision
    Vector3 player_pos = Vector3Transform(*player->position, MatrixInvert(terrain.model->transform));
    int cols = (int)(terrain.width*terrain.resolution);
    int rows = (int)(terrain.length*terrain.resolution);
    float mesh_height = get_terrain_height(player_pos.x, player_pos.z, terrain.mesh->vertices,
                                    rows, cols, 1/terrain.resolution);
    // subtract epsilon for more lenient jumping
    float epsilon = 1;
    
    // not on block
    if (!floor) {
        floor = mesh_height != -1 && player_pos.y - player->height - epsilon <= mesh_height; 
    }
    if (mesh_height != -1 && player_pos.y - player->height <= mesh_height) {
        float y_move = mesh_height + player->height - player->position->y; 
        player_move_position(player,(Vector3){0, y_move, 0});
    } 
    
    char *text = floor == 1 ? "true" : "false";
    sprintf(debug, "%s", text);
    if (floor){
        player->vel_y = 0;
    } else {
        player->vel_y -= GRAVITY*dt;
    }

    // Jump
    if (IsKeyPressed(KEY_J)) {
        jump_force = jump_force == JUMP ? JUMP*3 : JUMP;
    }
    if (floor && IsKeyDown(KEY_SPACE)) {
        player->vel_y = jump_force;
    }
    
    player_move_position(player, (Vector3){0,player->vel_y*dt, 0});
    
}

int main(void)
{
    debug = malloc(255*sizeof(char));

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "EPIC MAN");
    //SetTargetFPS(120);

    // Create the mesh
    int width = 800, length = 800;
    int max_height = width/4;
    float resolution = 0.5;
    Image image;
    Mesh plane;
    Model model; 

    image = my_perlin_image((int)(width*resolution), (int)(length*resolution), GetRandomValue(0, 10000), GetRandomValue(0, 10000), 2, 2, 0.5, 6);
    plane = GenMeshHeightmap(image, (Vector3){width, max_height, length});
    model = LoadModelFromMesh(plane);
    
    Image mario = LoadImage("res/mario.png");
    mario = darken_image_from_noise(image, mario);
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(mario);    
    model.transform = MatrixTranslate(-width/2, 0, -length/2);

    Terrain terrain = {
        .model = &model,
        .mesh = &plane,
        .width = width,
        .length = length,
        .resolution = resolution
    };

    Block blocks[1024];
    int count = 0;

    Texture color_map = LoadTextureFromImage(image);

    Camera camera = {0};
    camera.position = (Vector3){0.0f, max_height, -1.0f}; // Forward vector is zero if target (x,z) is same
    camera.target = (Vector3){0.0f,  max_height, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    // camera.type = CAMERA_PERSPECTIVE;

    Player player = {0};
    player.camera = &camera;
    player.height = 8;
    player.position = &camera.position;

    rlSetLineWidth(10);
    DisableCursor();
    const float dude_speed = 10;
    float speed;
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Player Stuff
        move_player(&player, terrain, blocks, count, dt, dude_speed);

        Ray ray;
        ray.position = camera.position;
        ray.direction = GetCameraForward(&camera);

        Vector3 collision;
        bool collided = false;
        int block_index = -1;
        
        RayCollision closest = {0}; 
        for (int i = 0; i < count; i++) {
            RayCollision ray_col = GetRayCollisionBox(ray, blocks[i].bounds);
            if (ray_col.hit){
                if (!collided) {
                    collided = true;
                    closest = ray_col;
                    block_index = i;
                    continue;
                }
                float v1 = Vector3Distance(*player.position, closest.point);
                float v2 = Vector3Distance(*player.position, ray_col.point);
                if (v2 < v1) {
                    closest = ray_col;
                    block_index = i;   
                }  
            }
        }
        if (collided) {
            collision = Vector3Add(Vector3Scale(closest.normal, 2.5), closest.point); 
        } else {
            collided = raycast_heightmap(ray, &collision, image, max_height,
                                        terrain.resolution,(Vector3){-width/2, 0, -length/2});
            if (collided) collision.y += 2;
        }
        
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && collided){
            Vector3 size = (Vector3){5, 5, 5};
            BoundingBox bounds =  {Vector3Subtract(collision,(Vector3){size.x/2, size.y/2, size.z/2}),
                                   Vector3Add(collision,(Vector3){size.x/2, size.y/2, size.z/2})};
            Color color = {GetRandomValue(0,255),GetRandomValue(0,255),GetRandomValue(0,255),255};
            blocks[count++] = (Block){collision, size, bounds, color};
        } else if (collided && block_index != -1 && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            blocks[block_index] = (Block){0};
        }

        BeginDrawing();
        {
            ClearBackground(SKYBLUE);
            // ---3D----
            BeginMode3D(camera);
        
            DrawModel(model, Vector3Zero(), 1, WHITE);
            //DrawModelWires(model, Vector3Zero(), 1, WHITE);
            
            for (int i = 0; i < count; i++) {
                // Goofy check for empty struct
                if (blocks[i].size.x) {
                    DrawCubeV(blocks[i].pos, blocks[i].size, blocks[i].color);
                }
            }
            EndMode3D();
            
            // ---2D---
            DrawText(TextFormat("Position (%.1f, %.1f)", player.position->x, player.position->z), 10, 40, 20, BLUE);
            DrawText(TextFormat("ON FLOOR: %s", debug), 10, 70, 20, BLUE);
            DrawText(TextFormat("Raycast: (%f, %f, %f)", collision.x, collision.y, collision.z),
                   10, 100, 20, BLUE);
            DrawFPS(10, 10);
            float texture_scale = 200.0/width;
            DrawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 2, WHITE);
            DrawTextureEx(color_map, (Vector2){SCREEN_WIDTH-color_map.width*texture_scale, 0}, 0, texture_scale, WHITE);
        }
        EndDrawing();
    }

    UnloadModel(model); 
    CloseWindow();

    return 0;
}
