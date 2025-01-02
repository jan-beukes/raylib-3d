#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D depthTexture;
uniform float fogDensity;
uniform vec3 fogColor;

float linearizeDepth(float depth) {
    const float zNear = 0.1;
    const float zFar = 2000.0;
    return (2 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}

void main() {
    float depth = texture(depthTexture, fragTexCoord).r;
    vec3 texColor = texture(texture0, fragTexCoord).rgb;
    float z = linearizeDepth(depth);

    float fogValue = z * fogDensity;

    vec3 fog = fogColor * fogValue;
    vec3 scene = texColor * (1.0 - fogValue / 2);
    vec3 result = fog + scene;

    fragColor = vec4(result, 1.0);
}
