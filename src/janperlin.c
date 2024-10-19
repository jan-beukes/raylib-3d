#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <stdio.h>
#include "perlin.h"

#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)

Color *GetImageData(Image image)
{
    Color *pixels = malloc(image.width * image.height * sizeof(Color));
    for (int i = 0; i < image.width * image.height; i++)
    {
        int x = i % image.width;
        int y = i / image.width;
        pixels[i] = GetImageColor(image, x, y);
    }
    return pixels;
}

// Function to create a plane mesh with heights from a Perlin noise image
// Function to calculate normals and generate mesh
Mesh gen_mesh_terrain(Image heightmap, float max_height)
{
    #define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)

    Mesh mesh = { 0 };

    int mapX = heightmap.width;
    int mapZ = heightmap.height;

    Color *pixels = LoadImageColors(heightmap);

    // One vertex per pixel, triangle per grid cell
    mesh.triangleCount = (mapX - 1)*(mapZ - 1)*2;    // One quad equals two triangles

    mesh.vertexCount = mesh.triangleCount * 3; // Each triangle has 3 vertices

    // Allocate space for vertex data
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));   // 3 floats per vertex (x, y, z)
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));    // 3 floats per normal (x, y, z)
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));  // 2 floats per texcoord (u, v)
    mesh.colors = NULL;

    int vCounter = 0;   // Vertex position counter
    int tcCounter = 0;  // Texcoords counter
    int nCounter = 0;   // Normal counter

    // Scale factors for heightmap dimensions and vertex heights
    Vector3 scaleFactor = {1, max_height, 1};

    Vector3 vA, vB, vC, vN;

    for (int z = 0; z < mapZ - 1; z++)
    {
        for (int x = 0; x < mapX - 1; x++)
        {
            // First triangle (vA, vB, vC)
            mesh.vertices[vCounter]     = (float)x * scaleFactor.x;
            mesh.vertices[vCounter + 1] = pow(GRAY_VALUE(pixels[(int) (x + z * mapX)])/255, 3) * scaleFactor.y;
            mesh.vertices[vCounter + 2] = (float)z * scaleFactor.z;

            mesh.vertices[vCounter + 3] = (float)x * scaleFactor.x;
            mesh.vertices[vCounter + 4] = pow(GRAY_VALUE(pixels[(int) (x + (z + 1) * mapX)])/255, 3) * scaleFactor.y;
            mesh.vertices[vCounter + 5] = (float)(z + 1) * scaleFactor.z;

            mesh.vertices[vCounter + 6] = (float)(x + 1) * scaleFactor.x;
            mesh.vertices[vCounter + 7] = pow(GRAY_VALUE(pixels[(int) ((x + 1) + z * mapX)])/255, 3) * scaleFactor.y;
            mesh.vertices[vCounter + 8] = (float)z * scaleFactor.z;

            // Second triangle (same as the second part in GenMeshHeightmap)
            mesh.vertices[vCounter + 9]  = mesh.vertices[vCounter + 6];
            mesh.vertices[vCounter + 10] = mesh.vertices[vCounter + 7];
            mesh.vertices[vCounter + 11] = mesh.vertices[vCounter + 8];

            mesh.vertices[vCounter + 12] = mesh.vertices[vCounter + 3];
            mesh.vertices[vCounter + 13] = mesh.vertices[vCounter + 4];
            mesh.vertices[vCounter + 14] = mesh.vertices[vCounter + 5];

            mesh.vertices[vCounter + 15] = (float)(x + 1) * scaleFactor.x;
            mesh.vertices[vCounter + 16] = pow(GRAY_VALUE(pixels[(int) ((x + 1) + (z + 1) * mapX)])/255, 3) * scaleFactor.y;
            mesh.vertices[vCounter + 17] = (float)(z + 1) * scaleFactor.z;

            vCounter += 18; // Move to the next set of vertices

            // Texcoords for both triangles
            mesh.texcoords[tcCounter]     = (float)x / (mapX - 1);
            mesh.texcoords[tcCounter + 1] = (float)z / (mapZ - 1);

            mesh.texcoords[tcCounter + 2] = (float)x / (mapX - 1);
            mesh.texcoords[tcCounter + 3] = (float)(z + 1) / (mapZ - 1);

            mesh.texcoords[tcCounter + 4] = (float)(x + 1) / (mapX - 1);
            mesh.texcoords[tcCounter + 5] = (float)z / (mapZ - 1);

            mesh.texcoords[tcCounter + 6] = mesh.texcoords[tcCounter + 4];
            mesh.texcoords[tcCounter + 7] = mesh.texcoords[tcCounter + 5];

            mesh.texcoords[tcCounter + 8] = mesh.texcoords[tcCounter + 2];
            mesh.texcoords[tcCounter + 9] = mesh.texcoords[tcCounter + 3];

            mesh.texcoords[tcCounter + 10] = (float)(x + 1) / (mapX - 1);
            mesh.texcoords[tcCounter + 11] = (float)(z + 1) / (mapZ - 1);
            tcCounter += 12; // Move to the next set of texcoords

            // Calculate normals for each triangle
            for (int i = 0; i < 18; i += 9)
            {
                vA = (Vector3){ mesh.vertices[nCounter + i], mesh.vertices[nCounter + i + 1], mesh.vertices[nCounter + i + 2] };
                vB = (Vector3){ mesh.vertices[nCounter + i + 3], mesh.vertices[nCounter + i + 4], mesh.vertices[nCounter + i + 5] };
                vC = (Vector3){ mesh.vertices[nCounter + i + 6], mesh.vertices[nCounter + i + 7], mesh.vertices[nCounter + i + 8] };

                vN = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(vB, vA), Vector3Subtract(vC, vA)));

                // Assign the normal to each vertex in the triangle
                for (int j = 0; j < 9; j += 3)
                {
                    mesh.normals[nCounter + i + j]     = vN.x;
                    mesh.normals[nCounter + i + j + 1] = vN.y;
                    mesh.normals[nCounter + i + j + 2] = vN.z;
                }
            }
            nCounter += 18; // Move to the next set of normals
        }
    }

    UnloadImageColors(pixels); // Clean up

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}

