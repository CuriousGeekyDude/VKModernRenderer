#version 460


layout (location=0) in vec2 lv_uv;

layout(location = 0) out vec4 lv_finalColor;



layout(set = 0, binding = 0) uniform UniformBuffer
{

	mat4   m_inMtx;
	mat4   m_viewMatrix;
	vec4   m_cameraPos;

}lv_cameraUniform;


struct Light
{
	vec4 m_position;
	vec4 m_color;

	float m_linear;
	float m_quadratic;
	float m_radius;
	float m_pad;
};


uint m_totalNumLights = 16;

layout(set = 0, binding = 1) readonly buffer LightData {Light lights[];} lv_lights;


layout(set = 0, binding = 2) uniform sampler2D lv_gbufferPos;
layout(set = 0, binding = 3) uniform sampler2D lv_gbufferNormal;
layout(set = 0, binding = 4) uniform sampler2D lv_gbufferAlbedoSpec;
layout(set = 0, binding = 5) uniform sampler2D lv_gbufferTangent;
layout(set = 0, binding = 6) uniform sampler2D lv_gbufferNormalVertex;
layout(set = 0, binding = 7) uniform sampler2D lv_occlusionFactor;




vec3 NormalSampleToWorldSpace(vec3 l_sampledNormal, vec3 l_normalVertex, vec3 l_tangent)
{
    vec3 lv_normalT = 2.f * l_sampledNormal - 1.f;

    vec3 N = l_normalVertex;
    vec3 T = normalize(l_tangent - dot(l_tangent, N)*N);
    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);

    return TBN*lv_normalT;
    
}





void main()
{

    float lv_occlusion = texture(lv_occlusionFactor ,lv_uv).r;

	vec4 lv_worldPos = vec4(texture(lv_gbufferPos, lv_uv).xyz, 1.f);
    vec4 lv_viewPos = lv_cameraUniform.m_viewMatrix * lv_worldPos;
	vec3 lv_albedo = texture(lv_gbufferAlbedoSpec, lv_uv).rgb;
    vec3 lv_lightning = lv_albedo*0.0004f*lv_occlusion;
    vec3 lv_fragPos = texture(lv_gbufferPos, lv_uv).rgb;
    vec3 lv_normal = NormalSampleToWorldSpace(texture(lv_gbufferNormal, lv_uv).rgb, texture(lv_gbufferNormalVertex, lv_uv).rgb, texture(lv_gbufferTangent, lv_uv).rgb);
    float lv_specular = texture(lv_gbufferAlbedoSpec, lv_uv).a;
    vec3 lv_dir = normalize(lv_cameraUniform.m_cameraPos.xyz - lv_fragPos);


	for(uint i = 0; i < m_totalNumLights; ++i) {

        Light lv_light = lv_lights.lights[i];

        vec3 lv_lightDir = normalize(lv_light.m_position.xyz - lv_fragPos);
        vec3 lv_diffuse = max(dot(lv_normal, lv_lightDir), 0.0) * lv_albedo * lv_light.m_color.rgb;
        
        //specular
        vec3 halfwayDir = normalize(lv_lightDir + lv_dir);  
        float spec = pow(max(dot(lv_normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lv_light.m_color.rgb * spec * lv_specular;

        // attenuation
        float distance = length(lv_light.m_position.xyz - lv_fragPos);
        float attenuation = 1.0 / (1.0 + lv_light.m_linear * distance + lv_light.m_quadratic * distance * distance);
        lv_diffuse *= attenuation;
        specular *= attenuation;
        lv_lightning += lv_diffuse + specular;

	}

    //lv_finalColor = vec4(lv_lightning, 1.f);

    lv_lightning = vec3(1.f) - exp(-lv_lightning*2.5f);

    lv_finalColor.rgb = pow(lv_lightning, vec3(1.f/2.2f));
    lv_finalColor.a = 1.f;

}