#shader vertex
#version 150 core
in vec2 position;
in vec3 color;
in vec2 texcoord;
out vec3 Color;
out vec2 Texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
  Color = color;
  Texcoord = texcoord;
  gl_Position = proj * view * model * vec4(position, 0.0, 1.0);
}

#shader fragment
#version 150 core
in vec3 Color;
in vec2 Texcoord;
out vec4 outColor;
uniform sampler2D tex;
void main()
{
  outColor = texture(tex, Texcoord) * vec4(Color, 1.0);
}
