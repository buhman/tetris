#version 450
#extension GL_EXT_debug_printf : require

layout(binding = 0, std140) uniform dyn_t {
  mat4 model;
  mat4 view;
  vec4 color;
} dyn;

layout(binding = 2, std140) uniform ubo_t {
  mat4 view;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragVertColor;
layout(location = 2) out vec4 fragPos;

void main() {
  gl_Position = ubo.view * dyn.view * dyn.model * vec4(inPosition, 1.0);
  fragColor = dyn.color;
  fragVertColor = inColor;
  fragPos = dyn.model * vec4(inPosition, 1.0);
}
