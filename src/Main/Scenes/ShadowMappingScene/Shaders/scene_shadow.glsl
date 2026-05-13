#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 lightSpaceMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;  // Position des Fragments aus Lichtsicht

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos          = worldPos.xyz;
    Normal           = mat3(transpose(inverse(model))) * aNormal;
    TexCoord         = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = proj * view * worldPos;
}

#shader fragment
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perspective divide → Normalized Device Coordinates (-1 to 1)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    // Tiefe des nächsten Objekts aus Lichtsicht (aus Shadow Map)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Tiefe des aktuellen Fragments aus Lichtsicht
    float currentDepth = projCoords.z;

    // Shadow Bias: verhindert "Shadow Acne" (Selbst-Beschattungs-Artefakte)
    // Je flacher der Winkel zwischen Normal und Licht, desto mehr Bias nötig
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // PCF: Percentage Closer Filtering für weiche Schatten
    // Statt eines Punktes samplen wir einen 3x3 Bereich
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;  // Durchschnitt der 9 Samples

    return shadow;
}

void main() {
    vec3 color = texture(diffuseTexture, TexCoord).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Phong Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Ambient (immer sichtbar, auch im Schatten)
    vec3 ambient = 0.3 * color;

    // Diffuse (wird durch Schatten blockiert)
    vec3 diffuse = diff * color;

    // shadow = 0.0 → voll beleuchtet, 1.0 → voll im Schatten
    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);

    vec3 result = ambient + (1.0 - shadow) * diffuse;
    FragColor = vec4(result, 1.0);
}
