#include <raylib.h>
#include <stdlib.h>
#include "../lib/stb_perlin.h"

Image my_perlin_image(int width, int height, int x_off, int y_off, float scale, float lacunarity, float gain, int octaves)
{
    Color *pixels = (Color *)RL_MALLOC(width*height*sizeof(Color));


    float aspectRatio = (float)width / (float)height;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float nx = (float)(x + x_off)*(scale/(float)width);
            float ny = (float)(y + y_off)*(scale/(float)height);

            // Apply aspect ratio compensation to wider side
            if (width > height) nx *= aspectRatio;
            else ny /= aspectRatio;
            
            // Basic perlin noise implementation (not used)
            //float p = (stb_perlin_noise3(nx, ny, 0.0f, 0, 0, 0);

            // Calculate a better perlin noise using fbm (fractal brownian motion)
            // Typical values to start playing with:
            //   lacunarity = ~2.0   -- spacing between successive octaves (use exactly 2.0 for wrapping output)
            //   gain       =  0.5   -- relative weighting applied to each successive octave
            //   octaves    =  6     -- number of "octaves" of noise3() to sum
            float p = stb_perlin_fbm_noise3(nx, ny, 1.0f, lacunarity, gain, octaves);
            // Clamp between -1.0f and 1.0f
            if (p < -1.0f) p = -1.0f;
            if (p > 1.0f) p = 1.0f;

            // We need to normalize the data from [-1..1] to [0..1]
            float np = (p + 1.0f)/2.0f;

            int intensity = (int)(np*255.0f);
            pixels[y*width + x] = (Color){ intensity, intensity, intensity, 255 };
        }
    }

    Image image = {
        .data = pixels,
        .width = width,
        .height = height,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };

    return image;
}
