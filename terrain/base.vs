#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform int maxHeight;

out vec2 fragTexCoord;
out vec4 fragColor;

const float lightAngle = 65;
const float ambiant = 0.3;

void main()
{
    float value = max(0, dot(vec3(-cos(lightAngle), sin(lightAngle), 0), vertexNormal));
    fragColor = vec4(vec3(value + ambiant), 1);

    fragTexCoord = vertexTexCoord;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
