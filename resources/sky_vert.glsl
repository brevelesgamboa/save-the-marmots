#version 330 core

uniform mat4 inversePV;
out vec3 skyDirection;

void main()
{
    // One triangle covers the screen; its corners require no vertex buffer.
    vec2 corners[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
    vec2 corner = corners[gl_VertexID];
    vec4 direction = inversePV * vec4(corner, 1.0, 1.0);
    skyDirection = direction.xyz / direction.w;
    gl_Position = vec4(corner, 1.0, 1.0);
}
