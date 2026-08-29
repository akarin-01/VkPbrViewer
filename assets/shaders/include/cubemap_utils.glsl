#ifndef CUBEMAP_UTILS_GLSL
#define CUBEMAP_UTILS_GLSL

// Face, faceCoord -> World dir(cube)
vec3 GetDirection(uint face, vec2 faceCoord)
{
    switch (face)
    {
    case 0u: return normalize(vec3( 1.0, -faceCoord.y, -faceCoord.x));      // +X
    case 1u: return normalize(vec3(-1.0, -faceCoord.y,  faceCoord.x));      // -X
    case 2u: return normalize(vec3( faceCoord.x,  1.0,  faceCoord.y));      // +Y
    case 3u: return normalize(vec3( faceCoord.x, -1.0, -faceCoord.y));      // -Y
    case 4u: return normalize(vec3( faceCoord.x, -faceCoord.y,  1.0));      // +Z
    case 5u: return normalize(vec3(-faceCoord.x, -faceCoord.y, -1.0));      // -Z
    }
    return vec3(0.0);
}

#endif
