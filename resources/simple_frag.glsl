#version 330 core

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;
uniform vec3 lightColor;

in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;
out vec4 color;

void main()
{
    // Interpolated vectors must be normalized again for each pixel.
    vec3 N = normalize(fragNor);
    vec3 L = normalize(lightDir);
    vec3 eyeDirection = normalize(-EPos);
    vec3 H = normalize(L + eyeDirection);
    float diffuse = max(dot(N, L), 0.0);
    // A surface facing away from the light has no specular highlight.
    float specular = 0.0;
    if (diffuse > 0.0) {
        specular = pow(max(dot(N, H), 0.0), MatShine);
    }
    color = vec4(MatAmb + lightColor * (MatDif * diffuse + MatSpec * specular), 1.0);
}
