#version 460 core




layout(location = 0) in vec2 texCoord;

layout (location = 0) out vec4 downsample;


layout(set = 0, binding = 0) uniform sampler2D lv_mipTexture;
layout(set = 0, binding = 1) uniform UniformBuffer
{

	vec4 m_mipchainDimensions;
	uint m_indexMipchain;
	uint m_pad0;
	uint m_pad1;
	uint m_pad2;

}lv_currentMipInfo;




void main()
{

    uint lv_mipIndex = lv_currentMipInfo.m_indexMipchain;

    
    vec2 srcTexelSize = 1.0 / lv_currentMipInfo.m_mipchainDimensions.xy;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;
    vec2 texCoord1 = texCoord;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===


      vec4 a = vec4(0.f);
      vec4 b = vec4(0.f);
      vec4 c = vec4(0.f);

      vec4 d = vec4(0.f);
      vec4 e = vec4(0.f);
      vec4 f = vec4(0.f);

      vec4 g = vec4(0.f);
      vec4 h = vec4(0.f);
      vec4 i = vec4(0.f);

      vec4 j = vec4(0.f);
      vec4 k = vec4(0.f);
      vec4 l = vec4(0.f);
      vec4 m = vec4(0.f);


    

        a = texture(lv_mipTexture, vec2(texCoord1.x - 2*x, texCoord1.y + 2*y));
        b = texture(lv_mipTexture, vec2(texCoord1.x,       texCoord1.y + 2*y));
        c = texture(lv_mipTexture, vec2(texCoord1.x + 2*x, texCoord1.y + 2*y));

        d = texture(lv_mipTexture, vec2(texCoord1.x - 2*x, texCoord1.y));
        e = texture(lv_mipTexture, vec2(texCoord1.x,       texCoord1.y));
        f = texture(lv_mipTexture, vec2(texCoord1.x + 2*x, texCoord1.y));

        g = texture(lv_mipTexture, vec2(texCoord1.x - 2*x, texCoord1.y - 2*y));
        h = texture(lv_mipTexture, vec2(texCoord1.x,       texCoord1.y - 2*y));
        i = texture(lv_mipTexture, vec2(texCoord1.x + 2*x, texCoord1.y - 2*y));

        j = texture(lv_mipTexture, vec2(texCoord1.x - x, texCoord1.y + y));
        k = texture(lv_mipTexture, vec2(texCoord1.x + x, texCoord1.y + y));
        l = texture(lv_mipTexture, vec2(texCoord1.x - x, texCoord1.y - y));
        m = texture(lv_mipTexture, vec2(texCoord1.x + x, texCoord1.y - y));

   
        
   


    downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;
    downsample.a = 1.f;

    //downsample = max(downsample, 0.0001f);
}