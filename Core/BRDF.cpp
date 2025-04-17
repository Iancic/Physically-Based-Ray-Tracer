#include "precomp.h"
#include "BRDF.h"

BRDF* BRDF::brdf_Instance = nullptr;

BRDF::BRDF()
{

}

BRDF::~BRDF()
{

}

float BRDF::luminance(float3 rgb)
{
	return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 BRDF::baseColorToSpecularF0(const float3 baseColor, const float metalness, const float reflectance)
{
	UNREFERENCED_PARAMETER(reflectance);
#if USE_REFLECTANCE_PARAMETER
	const float minDielectricsF0 = 0.16f * reflectance * reflectance;
#else
	const float minDielectricsF0 = MIN_DIELECTRICS_F0;
#endif
	return lerp(float3(minDielectricsF0, minDielectricsF0, minDielectricsF0), baseColor, metalness);
}

float3 BRDF::baseColorToDiffuseReflectance(float3 baseColor, float metalness)
{
	return baseColor * (1.0f - metalness);
}

float3 BRDF::evalVoid(const BrdfData data)
{
	UNREFERENCED_PARAMETER(data);
	return float3(0.0f, 0.0f, 0.0f);
}

float4 BRDF::getRotationToZAxis(float3 input)
{
	// Handle special case when input is exact or near opposite of (0, 0, 1)
	if (input.z < -0.99999f) return float4(1.0f, 0.0f, 0.0f, 0.0f);

	return normalize(float4(input.y, -input.x, 0.0f, 1.0f + input.z));
}

float4 BRDF::invertRotation(float4 q)
{
	return float4(-q.x, -q.y, -q.z, q.w);
}

float3 BRDF::rotatePoint(float4 q, float3 v)
{
	float3 qAxis = float3(q.x, q.y, q.z);
	return 2.0f * dot(qAxis, v) * qAxis + (q.w * q.w - dot(qAxis, qAxis)) * v + 2.0f * q.w * cross(qAxis, v);
}

float3 BRDF::sampleHemisphere(float2 u, float pdf)
{

	float a = sqrt(u.x);
	float b = TWO_PI * u.y;

	float3 result = float3(
		a * cos(b),
		a * sin(b),
		sqrt(1.0f - u.x));

	pdf = result.z * ONE_OVER_PI;

	return result;
}

float3 BRDF::sampleHemisphere(float2 u)
{
	float pdf{};
	return sampleHemisphere(u, pdf);
}

float3 BRDF::evalFresnelSchlick(float3 f0, float f90, float NdotS)
{
	return f0 + (f90 - f0) * pow(1.0f - NdotS, 5.0f);
}

float3 BRDF::evalFresnelSchlickSphericalGaussian(float3 f0, float f90, float NdotV)
{
	return f0 + (f90 - f0) * exp2((-5.55473f * NdotV - 6.983146f) * NdotV);
}

float3 BRDF::evalFresnel(float3 f0, float f90, float NdotS)
{
	// Default is Schlick's approximation
	return evalFresnelSchlick(f0, f90, NdotS);
}

float BRDF::shadowedF90(float3 F0)
{
	const float t = (1.0f / MIN_DIELECTRICS_F0);
	return min(1.0f, t * luminance(F0));
}

float BRDF::lambertian(const BrdfData data)
{
	UNREFERENCED_PARAMETER(data);
	return 1.0f;
}

float3 BRDF::evalLambertian(const BrdfData data)
{
	return data.diffuseReflectance * (ONE_OVER_PI * data.NdotL);
}

float BRDF::Smith_G_a(float alpha, float NdotS)
{
	return NdotS / (max(0.00001f, alpha) * sqrt(1.0f - min(0.99999f, NdotS * NdotS)));
}

float BRDF::Smith_G_Lambda_GGX(float a)
{
	return (-1.0f + sqrt(1.0f + (1.0f / (a * a)))) * 0.5f;
}

float BRDF::Smith_G_Lambda_Beckmann_Walter(float a)
{
	if (a < 1.6f) {
		return (1.0f - (1.259f - 0.396f * a) * a) / ((3.535f + 2.181f * a) * a);
		//return ((1.0f + (2.276f + 2.577f * a) * a) / ((3.535f + 2.181f * a) * a)) - 1.0f; //< Walter's original
	}
	else {
		return 0.0f;
	}
}

float BRDF::Smith_G1_General(float a)
{
	return 1.0f / (1.0f + Smith_G_Lambda(a));
}

float BRDF::Smith_G1_GGX(float a)
{
	float a2 = a * a;
	return 2.0f / (sqrt((a2 + 1.0f) / a2) + 1.0f);
}

float BRDF::Smith_G1_GGX(float alpha, float NdotS, float alphaSquared, float NdotSSquared)
{
	UNREFERENCED_PARAMETER(NdotS);
	(void)alpha;
	return 2.0f / (sqrt(((alphaSquared * (1.0f - NdotSSquared)) + NdotSSquared) / NdotSSquared) + 1.0f);
}

float BRDF::Smith_G2_Height_Correlated(float alpha, float NdotL, float NdotV)
{
	float aL = Smith_G_a(alpha, NdotL);
	float aV = Smith_G_a(alpha, NdotV);
	return 1.0f / (1.0f + Smith_G_Lambda(aL) + Smith_G_Lambda(aV));
}

float BRDF::Smith_G2_Separable_GGX_Lagarde(float alphaSquared, float NdotL, float NdotV)
{
	float a = NdotV + sqrt(alphaSquared + NdotV * (NdotV - alphaSquared * NdotV));
	float b = NdotL + sqrt(alphaSquared + NdotL * (NdotL - alphaSquared * NdotL));
	return 1.0f / (a * b);
}

float BRDF::Smith_G2_Height_Correlated_GGX_Lagarde(float alphaSquared, float NdotL, float NdotV)
{
	float a = NdotV * sqrt(alphaSquared + NdotL * (NdotL - alphaSquared * NdotL));
	float b = NdotL * sqrt(alphaSquared + NdotV * (NdotV - alphaSquared * NdotV));
	return 0.5f / (a + b);
}

float BRDF::Smith_G2_Height_Correlated_GGX_Hammon(float alpha, float NdotL, float NdotV)
{
	return 0.5f / (lerp(2.0f * NdotL * NdotV, NdotL + NdotV, alpha));
}

float BRDF::Smith_G2_Over_G1_Height_Correlated(float alpha, float alphaSquared, float NdotL, float NdotV)
{
	float G1V = Smith_G1(alpha, NdotV, alphaSquared, NdotV * NdotV);
	float G1L = Smith_G1(alpha, NdotL, alphaSquared, NdotL * NdotL);
	return G1L / (G1V + G1L - G1V * G1L);
}

float BRDF::Smith_G2(float alpha, float alphaSquared, float NdotL, float NdotV)
{
	UNREFERENCED_PARAMETER(alpha);
#if USE_OPTIMIZED_G2 && (MICROFACET_DISTRIBUTION == GGX)
#if USE_HEIGHT_CORRELATED_G2
#define G2_DIVIDED_BY_DENOMINATOR 1
	return Smith_G2_Height_Correlated_GGX_Lagarde(alphaSquared, NdotL, NdotV);
#else
#define G2_DIVIDED_BY_DENOMINATOR 1
	return Smith_G2_Separable_GGX_Lagarde(alphaSquared, NdotL, NdotV);
#endif
#else
#if USE_HEIGHT_CORRELATED_G2
	return Smith_G2_Height_Correlated(alpha, NdotL, NdotV);
#else
	return Smith_G2_Separable(alpha, NdotL, NdotV);
#endif
#endif

}

float BRDF::Beckmann_D(float alphaSquared, float NdotH)
{
	float cos2Theta = NdotH * NdotH;
	float numerator = exp((cos2Theta - 1.0f) / (alphaSquared * cos2Theta));
	float denominator = PI * alphaSquared * cos2Theta * cos2Theta;
	return numerator / denominator;
}

float BRDF::GGX_D(float alphaSquared, float NdotH)
{
	float b = ((alphaSquared - 1.0f) * NdotH * NdotH + 1.0f);
	return alphaSquared / (PI * b * b);
}

float3 BRDF::sampleGGXVNDF(float3 Ve, float2 alpha2D, float2 u)
{

	// Section 3.2: transforming the view direction to the hemisphere configuration
	float3 Vh = normalize(float3(alpha2D.x * Ve.x, alpha2D.y * Ve.y, Ve.z));

#if USE_VNDF_WITH_SPHERICAL_CAPS

	// Source: "Sampling Visible GGX Normals with Spherical Caps" by Dupuy & Benyoub

	// Sample a spherical cap in (-Vh.z, 1]
	float phi = 2.0f * PI * u.x;
	float z = ((1.0f - u.y) * (1.0f + Vh.z)) - Vh.z;
	float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
	float x = sinTheta * cos(phi);
	float y = sinTheta * sin(phi);

	// compute halfway direction;
	float3 Nh = float3(x, y, z) + Vh;

#else

	// Source: "Sampling the GGX Distribution of Visible Normals" by Heitz
	// See also https://hal.inria.fr/hal-00996995v1/document and http://jcgt.org/published/0007/04/01/

	// Section 4.1: orthonormal basis (with special case if cross product is zero)
	float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
	float3 T1 = lensq > 0.0f ? float3(-Vh.y, Vh.x, 0.0f) * rsqrt(lensq) : float3(1.0f, 0.0f, 0.0f);
	float3 T2 = cross(Vh, T1);

	// Section 4.2: parameterization of the projected area
	float r = sqrt(u.x);
	float phi = TWO_PI * u.y;
	float t1 = r * cos(phi);
	float t2 = r * sin(phi);
	float s = 0.5f * (1.0f + Vh.z);
	t2 = lerp(sqrt(1.0f - t1 * t1), t2, s);

	// Section 4.3: reprojection onto hemisphere
	float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;

#endif

	// Section 3.4: transforming the normal back to the ellipsoid configuration
	return normalize(float3(alpha2D.x * Nh.x, alpha2D.y * Nh.y, max(0.0f, Nh.z)));
}

float BRDF::sampleGGXVNDFReflectionPdf(float alpha, float alphaSquared, float NdotH, float NdotV, float LdotH)
{
	UNREFERENCED_PARAMETER(LdotH);
	NdotH = max(0.00001f, NdotH);
	NdotV = max(0.00001f, NdotV);
	return (GGX_D(max(0.00001f, alphaSquared), NdotH) * Smith_G1_GGX(alpha, NdotV, alphaSquared, NdotV * NdotV)) / (4.0f * NdotV);
}

float BRDF::waltersTrick(float alpha, float NdotV)
{
	return (1.2f - 0.2f * sqrt(abs(NdotV))) * alpha;
}

float BRDF::sampleWalterReflectionPdf(float alpha, float alphaSquared, float NdotH, float NdotV, float LdotH)
{
	UNREFERENCED_PARAMETER(NdotV);
	UNREFERENCED_PARAMETER(alpha);
	NdotH = max(0.00001f, NdotH);
	LdotH = max(0.00001f, LdotH);
	return Microfacet_D(max(0.00001f, alphaSquared), NdotH) * NdotH / (4.0f * LdotH);
}

float3 BRDF::sampleBeckmannWalter(float3 Vlocal, float2 alpha2D, float2 u)
{
	UNREFERENCED_PARAMETER(Vlocal);
	float alpha = dot(alpha2D, float2(0.5f, 0.5f));

	// Equations (28) and (29) from Walter's paper for Beckmann distribution
	float tanThetaSquared = -(alpha * alpha) * log(1.0f - u.x);
	float phi = TWO_PI * u.y;

	// Calculate cosTheta and sinTheta needed for conversion to H vector
	float cosTheta = rsqrt(1.0f + tanThetaSquared);
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

	// Convert sampled spherical coordinates to H vector
	return normalize(float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta));
}

