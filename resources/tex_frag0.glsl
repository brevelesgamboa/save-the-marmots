#version 330 core

uniform sampler2D Texture0;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;
uniform vec3 lightColor;

in vec2 vTexCoord;
in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;
out vec4 Outcolor;

void main()
{
    vec3 N = normalize(fragNor);
    vec3 L = normalize(lightDir);
    vec3 eyeDirection = normalize(-EPos);
    vec3 H = normalize(L + eyeDirection);
    float diffuse = max(dot(N, L), 0.0);
    float specular = 0.0;
    if (diffuse > 0.0) {
        specular = pow(max(dot(N, H), 0.0), MatShine);
    }
    // The image supplies surface color; GLSL still lights the ground per pixel.
    vec3 surfaceColor = texture(Texture0, vTexCoord).rgb;
    vec3 lighting = surfaceColor * (MatAmb + lightColor * MatDif * diffuse);
    Outcolor = vec4(lighting + lightColor * MatSpec * specular, 1.0);
}
