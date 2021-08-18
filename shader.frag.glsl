#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragVertColor;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

const float SRGB_ALPHA = 0.055;

float linear_to_srgb(float channel) {
    if(channel <= 0.0031308)
        return 12.92 * channel;
    else
        return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;
}

float srgb_to_linear(float channel) {
    if (channel <= 0.04045)
        return channel / 12.92;
    else
        return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);
}

vec3 rgb_to_srgb(vec3 rgb) {
    return vec3(
        linear_to_srgb(rgb.r),
        linear_to_srgb(rgb.g),
        linear_to_srgb(rgb.b)
    );
}

vec3 srgb_to_rgb(vec3 srgb) {
    return vec3(
        srgb_to_linear(srgb.r),
        srgb_to_linear(srgb.g),
        srgb_to_linear(srgb.b)
    );
}

const vec3 lightDir = vec3(0.76, 0.8, -0.5);

void main() {
  vec3 dX = dFdx((2 * fragPos) - 1);
  vec3 dY = dFdy((2 * fragPos) - 1);
  vec3 normal = normalize(cross(dX, dY));
  float light = max(0.0, dot(lightDir, normal));

  //outColor = vec4(srgb_to_rgb(fragColor), 1.0);
  outColor = vec4(0.8 * fragColor + pow(light * fragColor, vec3(1.3)), 1.0);
  /*outColor = texture(texSampler, vec2(-0.5, -0.5) + (fragTexCoord * 0.5));*/
}
