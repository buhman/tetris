#version 450
#extension GL_EXT_debug_printf : require

layout(binding = 0, std140) uniform model_t {
  mat4 mat;
  vec3 color;
} model;

layout(binding = 2, std140) uniform view_t {
  mat4 mat;
} view;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;

void main() {
  vec4 size = vec4(0.05, 0.05, 0.05, 1.0);

  gl_Position = view.mat * model.mat * (vec4(inPosition, 1.0) * size);
  fragColor = model.color;
  fragTexCoord = inTexCoord;
  fragPos = gl_Position.xyz;
}