float3 BRDF::sampleGGXWalter(float3 Vlocal, float2 alpha2D, float2 u)
{
	UNREFERENCED_PARAMETER(Vlocal);
	float alpha = dot(alpha2D, float2(0.5f, 0.5f));
	float alphaSquared = alpha * alpha;

	// Calculate cosTheta and sinTheta needed for conversion to H vector
	float cosThetaSquared = (1.0f - u.x) / ((alphaSquared - 1.0f) * u.x + 1.0f);
	float cosTheta = sqrt(cosThetaSquared);
	float sinTheta = sqrt(1.0f - cosThetaSquared);
	float phi = TWO_PI * u.y;

	// Convert sampled spherical coordinates to H vector
	return normalize(float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta));
}

float BRDF::specularSampleWeightGGXVNDF(float alpha, float alphaSquared, float NdotL, float NdotV, float HdotL, float NdotH)
{
	UNREFERENCED_PARAMETER(NdotH);
	UNREFERENCED_PARAMETER(HdotL);
#if USE_HEIGHT_CORRELATED_G2
	return Smith_G2_Over_G1_Height_Correlated(alpha, alphaSquared, NdotL, NdotV);
#else 
	return Smith_G1_GGX(alpha, NdotL, alphaSquared, NdotL * NdotL);
#endif
}

