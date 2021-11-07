#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 1) uniform sampler textureSampler;
layout(binding = 2) uniform texture2D[] textures;
layout(binding = 3) uniform UniformBufferLighting
{
	mat4 invModelView;
	mat4 lightSpaceMatrix;
    vec4 directionDirectionalLight;
	vec4 colorDirectionalLight;
} ubLighting;
layout(binding = 4) uniform texture2D shadowMap;
layout(binding = 5) uniform sampler shadowMapSampler;

layout(location = 0) in vec3 inViewPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) flat in uint inMaterialID;
layout(location = 3) in mat3 inTBN;

layout(location = 0) out vec4 outColor;

vec3 sRGBToLinear(vec3 color)
{
	if (length(color) <= 0.04045)
    	return color / 12.92;
	else
		return pow((color + vec3(0.055)) / 1.055, vec3(2.4));
}

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

vec2 vogelDiskSample(int sampleIndex, int samplesCount, float phi)
{
  float GoldenAngle = 2.4;

  float r = sqrt(sampleIndex + 0.5) / sqrt(samplesCount);
  float theta = sampleIndex * GoldenAngle + phi;

  float sine = sin(theta);
  float cosine = cos(theta);
  
  return vec2(r * cosine, r * sine);
}

float interleavedGradientNoise(vec2 position_screen)
{
  vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
  return fract(magic.z * fract(dot(position_screen, magic.xy)));
}

const float BIAS = 0.001;

float textureProj(vec2 shadowMapCoords, float currentDepth)
{
	float closestDepth = texture(sampler2D(shadowMap, shadowMapSampler), shadowMapCoords).r;

    float shadow = currentDepth - BIAS > closestDepth  ? 1.0 : 0.0;

	return shadow;
}

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float isShadowed(in vec3 viewPos)
{
    vec4 rawPos = ubLighting.invModelView * vec4(viewPos, 1.0);
    vec4 posLightSpace = biasMat * ubLighting.lightSpaceMatrix * rawPos;
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    int nIteration = 16;
    float divisor = 700.0; 

    /*float r = 0.0;
    for(int i = 0; i < nIteration; ++i)
    {
        vec3 tempProjCoords = projCoords + vec3(vogelDiskSample(i, nIteration, interleavedGradientNoise(viewPos.xy)) / divisor, 0.0);
        vec2 projInt = vec2(tempProjCoords.x, tempProjCoords.y);
        r += textureProj(projInt, tempProjCoords.z);
    }
    r /= 16.0;*/

    return textureProj(projCoords.xy, projCoords.z);
}

const float PI = 3.14159265359;

void main() 
{
	vec3 normal = (texture(sampler2D(textures[inMaterialID * 5 + 1], textureSampler), inTexCoord).rgb * 2.0 - vec3(1.0)) * inTBN;
	normal = normalize(normal);
	vec3 albedo = sRGBToLinear(texture(sampler2D(textures[inMaterialID * 5], textureSampler), inTexCoord).rgb);
	float roughness = texture(sampler2D(textures[inMaterialID * 5 + 2], textureSampler), inTexCoord).r;
    float metallic = texture(sampler2D(textures[inMaterialID * 5 + 3], textureSampler), inTexCoord).r;

	vec3 V = normalize(-inViewPos);
    vec3 R = reflect(-V, normal);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0,albedo, metallic);

    vec3 Lo = vec3(0.0);

    // calculate per-light radiance
    vec3 L = normalize(-ubLighting.directionDirectionalLight.xyz);
    vec3 H = normalize(V + L);
    vec3 radiance = ubLighting.colorDirectionalLight.xyz;

    // cook-torrance brdf
    float NDF = DistributionGGX(normal, H, roughness);
    float G   = GeometrySmith(normal, V, L, roughness);
    vec3 F    = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
    vec3 specular     = nominator / max(denominator, 0.001);

    // add to outgoing radiance Lo
    float NdotL = max(dot(normal, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = albedo * 0.00;
	float shadow = isShadowed(inViewPos);
   	Lo *= 1.0 - shadow;

    vec3 color = ambient + Lo;

	outColor = vec4(color, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
