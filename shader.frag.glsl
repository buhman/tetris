#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragVertColor;
layout(location = 2) in vec4 fragPos;

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

const vec3 lightDir = vec3(0.5, 0.6, -0.9);

void main() {
  vec3 dX = dFdx(fragPos.xyz);
  vec3 dY = dFdy(fragPos.xyz);
  vec3 normal = normalize(cross(dX, dY));
  float light = max(0.0, dot(lightDir, normal));

  if (fragVertColor.x < 1.0f)
    outColor = vec4(0.0f);
  else {
    vec3 _color = vec3(srgb_to_rgb(fragColor.xyz) * fragVertColor);
    //outColor = vec4(_color, fragColor.w);
    outColor = vec4(pow(light * _color, vec3(1.5)), fragColor.w);
  }
  //
  /*outColor = texture(texSampler, vec2(-0.5, -0.5) + (fragTexCoord * 0.5));*/
}
