#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DMS image;

uniform int SCR_WIDTH;
uniform int SCR_HEIGHT;

uniform bool horizontal;
uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main() {
    vec2 viewPortDim = vec2(SCR_WIDTH, SCR_HEIGHT);
    // ivec2 coord = ivec2(viewPortDim * TexCoords);
    ivec2 coord = ivec2(TexCoords);
    //vec2 tex_offset = 1.0 / viewPortDim;
    vec2 tex_offset = 1.0 / textureSize(image); // gets size of single texel

    vec3 sample0 = texelFetch(image, coord, 0).rgb;
    vec3 sample1 = texelFetch(image, coord, 1).rgb;
    vec3 sample2 = texelFetch(image, coord, 2).rgb;
    vec3 sample3 = texelFetch(image, coord, 3).rgb;

    vec3 result = 0.25 * (sample0 + sample1 + sample2 + sample3);
    result = result * weight[0];

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            sample0 = texelFetch(image, coord + ivec2(tex_offset.x * i, 0.0), 0).rgb;
            sample1 = texelFetch(image, coord + ivec2(tex_offset.x * i, 0.0), 1).rgb;
            sample2 = texelFetch(image, coord + ivec2(tex_offset.x * i, 0.0), 2).rgb;
            sample3 = texelFetch(image, coord + ivec2(tex_offset.x * i, 0.0), 3).rgb;

            result += 0.25 * (sample0 + sample1 + sample2 + sample3) * weight[i];

            sample0 = texelFetch(image, coord - ivec2(tex_offset.x * i, 0.0), 0).rgb;
            sample1 = texelFetch(image, coord - ivec2(tex_offset.x * i, 0.0), 1).rgb;
            sample2 = texelFetch(image, coord - ivec2(tex_offset.x * i, 0.0), 2).rgb;
            sample3 = texelFetch(image, coord - ivec2(tex_offset.x * i, 0.0), 3).rgb;

            result += 0.25 * (sample0 + sample1 + sample2 + sample3) * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            sample0 = texelFetch(image, coord + ivec2(0.0, tex_offset.y * i), 0).rgb;
            sample1 = texelFetch(image, coord + ivec2(0.0, tex_offset.y * i), 1).rgb;
            sample2 = texelFetch(image, coord + ivec2(0.0, tex_offset.y * i), 2).rgb;
            sample3 = texelFetch(image, coord + ivec2(0.0, tex_offset.y * i), 3).rgb;

            result += 0.25 * (sample0 + sample1 + sample2 + sample3) * weight[i];

            sample0 = texelFetch(image, coord - ivec2(0.0, tex_offset.y * i), 0).rgb;
            sample1 = texelFetch(image, coord - ivec2(0.0, tex_offset.y * i), 1).rgb;
            sample2 = texelFetch(image, coord - ivec2(0.0, tex_offset.y * i), 2).rgb;
            sample3 = texelFetch(image, coord - ivec2(0.0, tex_offset.y * i), 3).rgb;

            result += 0.25 * (sample0 + sample1 + sample2 + sample3) * weight[i];
        }
    }

    FragColor = vec4(result, 1.0);
}