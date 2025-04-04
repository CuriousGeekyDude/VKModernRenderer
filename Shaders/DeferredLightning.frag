#version 460


layout (location=0) in vec2 lv_uv;

layout(location = 0) out vec4 lv_finalColor;



layout(set = 0, binding = 0) uniform UniformBuffer
{

	mat4   m_inMtx;
	mat4   m_viewMatrix;
	vec4   m_cameraPos;
    vec4   m_time;

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


uint m_totalNumLights = 1;
const float PI = 3.14159265359;



layout(set = 0, binding = 1) readonly buffer LightData {Light lights[];} lv_lights;


layout(set = 0, binding = 2) uniform sampler2D lv_gbufferPos;
layout(set = 0, binding = 3) uniform sampler2D lv_gbufferNormal;
layout(set = 0, binding = 4) uniform sampler2D lv_gbufferAlbedoSpec;
layout(set = 0, binding = 5) uniform sampler2D lv_gbufferTangent;
layout(set = 0, binding = 6) uniform sampler2D lv_gbufferNormalVertex;
layout(set = 0, binding = 7) uniform sampler2D lv_occlusionFactor;
layout(set = 0, binding = 8) uniform sampler2D lv_gbufferMetallic;
layout(set = 0, binding = 9) uniform samplerCube  lv_depthMapLight;

layout(set = 0,binding = 10) uniform  UniformBuffer2 { 

	mat4 m_viewMatrixSun;
	mat4 m_orthoMatrixSun;
	vec4 m_posSun;

} ubo;

layout(set = 0, binding = 11) uniform sampler2D lv_depth;



vec3 sampleOffsetDirections[59] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1),
   vec3(-1.f, -1.f, -1.f), vec3(-1.f, 1.f, 1.f), vec3(1.f, 1.f, -1.f),
   vec3(1.f, -1.f, 1.f), vec3( 1,  2,  0), vec3(-1, -2,  0), vec3( 2,  1,  0),
   vec3(-2, -1,  0), vec3( 1,  0,  2), vec3(-1,  0, -2), vec3( 0,  2,  1), vec3( 0, -2, -1),
   vec3( 2,  0,  1),
vec3(-2,  0, -1),
vec3( 0,  1,  2),
vec3( 0, -1, -2),
vec3( 1,  2,  1),
vec3(-1, -2, -1),
vec3( 2,  1, -1),
vec3(-2, -1,  1),
vec3( 1, -2,  2),
vec3(-1,  2, -2),
vec3(  0.276,  0.447,  0.851),
vec3( -0.724,  0.447,  0.526),
vec3( -0.724,  0.447, -0.526),
vec3(  0.276,  0.447, -0.851),
vec3(  0.724, -0.447,  0.526),
vec3( -0.276, -0.447,  0.851),
vec3( -0.894, -0.447,  0.000),
vec3( -0.276, -0.447, -0.851),
vec3(  0.724, -0.447, -0.526),
vec3(  0.309,  0.951,  0.000),
vec3( -0.809,  0.588,  0.000),
vec3(  0.809, -0.588,  0.000),
vec3( -0.309, -0.951,  0.000),
vec3(  0.000,  0.309,  0.951),
vec3(  0.000, -0.309,  0.951),
vec3(  0.000,  0.309, -0.951),
vec3(  0.000, -0.309, -0.951)
); 

