#version 310 es

precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor; // Vertex color

out vec3 fragColor; // Pass color to fragment shader

uniform mat4 VP;

void main()
{  
    gl_Position = VP *  vec4(aPos, 1.0);
    fragColor = aColor;
}