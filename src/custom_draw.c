#include "custom_draw.h"


#define TILE_WIDTH 4

// Draw cube with texture piece applied to all faces (tiled)
// center at pos
void draw_textured_cube(Texture2D texture, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    // Set desired texture to be enabled while drawing following vertex data
    rlSetTexture(texture.id);

    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);

        // Front face
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0, height/TILE_WIDTH); // Bottom left
        rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(width/TILE_WIDTH, height/TILE_WIDTH);// Bottom right
        rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(width/TILE_WIDTH, 0);// Top right
        rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0, 0); // Top left
        rlVertex3f(x - width/2, y + height/2, z + length/2);
        
        // Back face
        rlNormal3f(0.0f, 0.0f, - 1.0f);
        rlTexCoord2f(width/TILE_WIDTH, height/TILE_WIDTH); // Bottom right
        rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(width/TILE_WIDTH, 0); // top right
        rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(0, 0); // top left
        rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0, height/TILE_WIDTH); // bottom left
        rlVertex3f(x + width/2, y - height/2, z - length/2);

        // Top face
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0, 0); // top left
        rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(1, 1); // bottom left
        rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(0, 0);
        rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(1, 1);
        rlVertex3f(x + width/2, y + height/2, z - length/2);

        // Bottom face
        rlNormal3f(0.0f, - 1.0f, 0.0f);
        rlTexCoord2f(0, 0);
        rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(0, 0);
        rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(0, 0);
        rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(0, 0);
        rlVertex3f(x - width/2, y - height/2, z + length/2);

        // Right face
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(length/TILE_WIDTH, height/TILE_WIDTH); // bottom right
        rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(length/TILE_WIDTH, 0); // top right
        rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0, 0); // top left
        rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0, height/TILE_WIDTH); // bottom left
        rlVertex3f(x + width/2, y - height/2, z + length/2);

        // Left face
        rlNormal3f( - 1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0, height/TILE_WIDTH); // bottom left 
        rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(length/TILE_WIDTH, height/TILE_WIDTH); // bottom right
        rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(length/TILE_WIDTH, 0); // top right
        rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(0, 0); // top left
        rlVertex3f(x - width/2, y + height/2, z - length/2);

    rlEnd();

    rlSetTexture(0);
}

Mesh gen_mesh_plane_tiled(float width, float length, int resX, int resZ) {
  Mesh mesh = {0};

  resX++;
  resZ++;

  // Vertices definition
  int vertexCount = resX * resZ; // vertices get reused for the faces

  Vector3 *vertices = (Vector3 *)RL_MALLOC(vertexCount * sizeof(Vector3));
  for (int z = 0; z < resZ; z++) {
    // [-length/2, length/2]
    float zPos = ((float)z / (resZ - 1) - 0.5f) * length;
    for (int x = 0; x < resX; x++) {
      // [-width/2, width/2]
      float xPos = ((float)x / (resX - 1) - 0.5f) * width;
      vertices[x + z * resX] = (Vector3){xPos, 0.0f, zPos};
    }
  }

  // Normals definition
  Vector3 *normals = (Vector3 *)RL_MALLOC(vertexCount * sizeof(Vector3));
  for (int n = 0; n < vertexCount; n++)
    normals[n] = (Vector3){0.0f, 1.0f, 0.0f}; // Vector3.up;

  // TexCoords definition
  Vector2 *texcoords = (Vector2 *)RL_MALLOC(vertexCount * sizeof(Vector2));
  for (int v = 0; v < resZ; v++) {
    for (int u = 0; u < resX; u++) {
      texcoords[u + v * resX] =
          (Vector2){(float)u / (resX - 1), (float)v / (resZ - 1)};
    }
  }

  // Triangles definition (indices)
  int numFaces = (resX - 1) * (resZ - 1);
  int *triangles = (int *)RL_MALLOC(numFaces * 6 * sizeof(int));
  int t = 0;
  for (int face = 0; face < numFaces; face++) {
    // Retrieve lower left corner from face ind
    int i = face + face / (resX - 1);

    triangles[t++] = i + resX;
    triangles[t++] = i + 1;
    triangles[t++] = i;

    triangles[t++] = i + resX;
    triangles[t++] = i + resX + 1;
    triangles[t++] = i + 1;
  }

  mesh.vertexCount = vertexCount;
  mesh.triangleCount = numFaces * 2;
  mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
  mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
  mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
  mesh.indices = (unsigned short *)RL_MALLOC(mesh.triangleCount * 3 *
                                             sizeof(unsigned short));

  // Mesh vertices position array
  for (int i = 0; i < mesh.vertexCount; i++) {
    mesh.vertices[3 * i] = vertices[i].x;
    mesh.vertices[3 * i + 1] = vertices[i].y;
    mesh.vertices[3 * i + 2] = vertices[i].z;
  }

  // Mesh texcoords array
  for (int i = 0; i < mesh.vertexCount; i++) {
    mesh.texcoords[2 * i] = texcoords[i].x * resX;
    mesh.texcoords[2 * i + 1] = texcoords[i].y * resX;
  }

  // Mesh normals array
  for (int i = 0; i < mesh.vertexCount; i++) {
    mesh.normals[3 * i] = normals[i].x;
    mesh.normals[3 * i + 1] = normals[i].y;
    mesh.normals[3 * i + 2] = normals[i].z;
  }

  // Mesh indices array initialization
  for (int i = 0; i < mesh.triangleCount * 3; i++)
    mesh.indices[i] = triangles[i];

  RL_FREE(vertices);
  RL_FREE(normals);
  RL_FREE(texcoords);
  RL_FREE(triangles);

  // Upload vertex data to GPU (static mesh)
  UploadMesh(&mesh, false);
  return mesh;
}