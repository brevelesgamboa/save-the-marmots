#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPos;

out vec2 vTexCoord;
out vec3 fragNor;
out vec3 lightDir;
out vec3 EPos;

void main()
{
    // Use the same view space as the untextured shader.
    vec4 position = V * M * vec4(vertPos, 1.0);
    EPos = position.xyz;
    // The ground uses M = identity; the fragment shader restores normal length.
    fragNor = (V * M * vec4(vertNor, 0.0)).xyz;
    lightDir = (V * vec4(lightPos, 1.0)).xyz - EPos;
    vTexCoord = vertTex;
    gl_Position = P * position;
}
