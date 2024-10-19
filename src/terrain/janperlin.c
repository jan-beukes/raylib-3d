#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <stdio.h>
#include "perlin.h"

#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)
#define MIN(X, Y) (X) < (Y) ? (X) : (Y) 

void darken_image_from_noise(Image noise_image, Image *image) {
    ImageResize(image, noise_image.width, noise_image.height);

    Color *noise_pixels = LoadImageColors(noise_image);
    Color *image_pixels = LoadImageColors(*image);

    int min_color_value = 30;

    for (int y = 0; y < image->height; y++){
        for (int x = 0; x < image->width; x++){
            Color image_color = image_pixels[y*image->width + x];
            float noise_value = image_pixels[y*image->width + x].r / 255.0;
            Color new_color = {MIN(min_color_value + image_color.r*noise_value, 255),
                               MIN(min_color_value + image_color.g*noise_value, 255),
                               MIN(min_color_value + image_color.b*noise_value, 255),
                               255};
            ImageDrawPixel(image, x, y, new_color);

        }
    }

}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Perlin Noise Plane");


    // Create the mesh
    int width = 500, length = 500;
    int max_height = 2000;
    float scale = 20;
    Image image;
    Mesh plane;
    Model model; 

    image = my_perlin_image(width, length, GetRandomValue(0, 10000), GetRandomValue(0, 10000), 2, 2, 0.5, 6);
    plane = GenMeshHeightmap(image, (Vector3){scale*width, max_height, scale*length});
    model = LoadModelFromMesh(plane);
    Image mario = LoadImage("res/mario.png");
    darken_image_from_noise(image, &mario);
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(mario);
    

    Texture color_map = LoadTextureFromImage(image);

    Camera camera = {0};
    camera.position = (Vector3){0.0f, max_height*2, 0.0f};
    camera.target = (Vector3){0.0f,  max_height/2, -1.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    // camera.type = CAMERA_PERSPECTIVE;

    DisableCursor();
    const float dude_speed = 80;
    float speed;
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        printf("%f\n", rlGetCullDistanceFar());

        if (IsKeyDown(KEY_LEFT_SHIFT)){
            speed = dude_speed*4;
        }else {
            speed = dude_speed;
        }

        Vector3 rot = {GetMouseDelta().x*0.1, GetMouseDelta().y*0.1, 0};
        
        Vector3 dr = {(IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * speed * dt,
                      (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * speed * dt, 0};

        UpdateCameraPro(&camera, dr, rot, 0);
        if (IsKeyDown(KEY_SPACE)){
            camera.position.y += speed*dt;
            camera.target.y += speed*dt;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL)){
            camera.position.y -= speed*dt;
            camera.target.y -= speed*dt;

        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModel(model, (Vector3){-width*scale/2, 0, -length*scale/2}, 1, WHITE);
        

        EndMode3D();

        DrawFPS(10, 10);
        float texture_scale = 200.0/width;
        DrawTextureEx(color_map, (Vector2){screenWidth-color_map.width*texture_scale, 0}, 0, texture_scale, WHITE);
        EndDrawing();
    }

    UnloadModel(model); 
    CloseWindow();

    return 0;
}
