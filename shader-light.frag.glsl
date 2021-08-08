#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

const vec3 lightDir = vec3(0.4, 0.5, 0.7);

void main() {
  vec3 dX = dFdx(fragPos);
  vec3 dY = dFdy(fragPos);
  vec3 normal = normalize(cross(dX, dY));
  float light = max(0.0, dot(lightDir, normal));
  outColor = vec4(
    (vec3(0.1,0.1,0.1) * fragColor) + pow(light * fragColor, vec3(1.5)),
    1.0);

  /*outColor = vec4(fragColor, 1.0);*/
  /*outColor = vec4(fragTexCoord, 0.0, 1.0);*/
  /*outColor = texture(texSampler, vec2(-0.5, -0.5) + (fragTexCoord * 0.5));*/
}
