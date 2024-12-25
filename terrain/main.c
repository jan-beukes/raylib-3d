#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>

#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b) / 3.0f)
#define MIN(X, Y) (X) < (Y) ? (X) : (Y)
#define MAX(X, Y) (X) > (Y) ? (X) : (Y)

Image my_perlin_image(int width, int height, int x_off, int y_off, float scale, float lacunarity, float gain, int octaves);

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define GRAVITY 100.0
#define JUMP 60
#define BLOCK_SIZE 5

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
  int height;
  float resolution;

} Terrain;

typedef struct Block {
  Vector3 pos;
  BoundingBox bounds;
  int texture_id;
  bool active;
} Block;

char *debug;
int jump_force = JUMP;

Image darken_image_from_noise(Image noise_image, Image image) {
  ImageResize(&image, noise_image.width, noise_image.height);

  Color *noise_pixels = LoadImageColors(noise_image);
  Color *image_pixels = LoadImageColors(image);
  Color *data = calloc(image.width * image.height, sizeof(Color));
  int min_color_value = 30;

  for (int y = 0; y < image.height; y++) {
    for (int x = 0; x < image.width; x++) {
      Color image_color = image_pixels[y * image.width + x];
      float noise_value = noise_pixels[y * image.width + x].r / 255.0;
      Color new_color = {
          MIN(min_color_value + image_color.r * noise_value, 255),
          MIN(min_color_value + image_color.g * noise_value, 255),
          MIN(min_color_value + image_color.b * noise_value, 255), 255};
      data[y * image.width + x] = new_color;
    }
  }
  Image new = {.data = data,
               .width = image.width,
               .height = image.height,
               .mipmaps = 1,
               .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  return new;
}

void player_move_position(Player *player, Vector3 move) {
  *player->position = Vector3Add(*player->position, move);
  player->camera->target = Vector3Add(player->camera->target, move);
}

// Function to compute barycentric coordinates for a point P in a triangle defined by vertices A, B, C
void barycentric_coordinates(Vector2 P, Vector2 A, Vector2 B, Vector2 C, float* lambda1, float* lambda2, float* lambda3) {
    float denom = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    
    *lambda1 = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denom;
    *lambda2 = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denom;
    *lambda3 = 1.0f - *lambda1 - *lambda2;  // Ensure that the sum of lambdas equals 1
}

float get_terrain_height(float x, float z, const float *vertices, int rows,
                         int cols, float cell_size) {
  // Find grid cell indices from position
  int cell_x = floor(x / cell_size);
  int cell_z = floor(z / cell_size);

  // handle out of bounds
  if (cell_x >= cols - 1 || cell_z >= rows - 1 || cell_x < 0 || cell_z < 0) {
    return -1;
  }

  // Indiced for corners of the grid cell
  int vertex_index_top_left =
      (cell_z * (cols - 1) + cell_x) * 18; // Step of 18 floats for each cell
  int vertex_index_bottom_left =
      vertex_index_top_left + 3; // Offset to the bottom-left vertex
  int vertex_index_top_right =
      vertex_index_top_left + 6; // Offset to the top-right vertex
  int vertex_index_bottom_right =
      vertex_index_top_left + 15; // Offset to the bottom-right vertex

  // Corners of the cell
  Vector3 tl = *(Vector3 *)&vertices[vertex_index_top_left];
  Vector3 bl = *(Vector3 *)&vertices[vertex_index_bottom_left];
  Vector3 tr = *(Vector3 *)&vertices[vertex_index_top_right];
  Vector3 br = *(Vector3 *)&vertices[vertex_index_bottom_right];

  // Height interpolated with barycentric coordinates
  // First triangle: V1, V2, V3
    float lambda1, lambda2, lambda3;
    barycentric_coordinates((Vector2){x, z}, (Vector2){tl.x, tl.z}, (Vector2){bl.x, bl.z}, (Vector2){tr.x, tr.z}, &lambda1, &lambda2, &lambda3);

    // Check if the point lies inside the first triangle
    if (lambda1 >= 0 && lambda2 >= 0 && lambda3 >= 0) {
        // Interpolate the height for the first triangle
        return lambda1 * tl.y + lambda2 * bl.y + lambda3 * tr.y;
    }

    // If the point doesn't lie inside the first triangle, check the second triangle: V3, V4, V2
    float lambda4, lambda5, lambda6;
    barycentric_coordinates((Vector2){x, z}, (Vector2){tr.x, tr.z}, (Vector2){br.x, br.z}, (Vector2){bl.x, bl.z}, &lambda4, &lambda5, &lambda6);
    
    // Interpolate the height for the second triangle
    return lambda4 * tr.y + lambda5 * br.y + lambda6 * bl.y;
}

bool raycast_heightmap(Ray ray, Vector3 *collision, Image heightmap_image,
                       int max_height, float terrain_res, Vector3 terrain_pos) {
  int width = heightmap_image.width;
  int height = heightmap_image.height;
  Color *heightmap_data =
      LoadImageColors(heightmap_image); // Assuming grayscale heightmap

  // Ray step parameters
  float step_size = 0.1f; // The distance to move along the ray each iteration
                          // (adjust for precision)
  float max_distance = 2 * width; // Maximum distance to march along the ray

  Vector3 ray_pos = ray.position;
  Vector3 ray_dir = Vector3Normalize(ray.direction); // Normalize the direction

  for (float distance = 0; distance < max_distance; distance += step_size) {
    // Move along the ray
    ray_pos = Vector3Add(ray.position, Vector3Scale(ray_dir, distance));

    // Convert current ray position to heightmap coordinates
    float terrain_x = (ray_pos.x - terrain_pos.x) * terrain_res;
    float terrain_z = (ray_pos.z - terrain_pos.z) * terrain_res;

    // Check if we're within the bounds of the heightmap
    if (terrain_x >= 0 && terrain_x < width && terrain_z >= 0 &&
        terrain_z < height) {
      // Get heightmap value at the current terrain coordinates
      int ix = (int)terrain_x;
      int iz = (int)terrain_z;
      float terrainHeight =
          GRAY_VALUE(heightmap_data[iz * width + ix]) / 255.0f * max_height +
          terrain_pos.y;

      // If the ray's y-position is below the terrain height, we've hit the
      // terrain
      if (ray_pos.y <= terrainHeight) {
        UnloadImageColors(heightmap_data); // Free the heightmap colors
        *collision = ray_pos;              // Return the collision point
        return true;
      }
    }
  }

  UnloadImageColors(heightmap_data); // Free the heightmap colors
  *collision = (Vector3){0};         // No collision found
  return false;
}

void move_player(Player *player, Terrain *terrain, Block *blocks, int block_count, float dt, float dude_speed) {
  float speed;
  bool floor = false;

  if (IsKeyDown(KEY_LEFT_SHIFT)) {
    speed = dude_speed * 4;
  } else {
    speed = dude_speed;
  }
  Vector3 rot = {GetMouseDelta().x * 0.1, GetMouseDelta().y * 0.1, 0};

  Vector3 dr = {(IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * speed * dt,
                (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * speed * dt, 0};

  // Block Collsion
  for (int i = 0; i < block_count; i++) {
    if (!blocks[i].active)
      continue;

    // Top collision
    Vector3 feet_pos = Vector3Add(
        Vector3Subtract(*player->position, (Vector3){0, player->height, 0}),
        (Vector3){0, player->vel_y * dt, 0});
    if (CheckCollisionBoxSphere(blocks[i].bounds, feet_pos, 0.1)) {
      floor = true;
    }
    if (CheckCollisionBoxSphere(
            blocks[i].bounds,
            Vector3Add(*player->position, (Vector3){0, player->height / 2, 0}),
            0.1)) {
      floor = true;
    }

    Vector3 move_vec =
        Vector3Add(Vector3Scale(GetCameraForward(player->camera), dr.x),
                   Vector3Scale(GetCameraRight(player->camera), dr.y));
    Vector3 player_point =
        Vector3Subtract(*player->position, (Vector3){0, player->height / 2, 0});

    bool collision_cur = CheckCollisionBoxSphere(blocks[i].bounds, player_point,
                                                 player->height / 2);
    bool collision_step = CheckCollisionBoxSphere(
        blocks[i].bounds, Vector3Add(player_point, move_vec),
        player->height / 2);
    // Collision on this frame
    if (!collision_cur && collision_step) {
      dr = Vector3Zero();
      break;
    }
  }

  UpdateCameraPro(player->camera, dr, rot, 0);

  // Handle Terrain collision
  Vector3 player_pos = Vector3Transform(
      *player->position, MatrixInvert(terrain->model->transform));

    int cols = (int)(terrain->width*terrain->resolution);
    int rows = (int)(terrain->length*terrain->resolution);
    float mesh_y = get_terrain_height(player_pos.x, player_pos.z, terrain->mesh->vertices,
                                    rows, cols, 1/terrain->resolution);
    if (mesh_y != -1) {
      float epsilon = 1; // subtract epsilon for more lenient jumping

      floor = floor || (player_pos.y - player->height - epsilon <= mesh_y);
      if (player_pos.y - player->height <= mesh_y) {
        float y_move = mesh_y + player->height - player->position->y;
        player->vel_y -= 100;
        player_move_position(player, (Vector3){0, y_move, 0});
      }
  }

  char *text = floor ? "true" : "false";
  sprintf(debug, "%s", text);
  if (floor) {
    player->vel_y = 0;
  } else {
    player->vel_y -= GRAVITY * dt;
  }

  // Jump
  if (IsKeyPressed(KEY_J)) {
    jump_force = jump_force == JUMP ? JUMP * 3 : JUMP;
  }
  if (floor && IsKeyDown(KEY_SPACE)) {
    player->vel_y = jump_force;
  }

  player_move_position(player, (Vector3){0, player->vel_y * dt, 0});
}

int main(void) {
  debug = malloc(255 * sizeof(char));
  // SetTraceLogLevel(LOG_WARNING);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "EPIC MAN");
  SetTargetFPS(144);

  // Create the mesh
  int width = 1200, length = 1200;
  int max_height = width / 4;
  const float resolution = 0.3;
  Image image;
  Mesh plane;
  Model model;

  Shader terrain_shader = LoadShader("terrain/base.vs", "terrain/tile.fs");
  Shader block_shader = LoadShader("terrain/base.vs", NULL);
  SetShaderValue(terrain_shader, GetShaderLocation(terrain_shader, "maxHeight"),
                 &max_height, SHADER_UNIFORM_INT);

  image = my_perlin_image((int)(width * resolution), (int)(length * resolution),
                          GetRandomValue(0, 10000), GetRandomValue(0, 10000), 2,
                          2, 0.4, 6);
  plane = GenMeshHeightmap(image, (Vector3){width, max_height, length});
  model = LoadModelFromMesh(plane);

  Image mario = LoadImage("res/tough_grass.png");
  ImageMipmaps(&mario);
  Texture texture = LoadTextureFromImage(mario);
  SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(texture, TEXTURE_FILTER_ANISOTROPIC_16X);
  model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = texture;
  model.materials->shader = terrain_shader;
  model.transform = MatrixTranslate(-width / 2, 0, -length / 2);

  Terrain terrain = {.model = &model,
                     .mesh = &plane,
                     .width = width,
                     .length = length,
                     .height = max_height,
                     .resolution = resolution};

  Block blocks[1024];
  int count = 0;
  int current_texture = 0;
  Texture block_textures[2] = {LoadTexture("res/floor.png"),
                               LoadTexture("res/wall1.png")};
  int texture_count = 2;
  Mesh block_mesh = GenMeshCube(BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
  Model block_model = LoadModelFromMesh(block_mesh);
  block_model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture =
      block_textures[0];
  block_model.materials[0].shader = block_shader;

  // block_materials[1].maps[MATERIAL_MAP_ALBEDO].texture =
  // LoadTexture("res/wall1.png");
  Texture color_map = LoadTextureFromImage(image);

  Camera camera = {0};
  camera.position =
      (Vector3){0.0f, max_height,
                -1.0f}; // Forward vector is zero if target (x,z) is same
  camera.target = (Vector3){0.0f, max_height, 0.0f};
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
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // Player Stuff
    move_player(&player, &terrain, blocks, count, dt, dude_speed);

    Ray ray;
    ray.position = camera.position;
    ray.direction = GetCameraForward(&camera);

    Vector3 collision;
    bool collided = false;
    int block_index = -1;

    RayCollision closest = {0};
    for (int i = 0; i < count; i++) {
      if (!blocks[i].active)
        continue;

      RayCollision ray_col = GetRayCollisionBox(ray, blocks[i].bounds);
      if (ray_col.hit) {
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
                                   terrain.resolution,
                                   (Vector3){-width / 2, 0, -length / 2});
      if (collided)
        collision.y += 2;
    }

    if (IsKeyPressed(KEY_ONE)) {
      current_texture = 0;
    } else if (IsKeyPressed(KEY_TWO)) {
      current_texture = 1;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && collided) {
      BoundingBox bounds = {
          Vector3Subtract(collision, (Vector3){BLOCK_SIZE / 2, BLOCK_SIZE / 2,
                                               BLOCK_SIZE / 2}),
          Vector3Add(collision, (Vector3){BLOCK_SIZE / 2, BLOCK_SIZE / 2,
                                          BLOCK_SIZE / 2})};
      blocks[count++] = (Block){collision, bounds, current_texture, true};

    } else if (collided && block_index != -1 &&
               IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
      printf("remove %d\n", block_index);
      blocks[block_index].active = false;
    }

    BeginDrawing();
    {
      ClearBackground(SKYBLUE);
      // ---3D----
      BeginMode3D(camera);

      DrawModel(model, Vector3Zero(), 1, WHITE);

      for (int i = 0; i < count; i++) {
        if (blocks[i].active) {
          block_model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture =
              block_textures[blocks[i].texture_id];
          DrawModel(block_model, blocks[i].pos, 1, WHITE);
        }
      }
      EndMode3D();

      // ---2D---
      DrawFPS(10, 10);
      // Text
      DrawText(TextFormat("Position (%.1f, %.1f, %.1f)", player.position->x,
                          player.position->z, player.position->y),
               10, 40, 20, WHITE);
      DrawText(TextFormat("ON FLOOR: %s", debug), 10, 70, 20, WHITE);
      DrawText(TextFormat("Raycast: (%.1f, %.1f, %.1f)", collision.x,
                          collision.y, collision.z),
               10, 100, 20, WHITE);
      DrawText(TextFormat("Blocks: %d", count), 10, 130, 20, WHITE);

      float texture_scale = 200.0 / (width * resolution);
      DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2, WHITE);
      DrawTextureEx(
          color_map,
          (Vector2){SCREEN_WIDTH - color_map.width * texture_scale, 0}, 0,
          texture_scale, WHITE);
    }
    EndDrawing();
  }

  UnloadModel(model);
  CloseWindow();

  return 0;
}
