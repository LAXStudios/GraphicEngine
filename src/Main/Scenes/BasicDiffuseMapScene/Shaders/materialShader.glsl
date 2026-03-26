#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
  TexCoords = aTexCoords;
  FragPos = vec3(model * vec4(aPos, 1.0f));
  Normal = mat3(transpose(inverse(model))) * aNormal;
  gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#shader fragment 
#version 330 core

struct Material {
  sampler2D diffuse;
  sampler2D specular;
  sampler2D emission;

  float shininess;
};

struct Light {
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform Material material;
uniform Light light;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

void main()
{
  // ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));

  // diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light.position - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));

  // specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));

  // emission
  vec3 emission = vec3(0.0);
  if(texture(material.specular, TexCoords).r == 0.0)
  {
    emission = texture(material.emission, TexCoords).rgb;
  }

  vec3 result = ambient + diffuse + specular + emission;
  FragColor = vec4(result, 1.0);
}
