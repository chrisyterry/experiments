#version 450

layout(location = 0) out vec4 outColor; // fragment color output; location specifies framebuffer index
layout(location = 0) in vec3 fragColor; // fragment color input

// ran for each fragment
void main() {
    outColor = vec4(fragColor, 1.0); // output channels in [0, 1] range; each fragment will have interpolated protion of color
}