float BRDF::specularSampleWeightBeckmannWalter(float alpha, float alphaSquared, float NdotL, float NdotV, float HdotL, float NdotH)
{
	return (HdotL * Smith_G2(alpha, alphaSquared, NdotL, NdotV)) / (NdotV * NdotH);
}

float BRDF::specularSampleWeightGGXWalter(float alpha, float alphaSquared, float NdotL, float NdotV, float HdotL, float NdotH)
{
#if USE_OPTIMIZED_G2 
	return (NdotL * HdotL * Smith_G2(alpha, alphaSquared, NdotL, NdotV) * 4.0f) / NdotH;
#else
	return (HdotL * Smith_G2(alpha, alphaSquared, NdotL, NdotV)) / (NdotV * NdotH);
#endif
}

float3 BRDF::sampleSpecularMicrofacet(float3 Vlocal, float alpha, float alphaSquared, float3 specularF0, float2 u, float3 weight)
{

	// Sample a microfacet normal (H) in local space
	float3 Hlocal;
	if (alpha == 0.0f) {
		// Fast path for zero roughness (perfect reflection), also prevents NaNs appearing due to divisions by zeroes
		Hlocal = float3(0.0f, 0.0f, 1.0f);
	}
	else {
		// For non-zero roughness, this calls VNDF sampling for GG-X distribution or Walter's sampling for Beckmann distribution
		Hlocal = sampleSpecularHalfVector(Vlocal, float2(alpha, alpha), u);
	}

	//TODO: SIMD THIS? max min and dot
	// Reflect view direction to obtain light vector
	float3 Llocal = reflect(-Vlocal, Hlocal);

	// Note: HdotL is same as HdotV here
	// Clamp dot products here to small value to prevent numerical instability. Assume that rays incident from below the hemisphere have been filtered
	float HdotL = max(0.00001f, min(1.0f, dot(Hlocal, Llocal)));
	const float3 Nlocal = float3(0.0f, 0.0f, 1.0f);
	float NdotL = max(0.00001f, min(1.0f, dot(Nlocal, Llocal)));
	float NdotV = max(0.00001f, min(1.0f, dot(Nlocal, Vlocal)));
	float NdotH = max(0.00001f, min(1.0f, dot(Nlocal, Hlocal)));
	float3 F = evalFresnel(specularF0, shadowedF90(specularF0), HdotL);

	// Calculate weight of the sample specific for selected sampling method 
	// (this is microfacet BRDF divided by PDF of sampling method - notice how most terms cancel out)
	weight = F * specularSampleWeight(alpha, alphaSquared, NdotL, NdotV, HdotL, NdotH);

	return Llocal;
}

