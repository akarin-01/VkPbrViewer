#version 450

layout(set = 0, binding = 0) uniform sampler2D offlineTex;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 offline = texture(offlineTex, fragTexCoord).rgb;
    outColor = vec4(ACESFilm(offline), 1.0);
}
