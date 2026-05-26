#shader vertex
#version 330 core 

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main(){
  vec4 worldPos = model * vec4(aPos, 1.0f);
  vWorldPos = worldPos.xyz;
  vNormal = mat3(transpose(inverse(model))) * aNormal;
  vUV = aUV;

  gl_Position = proj * view * worldPos;
}

#shader fragment
#version 330 core

in vec3 vWorldPos; 
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

uniform float maxHeight;
uniform vec3 lightDir;
uniform vec3 lightColor;

void main() {
  float normalizedHeight = vWorldPos.y / maxHeight;
    
  vec3 color;
  if (normalizedHeight < 0.15) {
    // Tiefstes: Sand/Wasser
    color = mix(vec3(0.2, 0.3, 0.6), vec3(0.76, 0.70, 0.50),
                    normalizedHeight / 0.15);
  } else if (normalizedHeight < 0.45) {
    // Gras
    color = mix(vec3(0.3, 0.55, 0.2), vec3(0.25, 0.45, 0.15),
                    (normalizedHeight - 0.15) / 0.30);
  } else if (normalizedHeight < 0.75) {
    // Fels
    color = mix(vec3(0.5, 0.45, 0.4), vec3(0.4, 0.35, 0.3),
                    (normalizedHeight - 0.45) / 0.30);
  } else {
    // Schnee
    color = mix(vec3(0.85, 0.85, 0.9), vec3(1.0, 1.0, 1.0),
                    (normalizedHeight - 0.75) / 0.25);
  }

  // Diffuse Beleuchtung (Lambertian)
  vec3 N = normalize(vNormal);
  float diff = max(dot(N, normalize(lightDir)), 0.0);

  vec3 ambient  = 0.15 * lightColor * color;
  vec3 diffuse  = diff * lightColor * color;

  FragColor = vec4(ambient + diffuse, 1.0);
}
