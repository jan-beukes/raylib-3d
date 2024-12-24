#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform int maxHeight;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;

// NOTE: Add here your custom variables

void main()
{
    float value = sqrt(max(0, vertexPosition.y / maxHeight));
    fragColor = vec4(vec3(value), 1);

    fragTexCoord = vertexTexCoord;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