float3 BRDF::evalMicrofacet(const BrdfData data)
{
	float D = Microfacet_D(max(0.00001f, data.alphaSquared), data.NdotH);
	float G2 = Smith_G2(data.alpha, data.alphaSquared, data.NdotL, data.NdotV);
	float3 F = evalFresnel(data.specularF0, shadowedF90(data.specularF0), data.VdotH); //< Unused, F is precomputed already

#if G2_DIVIDED_BY_DENOMINATOR
	return data.F * (G2 * D * data.NdotL);
#else
	return ((data.F * G2 * D) / (4.0f * data.NdotL * data.NdotV)) * data.NdotL;
#endif
}

BrdfData BRDF::prepareBRDFData(float3 N, float3 L, float3 V, MaterialProperties material)
{
	BrdfData data;

	// Evaluate VNHL vectors
	data.V = V;
	data.N = N;
	data.H = normalize(L + V);
	data.L = L;

	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	data.Vbackfacing = (NdotV <= 0.0f);
	data.Lbackfacing = (NdotL <= 0.0f);

	// Clamp NdotS to prevent numerical instability. Assume vectors below the hemisphere will be filtered using 'Vbackfacing' and 'Lbackfacing' flags
	data.NdotL = min(max(0.00001f, NdotL), 1.0f);
	data.NdotV = min(max(0.00001f, NdotV), 1.0f);

	data.LdotH = saturate(dot(L, data.H));
	data.NdotH = saturate(dot(N, data.H));
	data.VdotH = saturate(dot(V, data.H));

	// Convert base color from sRGB to linear
	float3 linearBaseColor = srgbToLinear(material.baseColor);

	// Unpack material properties
	data.specularF0 = baseColorToSpecularF0(material.baseColor, material.metalness);
	data.diffuseReflectance = baseColorToDiffuseReflectance(material.baseColor, material.metalness);

	// Unpack 'perceptively linear' -> 'linear' -> 'squared' roughness
	data.roughness = material.roughness;
	data.alpha = material.roughness * material.roughness;
	data.alphaSquared = data.alpha * data.alpha;

	// Pre-calculate some more BRDF terms
	data.F = evalFresnel(data.specularF0, shadowedF90(data.specularF0), data.LdotH);

	return data;
}

