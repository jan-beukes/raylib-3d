#ifndef CUSTOM_PLANE_H
#define CUSTOM_PLANE_H

#include <raylib.h>
#include <rlgl.h>
#include <stdlib.h>

void draw_textured_cube(Texture2D texture, Vector3 position, float width, float height, float length, Color color);
// Generate a plane with tiled uv coordinated
Mesh gen_mesh_plane_tiled(float width, float length, int resX, int resZ);

#endif