void color_noise(Image *noise_image) {
    // altered colored perlin noise image
    int gray_threshold = 120;

    Color *pixels = LoadImageColors(*noise_image);
    for (int i=0; i< noise_image->width * noise_image->height; i++){
        int x = i % noise_image->width;
        int y = i / noise_image->width;
        int gray = pixels[i].r;
        Color my_color;

        int thresh = 0;
        if (gray >= gray_threshold) {
            my_color = GRAY;
            thresh = 255;
        } else {
            thresh = gray_threshold;
            my_color = DARKGREEN;
            gray = gray > 60 ? gray : 60;
        }

        my_color = ColorLerp(ColorBrightness(my_color, -1) , my_color, (float)gray/thresh);
        ImageDrawPixel(noise_image, x, y, my_color);
        
    }
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Perlin Noise Plane");


    // Create the mesh
    int width = 800, length = 800;
    int max_height = 200;
    float map_scale = 0.1;
    Image images[4];
    Mesh planes[4];
    Model models[4]; 

    for (int i = 0; i < 4; i++){
        int x = i % 2;
        int y = i/2; 
        images[i] = my_perlin_image(width, length, x*(width-1), y*(length-1), 2, 2, 0.6, 6);
        planes[i] = gen_mesh_terrain(images[i], max_height);
        models[i] = LoadModelFromMesh(planes[i]);
        color_noise(&images[i]);
        models[i].materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(images[i]);
    }
    ExportImage(images[0], "noise.png");

    Camera camera = {0};
    camera.position = (Vector3){0.0f, max_height*map_scale, 0.0f};
    camera.target = (Vector3){0.0f,  max_height*map_scale, -1.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    // camera.type = CAMERA_PERSPECTIVE;

    DisableCursor();
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        printf("(%f, %f, %f)\n", camera.position.x, camera.position.y, camera.position.z);
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        if (IsKeyDown(KEY_SPACE)){
            camera.position.y += 10*dt;
            camera.target.y += 10*dt;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL)){
            camera.position.y -= 10*dt;
            camera.target.y -= 10*dt;

        }

        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);
        for (int i = 0; i < 4; i++) {
            int x = i % 2 == 0 ? -1 : 1;
            int y = i/2 < 1 ? -1 : 1;
            DrawModel(models[i], (Vector3){x*width*map_scale/2 - x*map_scale, 0, y*length*map_scale/2 - y*map_scale}, map_scale, WHITE);
        }

        EndMode3D();

        DrawFPS(10, 10);

        EndDrawing();
    }

    for (int i = 0; i < 4; i++) UnloadModel(models[i]); 
    CloseWindow();

    return 0;
}
