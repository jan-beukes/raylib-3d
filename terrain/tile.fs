#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Texel color fetching from texture sampler
    vec4 texColor = texture(texture0, fract(fragTexCoord * 20));

    finalColor = fragColor * texColor;
    finalColor *= finalColor;
}


