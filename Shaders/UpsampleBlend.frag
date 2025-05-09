#version 460 core



layout(location = 0)in vec2 texCoord;
layout (location = 0) out vec4 upsample;


layout(set = 0, binding = 0)uniform sampler2D srcTexture;
layout(set = 0, binding = 1) uniform UniformBuffer
{

	vec4 m_mipchainDimensions;
	uint m_indexMipchain;
	float m_radius;
	uint m_pad1;
	uint m_pad2;

}lv_currentMipInfo;



void main()
{

    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    float x = lv_currentMipInfo.m_radius;
    float y = lv_currentMipInfo.m_radius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec4 a = texture(srcTexture, vec2(texCoord.x - x, texCoord.y + y));
    vec4 b = texture(srcTexture, vec2(texCoord.x,     texCoord.y + y));
    vec4 c = texture(srcTexture, vec2(texCoord.x + x, texCoord.y + y));

    vec4 d = texture(srcTexture, vec2(texCoord.x - x, texCoord.y));
    vec4 e = texture(srcTexture, vec2(texCoord.x,     texCoord.y));
    vec4 f = texture(srcTexture, vec2(texCoord.x + x, texCoord.y));

    vec4 g = texture(srcTexture, vec2(texCoord.x - x, texCoord.y - y));
    vec4 h = texture(srcTexture, vec2(texCoord.x,     texCoord.y - y));
    vec4 i = texture(srcTexture, vec2(texCoord.x + x, texCoord.y - y));

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;
    upsample.a = 1.f;
}