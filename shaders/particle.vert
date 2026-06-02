#version 330 core

// Particle data — position, size, opacity
layout (location = 0) in vec3  aPos;
layout (location = 1) in float aSize;
layout (location = 2) in float aOpacity;

out float opacity;
out vec3  fragPos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    fragPos     = aPos;
    opacity     = aOpacity;

    gl_Position  = projection * view * vec4(aPos, 1.0);

    // gl_PointSize sets the pixel size of this point
    // Divide by gl_Position.w to make size perspective-correct
    // (further particles appear smaller)
    gl_PointSize = aSize * 80.0 / gl_Position.w;
}