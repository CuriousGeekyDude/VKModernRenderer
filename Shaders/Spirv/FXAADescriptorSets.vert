#version 460 core




layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 lv_output;


layout(set = 0, binding = 0) uniform sampler2D screenTexture;




float rgb2luma(vec3 rgb){
    return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}


float QUALITY(int i)
{
    
    if( 1 <= i && i <= 4) {
        
        return 1.f;

    }
    
    if(i == 5) {
    
        return 1.5f;
    }

    if(6 <= i && i <= 9) {
        return 2.f;
    }

    if(i == 10) {
        return 4.f;
    }

    if(i == 11) {
        return 8.f;
    }

    return 1.f;

}



float EDGE_THRESHOLD_MIN = 0.0312f;
float EDGE_THRESHOLD_MAX = 0.125f;
float SUBPIXEL_QUALITY = 0.75f;
int ITERATIONS = 12;


void main()
{


	vec3 colorCenter = texture(screenTexture,uv).rgb;
    ivec2 lv_inverseScreenSize = textureSize(screenTexture, 0);
    vec2 inverseScreenSize = vec2(1.f/float(lv_inverseScreenSize.x), 1.f/(lv_inverseScreenSize.y));

    float lumaCenter = rgb2luma(colorCenter);

    float lumaDown = rgb2luma(textureOffset(screenTexture,uv,ivec2(0,-1)).rgb);
    float lumaUp = rgb2luma(textureOffset(screenTexture,uv,ivec2(0,1)).rgb);
    float lumaLeft = rgb2luma(textureOffset(screenTexture,uv,ivec2(-1,0)).rgb);
    float lumaRight = rgb2luma(textureOffset(screenTexture,uv,ivec2(1,0)).rgb);

    float lumaMin = min(lumaCenter,min(min(lumaDown,lumaUp),min(lumaLeft,lumaRight)));
    float lumaMax = max(lumaCenter,max(max(lumaDown,lumaUp),max(lumaLeft,lumaRight)));

    float lumaRange = lumaMax - lumaMin;

    if(lumaRange < max(EDGE_THRESHOLD_MIN,lumaMax*EDGE_THRESHOLD_MAX)){
        colorCenter = vec3(1.f) - exp(-colorCenter*2.5f);
        lv_output = vec4(colorCenter.rgb, 1.f);
        return;
    }
	

    float lumaDownLeft = rgb2luma(textureOffset(screenTexture,uv,ivec2(-1,-1)).rgb);
    float lumaUpRight = rgb2luma(textureOffset(screenTexture,uv,ivec2(1,1)).rgb);
    float lumaUpLeft = rgb2luma(textureOffset(screenTexture,uv,ivec2(-1,1)).rgb);
    float lumaDownRight = rgb2luma(textureOffset(screenTexture,uv,ivec2(1,-1)).rgb);

    float lumaDownUp = lumaDown + lumaUp;
    float lumaLeftRight = lumaLeft + lumaRight;

    float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
    float lumaDownCorners = lumaDownLeft + lumaDownRight;
    float lumaRightCorners = lumaDownRight + lumaUpRight;
    float lumaUpCorners = lumaUpRight + lumaUpLeft;

    float edgeHorizontal =  abs(-2.0 * lumaLeft + lumaLeftCorners)  + abs(-2.0 * lumaCenter + lumaDownUp ) * 2.0    + abs(-2.0 * lumaRight + lumaRightCorners);
    float edgeVertical =    abs(-2.0 * lumaUp + lumaUpCorners)      + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0  + abs(-2.0 * lumaDown + lumaDownCorners);

    bool isHorizontal = (edgeHorizontal >= edgeVertical);

    float luma1 = isHorizontal ? lumaDown : lumaLeft;
    float luma2 = isHorizontal ? lumaUp : lumaRight;
    float gradient1 = luma1 - lumaCenter;
    float gradient2 = luma2 - lumaCenter;

    bool is1Steepest = abs(gradient1) >= abs(gradient2);

    float gradientScaled = 0.25*max(abs(gradient1),abs(gradient2));

    float stepLength = isHorizontal ? inverseScreenSize.y : inverseScreenSize.x;

    float lumaLocalAverage = 0.0;

    if(is1Steepest){
        stepLength = - stepLength;
        lumaLocalAverage = 0.5*(luma1 + lumaCenter);
    } else {
        lumaLocalAverage = 0.5*(luma2 + lumaCenter);
    }

    vec2 currentUv = uv;
    if(isHorizontal){
    currentUv.y += stepLength * 0.5;
    } else {
        currentUv.x += stepLength * 0.5;
    }


    vec2 offset = isHorizontal ? vec2(inverseScreenSize.x,0.0) : vec2(0.0,inverseScreenSize.y);
    vec2 uv1 = currentUv - offset;
    vec2 uv2 = currentUv + offset;

    float lumaEnd1 = rgb2luma(texture(screenTexture,uv1).rgb);
    float lumaEnd2 = rgb2luma(texture(screenTexture,uv2).rgb);
    lumaEnd1 -= lumaLocalAverage;
    lumaEnd2 -= lumaLocalAverage;

    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    bool reachedBoth = reached1 && reached2;

    if(!reached1){
        uv1 -= offset;
    }
    if(!reached2){
        uv2 += offset;
    }
    
    if(!reachedBoth){

        for(int i = 2; i < ITERATIONS; i++){

            if(!reached1){
                lumaEnd1 = rgb2luma(texture(screenTexture, uv1).rgb);
                lumaEnd1 = lumaEnd1 - lumaLocalAverage;
            }

            if(!reached2){
                lumaEnd2 = rgb2luma(texture(screenTexture, uv2).rgb);
                lumaEnd2 = lumaEnd2 - lumaLocalAverage;
            }

            reached1 = abs(lumaEnd1) >= gradientScaled;
            reached2 = abs(lumaEnd2) >= gradientScaled;
            reachedBoth = reached1 && reached2;

            if(!reached1){
                uv1 -= offset * QUALITY(i);
            }
            if(!reached2){
                uv2 += offset * QUALITY(i);
            }

            if(reachedBoth){ break;}
        }
    }


    float distance1 = isHorizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);

    bool isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);

    float edgeThickness = (distance1 + distance2);

    float pixelOffset = - distanceFinal / edgeThickness + 0.5;


    bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;


    bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;

    float finalOffset = correctVariation ? pixelOffset : 0.0;

    float lumaAverage = (1.0/12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
    float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter)/lumaRange,0.0,1.0);
    float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

    finalOffset = max(finalOffset,subPixelOffsetFinal);

    vec2 finalUv = uv;
    if(isHorizontal){
        finalUv.y += finalOffset * stepLength;
    } else {
        finalUv.x += finalOffset * stepLength;
    }

    vec3 finalColor = texture(screenTexture,finalUv).rgb;
    finalColor = vec3(1.f) - exp(-finalColor*2.5f);
    lv_output = vec4(finalColor.rgb, 1.f);

}