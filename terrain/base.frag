#version 330

in vec2 texCoord;
in vec3 fragNormal;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0; // diffuse texture
uniform int tile;

const float ambientValue = 0.8;
const vec3 diffuseCol = vec3(0.4, 0.4, 0.4);

void main() {
    const vec3 lightDir = normalize(-vec3(-0.2, -0.2, -0.0));
    vec3 texColor = texture(texture0, fract(texCoord * tile)).rgb; // tile
    
    // diffuse 
    vec3 norm = normalize(fragNormal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseCol * diff * texColor;

    vec3 ambient = ambientValue * texColor;
    vec3 result = ambient + diffuse;

    finalColor = vec4(result, 1.0);
}