float3 BRDF::evalCombinedBRDF(float3 N, float3 L, float3 V, MaterialProperties material)
{
	// Prepare data needed for BRDF evaluation - unpack material properties and evaluate commonly used terms (e.g. Fresnel, NdotL, ...)
	const BrdfData data = prepareBRDFData(N, L, V, material);

	// Ignore V and L rays "below" the hemisphere
	if (data.Vbackfacing || data.Lbackfacing) return float3(0.0f, 0.0f, 0.0f);

	// Eval specular and diffuse BRDFs
	float3 specular = evalSpecular(data);
	float3 diffuse = evalDiffuse(data);

	return (float3(1.0f, 1.0f, 1.0f) - data.F) * diffuse + specular;
}

bool BRDF::evalIndirectCombinedBRDF(float2 u, float3 shadingNormal, float3 geometryNormal, float3 V, MaterialProperties material, const int brdfType, float3& rayDirection, float3& sampleWeight)
{
	UNREFERENCED_PARAMETER(geometryNormal);
	// Ignore incident ray coming from "below" the hemisphere
	//if (dot(shadingNormal, V) <= 0.0f) return false;

	// Transform view direction into local space of our sampling routines 
	// (local space is oriented so that its positive Z axis points along the shading normal)
	float4 qRotationToZ = getRotationToZAxis(shadingNormal);
	float3 Vlocal = rotatePoint(qRotationToZ, V);
	const float3 Nlocal = float3(0.0f, 0.0f, 1.0f);

	float3 rayDirectionLocal = float3(0.0f, 0.0f, 0.0f);

	if (brdfType == DIFFUSE_TYPE) 
	{
		// Sample diffuse ray using cosine-weighted hemisphere sampling 
		rayDirectionLocal = sampleHemisphere(u);
		const BrdfData data = prepareBRDFData(Nlocal, rayDirectionLocal, Vlocal, material);

		// Function 'diffuseTerm' is predivided by PDF of sampling the cosine weighted hemisphere
		sampleWeight = data.diffuseReflectance * diffuseTerm(data);
		
		// Sample a half-vector of specular BRDF. Note that we're reusing random variable 'u' here, but correctly it should be an new independent random number
		float3 Hspecular = sampleSpecularHalfVector(Vlocal, float2(data.alpha, data.alpha), u);

		// Clamp HdotL to small value to prevent numerical instability. Assume that rays incident from below the hemisphere have been filtered
		// Note: VdotH is always positive for VNDF sampling so we don't need to test if it's positive like we do for sampling with Walter's method
		float VdotH = max(0.00001f, min(1.0f, dot(Vlocal, Hspecular)));
		sampleWeight *= (float3(1.0f, 1.0f, 1.0f) - evalFresnel(data.specularF0, shadowedF90(data.specularF0), VdotH));

	}
	else if (brdfType == SPECULAR_TYPE) 
	{
		const BrdfData data = prepareBRDFData(Nlocal, float3(0.0f, 0.0f, 1.0f) /* unused L vector */, Vlocal, material);
		rayDirectionLocal = sampleSpecular(Vlocal, data.alpha, data.alphaSquared, data.specularF0, u, sampleWeight);
	}

	// Prevent tracing direction with no contribution
	if (luminance(sampleWeight) == 0.0f) return false;

	// Transform sampled direction Llocal back to V vector space
	rayDirection = normalize(rotatePoint(invertRotation(qRotationToZ), rayDirectionLocal));

	// Prevent tracing direction "under" the hemisphere (behind the triangle)
	//if (dot(geometryNormal, rayDirection) <= 0.0f) return false;

	return true;
}

