#version 330 core

in  float opacity;
in  vec3  fragPos;
out vec4  FragColor;

uniform vec3  viewPos;
uniform vec3  fogColor;
uniform float fogDensity;

void main() {
    // gl_PointCoord = UV within the point sprite (0-1)
    // Use it to make circular particles instead of square ones
    // distance from centre of point
    vec2  coord  = gl_PointCoord - vec2(0.5);
    float dist   = length(coord);

    // Discard pixels outside the circle
    if (dist > 0.5) discard;

    // Soft edge — fade out toward the circle edge
    float alpha = opacity * (1.0 - smoothstep(0.15, 0.5, dist));  // was 0.3, now fades sooner
    // Bubble colour — slightly blue-white
    // Silt is handled by same shader, slightly more grey
    vec3 color = vec3(0.7, 0.85, 1.0);

    // Fog
    float fogDist   = length(viewPos - fragPos);
    float fogFactor = clamp(exp(-fogDensity * fogDist), 0.0, 1.0);
    color = mix(fogColor, color, fogFactor);

    FragColor = vec4(color, alpha);
}