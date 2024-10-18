#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <stdio.h>

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
Mesh gen_mesh_terrain(Image heightmap, Vector3 size)
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
    Vector3 scaleFactor = { size.x / (mapX - 1), size.y, size.z / (mapZ - 1) };

    Vector3 vA, vB, vC, vN;

    for (int z = 0; z < mapZ - 1; z++)
    {
        for (int x = 0; x < mapX - 1; x++)
        {
            // First triangle (vA, vB, vC)
            mesh.vertices[vCounter]     = (float)x * scaleFactor.x;
            mesh.vertices[vCounter + 1] = pow(GRAY_VALUE(pixels[x + z * mapX])/255, 3) * scaleFactor.y;
            mesh.vertices[vCounter + 2] = (float)z * scaleFactor.z;

            mesh.vertices[vCounter + 3] = (float)x * scaleFactor.x;
            mesh.vertices[vCounter + 4] = pow(GRAY_VALUE(pixels[x + (z + 1) * mapX])/255, 3) * scaleFactor.y;
            mesh.vertices[vCounter + 5] = (float)(z + 1) * scaleFactor.z;

            mesh.vertices[vCounter + 6] = (float)(x + 1) * scaleFactor.x;
            mesh.vertices[vCounter + 7] = pow(GRAY_VALUE(pixels[(x + 1) + z * mapX])/255, 3) * scaleFactor.y;
            mesh.vertices[vCounter + 8] = (float)z * scaleFactor.z;

            // Second triangle (same as the second part in GenMeshHeightmap)
            mesh.vertices[vCounter + 9]  = mesh.vertices[vCounter + 6];
            mesh.vertices[vCounter + 10] = mesh.vertices[vCounter + 7];
            mesh.vertices[vCounter + 11] = mesh.vertices[vCounter + 8];

            mesh.vertices[vCounter + 12] = mesh.vertices[vCounter + 3];
            mesh.vertices[vCounter + 13] = mesh.vertices[vCounter + 4];
            mesh.vertices[vCounter + 14] = mesh.vertices[vCounter + 5];

            mesh.vertices[vCounter + 15] = (float)(x + 1) * scaleFactor.x;
            mesh.vertices[vCounter + 16] = pow(GRAY_VALUE(pixels[(x + 1) + (z + 1) * mapX])/255, 3) * scaleFactor.y;
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

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Perlin Noise Plane");

    Camera camera = {0};
    camera.position = (Vector3){0.0f, 100.0f, 0.0f};
    camera.target = (Vector3){0.0f, 50.0f, -1.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    // camera.type = CAMERA_PERSPECTIVE;

    // Create the mesh
    int width = 800, length = 800, scale = 8;
    int max_height = 60;
    Image noiseImage = GenImagePerlinNoise(width, length, 0, 0, (float) scale);
    ExportImage(noiseImage, "noise.png");
    Mesh plane = gen_mesh_terrain(noiseImage, (Vector3){width, max_height, length});
    Model model = LoadModelFromMesh(plane);
    
    // altered colored perlin noise image
    int count = 0;
    float white_threshold = 150;
    Color my_color = VIOLET;

    Color *pixels = LoadImageColors(noiseImage);
    for (int i=0; i< noiseImage.width * noiseImage.height; i++){
        int x = i % noiseImage.width;
        int y = i / noiseImage.width;
        if (pixels[i].r < white_threshold){
            int gray = pixels[i].r;
            Vector3 c_vec = Vector3Normalize((Vector3) {my_color.r, my_color.g, my_color.b});
            Color color = {(short)(c_vec.x*gray), (short)(c_vec.y*gray), (short)(c_vec.z*gray), 255};

            ImageDrawPixel(&noiseImage, x, y, color);
        }
        
    }
    model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(noiseImage);

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
        ClearBackground((Color){20, 20, 20});

        BeginMode3D(camera);
        DrawModel(model, (Vector3){-width/2, 0, -length/2}, 1.0f, WHITE);
        EndMode3D();

        DrawFPS(10, 10);

        EndDrawing();
    }

    UnloadModel(model);
    CloseWindow();

    return 0;
}
