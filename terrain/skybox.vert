#version 330 core
in vec3 vertexPosition;

out vec3 fragTexCoord;

uniform mat4 matProjection;
uniform mat4 matView;

void main() {
    fragTexCoord = vertexPosition;
    gl_Position = matProjection * mat4(mat3(matView)) * vec4(vertexPosition, 1.0);
}