float ShadowCalculation(vec3 lv_worldPos, vec3 lv_normal, vec3 lv_lightPos)
{
    //vec3 lv_lightPos = ubo.m_posSun.xyz;

    vec3 lv_dirVector = lv_worldPos - lv_lightPos;

    float lv_depthFragToLight = length(lv_dirVector);



    float shadow = 0.0;
    float bias   = max(0.05 * (1.0 - dot(lv_normal, lv_dirVector)), 0.005);
    int samples  = 59;
    float viewDistance = length(lv_cameraUniform.m_cameraPos.xyz - lv_worldPos);
    float diskRadius = (1.0 + (viewDistance / 100)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(lv_depthMapLight, lv_dirVector + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= 100;   // undo mapping [0;1]
        if(lv_depthFragToLight - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples); 

    
    
    return shadow;
}



vec3 NormalSampleToWorldSpace(vec3 l_sampledNormal, vec3 l_normalVertex, vec3 l_tangent)
{
    vec3 lv_normalT = 2.f * l_sampledNormal - 1.f;

    vec3 N = l_normalVertex;
    vec3 T = normalize(l_tangent - dot(l_tangent, N)*N);
    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);

    return TBN*lv_normalT;
    
}



float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / (denom+0.008f);
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / (denom +0.008f);
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------



float heaviside( float v ) {
    if ( v > 0.0 ) return 1.0;
    else return 0.0;
}



void main()
{

    float lv_occlusion = texture(lv_occlusionFactor ,lv_uv).r;
    float lv_metallic = texture(lv_gbufferMetallic ,lv_uv).b;
    float lv_roughness = texture(lv_gbufferMetallic ,lv_uv).g;
    vec3 lv_normal = NormalSampleToWorldSpace(texture(lv_gbufferNormal, lv_uv).rgb, texture(lv_gbufferNormalVertex, lv_uv).rgb, texture(lv_gbufferTangent, lv_uv).rgb);
    vec4 lv_albedo = texture(lv_gbufferAlbedoSpec, lv_uv);
    
    

	vec4 lv_worldPos = vec4(texture(lv_gbufferPos, lv_uv).xyz, 1.f);
    vec4 lv_viewPos = lv_cameraUniform.m_viewMatrix * lv_worldPos;
    vec3 lv_fragPos = texture(lv_gbufferPos, lv_uv).rgb;
    vec3 lv_dir = normalize(lv_cameraUniform.m_cameraPos.xyz - lv_fragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, lv_albedo.rgb, lv_metallic);


    float lv_shadow = ShadowCalculation(lv_worldPos.xyz, lv_normal, lv_lights.lights[0].m_position.xyz);

    vec3 lv_lightning = lv_albedo.rgb * lv_occlusion * 0.01f;

    vec3 Lo = vec3(0.0f);
    //float alpha = pow(lv_roughness, 2.0);
	for(uint i = 0; i < m_totalNumLights; ++i) {
        
        vec3 lv_lightPos = lv_lights.lights[i].m_position.xyz;
        vec3 L = normalize(lv_lightPos - lv_fragPos);
        vec3 H = normalize(lv_dir + L);
   

        // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#specular-brdf
        //float NdotH = dot(lv_normal, H);
        //float alpha_squared = alpha * alpha;
        //float d_denom = ( NdotH * NdotH ) * ( alpha_squared - 1.0 ) + 1.0;
        //float distribution = ( alpha_squared * heaviside( NdotH ) ) / ( PI * d_denom * d_denom );

        //float NdotL = clamp( dot(lv_normal, L), 0, 1 );

        //if ( NdotL > 1e-5 ) {
            //float NdotV = dot(lv_normal, lv_worldPos.xyz);
            //float HdotL = dot(H, L);
            //float HdotV = dot(H, lv_worldPos.xyz);

            //float visibility = ( heaviside( HdotL ) / ( abs( NdotL ) + sqrt( alpha_squared + ( 1.0 - alpha_squared ) * ( NdotL * NdotL ) ) ) ) * ( heaviside( HdotV ) / ( abs( NdotV ) + sqrt( alpha_squared + ( 1.0 - alpha_squared ) * ( NdotV * NdotV ) ) ) );

            //float specular_brdf = visibility * distribution;

            //vec3 diffuse_brdf = (1 / PI) * lv_albedo.rgb;

            // NOTE(marco): f0 in the formula notation refers to the base colour here
            //vec3 conductor_fresnel = specular_brdf * ( lv_albedo.rgb + ( 1.0 - lv_albedo.rgb ) * pow( 1.0 - abs( HdotV ), 5 ) );

            // NOTE(marco): f0 in the formula notation refers to the value derived from ior = 1.5
            //float f0 = 0.04; // pow( ( 1 - ior ) / ( 1 + ior ), 2 )
            //float fr = f0 + ( 1 - f0 ) * pow(1 - abs( HdotV ), 5 );
            //vec3 fresnel_mix = mix( diffuse_brdf, vec3( specular_brdf ), fr );

            //vec3 material_colour = mix( fresnel_mix, conductor_fresnel, lv_metallic );

            //material_colour *= lv_occlusion;

            //Lo += material_colour;
        //} else {
            //Lo += lv_albedo.rgb * lv_occlusion;
        //}










        float distance = length(lv_lightPos - lv_fragPos);
        float attenuation = 1.0 / distance*distance;
        vec3 radiance = vec3(1.f, 1.f, 1.f) * 0.28 * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(lv_normal, H, lv_roughness);   
        float G   = GeometrySmith(lv_normal, lv_dir, L, lv_roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, lv_dir), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(lv_normal, lv_dir), 0.0) * max(dot(lv_normal, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        
        vec3 kD = vec3(1.0) - kS;
        
        kD *= 1.0 - lv_metallic;	  

        float NdotL = max(dot(lv_normal, L), 0.0);        

        Lo += (kD * lv_albedo.rgb / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

	}




    //lv_finalColor = vec4(lv_lightning, 1.f);
   // lv_lightning += (lv_diffuse + specular)*lv_albedo.rgb;

    //lv_lightning = vec3(1.f) - exp(-lv_lightning*2.5f);


    //lv_lightning /= (lv_lightning + vec3(1.f));

   //lv_finalColor.rgb = pow(lv_lightning, vec3(1.f/2.2f));

   lv_lightning += (1.f - lv_shadow) * Lo;
   lv_finalColor.rgb = lv_lightning;
   lv_finalColor.a = lv_albedo.a;

}