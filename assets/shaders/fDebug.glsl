#version 310 es

precision mediump float;

in vec3 fragColor; // Interpolated color
out vec4 FragColor;

void main()
{    
    FragColor = vec4(fragColor, 1.0);
}