float BRDF::getBrdfProbability(MaterialProperties material, float3 V, float3 shadingNormal)
{
	// Compute specular reflectance at normal incidence (F0) based on material properties
	float specularF0 = luminance(baseColorToSpecularF0(material.baseColor, material.metalness));
	float diffuseReflectance = luminance(baseColorToDiffuseReflectance(material.baseColor, material.metalness));

	// Compute Fresnel term using shading normal (approximate)
	float fresnelFactor = max(0.0f, dot(V, shadingNormal));
	float fresnel = saturate(luminance(evalFresnel(specularF0, shadowedF90(specularF0), fresnelFactor)));

	// Reduce the impact of specular (favoring diffuse)
	float adjustedFresnel = fresnel * 0.5f;  // Reduce Fresnel influence on BRDF selection

	// Compute specular and diffuse contributions
	float specular = adjustedFresnel;
	float diffuse = diffuseReflectance * (1.0f - adjustedFresnel) * 1.5f; // Boost diffuse term

	// Compute probability of selecting specular over diffuse
	float probability = specular / max(0.0001f, (specular + diffuse));

	// Clamp probability to avoid undersampling of less prominent BRDF
	return clamp(probability, 0.05f, 0.7f); // Shift range more towards diffuse
}
float3 BRDF::srgbToLinear(float3 c)
{
	return float3(
		(c.x <= 0.04045f) ? (c.x / 12.92f) : pow((c.x + 0.055f) / 1.055f, 2.4f),
		(c.y <= 0.04045f) ? (c.y / 12.92f) : pow((c.y + 0.055f) / 1.055f, 2.4f),
		(c.z <= 0.04045f) ? (c.z / 12.92f) : pow((c.z + 0.055f) / 1.055f, 2.4f)
	);
}
