#version 330 core

uniform sampler2D Texture0;
in vec3 skyDirection;
out vec4 color;

void main()
{
    // Wrap the image around the sky using longitude and latitude.
    vec3 direction = normalize(skyDirection);
    const float pi = 3.14159265;
    vec2 uv = vec2(atan(direction.z, direction.x) / (2.0 * pi) + 0.5,
                   asin(clamp(direction.y, -1.0, 1.0)) / pi + 0.5);
    // Repeat the square texture so stars keep their proportions and stay sharp.
    uv *= vec2(4.0, 2.0);
    color = vec4(texture(Texture0, uv).rgb, 1.0);
}
