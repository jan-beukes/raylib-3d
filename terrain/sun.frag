#version 330 core

in vec2 fragTexCoord;

out vec4 outColor;

uniform sampler2D texture0;
uniform mat4 view;
uniform mat4 projection;

const vec3 sunColor = vec3(1.0, 1.0, 0.6);
const float sunSize = 0.02;

void main() {
    const vec3 lightDir = normalize(vec3(-0.2, -0.2, 0.0));

    // Sample the scene color
    vec4 sceneColor = texture(texture0, fragTexCoord);

    // Compute a reference position for the sun in world space
    vec3 sunWorldPos = -lightDir * 1000.0; // A distant point in the light's direction

    // Transform to clip space
    vec4 clipSpacePos = projection * view * vec4(sunWorldPos, 1.0);

    // Perspective divide to get normalized device coordinates (NDC)
    vec3 ndcPos = clipSpacePos.xyz / clipSpacePos.w;

    // Map NDC to screen space (0 to 1)
    vec2 sunScreenPos = ndcPos.xy * 0.5 + 0.5;

    // Check if the sun is visible (clip space w must be positive)
    if (clipSpacePos.w > 0.0) {
        // Compute distance from this fragment to the sun's screen position
        float dist = length(fragTexCoord - sunScreenPos);

        // Create a radial gradient for the sun
        float sunGlow = 1.0 - smoothstep(sunSize, sunSize * 2.5, dist);

        // Add the sun's glow to the scene color
        vec3 finalColor = sceneColor.rgb + sunGlow * sunColor;

        outColor = vec4(finalColor, 1.0);
    } else {
        // If the sun is behind the camera, output the original scene color
        outColor = sceneColor;
    }
}
