#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPos;

out vec3 fragNor;
out vec3 lightDir;
out vec3 EPos;

void main()
{
    vec4 position = V * M * vec4(vertPos, 1.0);
    EPos = position.xyz;
    // The moon rock's nonuniform scale needs inverse-transpose in this shared shader.
    fragNor = transpose(inverse(mat3(V * M))) * vertNor;
    lightDir = (V * vec4(lightPos, 1.0)).xyz - EPos;
    // Projection is used only for placement, never for lighting vectors.
    gl_Position = P * position;
}
