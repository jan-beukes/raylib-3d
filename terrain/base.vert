#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matNormal;

out vec2 texCoord;
out vec3 fragNormal;

void main()
{
    texCoord = vertexTexCoord;
    fragNormal = vec3(matNormal) * vertexNormal;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
