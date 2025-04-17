// The code samples inspired from "Crash Course in BRDF Implementation" article and repository:
// https://boksajak.github.io/files/CrashCourseBRDF.pdf

#pragma once
#define NONE 0

// NDF definitions
#define GGX 1
#define BECKMANN 2

// Specular BRDFs
#define MICROFACET 1
#define PHONG 2

// Diffuse BRDFs
#define LAMBERTIAN 1
#define OREN_NAYAR 2
#define DISNEY 3
#define FROSTBITE 4

// BRDF types
#define DIFFUSE_TYPE 1
#define SPECULAR_TYPE 2

// PIs
#ifndef PI
#define PI 3.141592653589f
#endif

#ifndef TWO_PI
#define TWO_PI (2.0f * PI)
#endif

#ifndef ONE_OVER_PI
#define ONE_OVER_PI (1.0f / PI)
#endif

#ifndef ONE_OVER_TWO_PI
#define ONE_OVER_TWO_PI (1.0f / TWO_PI)
#endif

#ifndef MICROFACET_DISTRIBUTION
#define MICROFACET_DISTRIBUTION GGX
//#define MICROFACET_DISTRIBUTION BECKMANN
#endif

// Specify default specular and diffuse BRDFs
#ifndef SPECULAR_BRDF
#define SPECULAR_BRDF MICROFACET
//#define SPECULAR_BRDF PHONG
//#define SPECULAR_BRDF NONE
#endif

#ifndef DIFFUSE_BRDF
#define DIFFUSE_BRDF LAMBERTIAN
//#define DIFFUSE_BRDF OREN_NAYAR
//#define DIFFUSE_BRDF DISNEY
//#define DIFFUSE_BRDF FROSTBITE
//#define DIFFUSE_BRDF NONE
#endif

// Specifies minimal reflectance for dielectrics (when metalness is zero)
// Nothing has lower reflectance than 2%, but we use 4% to have consistent results with UE4, Frostbite, et al.
// Note: only takes effect when USE_REFLECTANCE_PARAMETER is not defined
#define MIN_DIELECTRICS_F0 0.4f

// Define this to use minimal reflectance (F0) specified per material, instead of global MIN_DIELECTRICS_F0 value
//#define USE_REFLECTANCE_PARAMETER 1

// Enable this to weigh diffuse by Fresnel too, otherwise specular and diffuse will be simply added together
// (this is disabled by default for Frostbite diffuse which is normalized to combine well with GGX Specular BRDF)
#if DIFFUSE_BRDF != FROSTBITE
#define COMBINE_BRDFS_WITH_FRESNEL 1
#endif

// Enable optimized G2 implementation which includes division by specular BRDF denominator (not available for all NDFs, check macro G2_DIVIDED_BY_DENOMINATOR if it was actually used)
#define USE_OPTIMIZED_G2 1

// Enable height correlated version of G2 term. Separable version will be used otherwise
#define USE_HEIGHT_CORRELATED_G2 1

// Select distribution function
#if MICROFACET_DISTRIBUTION == GGX
#define Microfacet_D GGX_D
#elif MICROFACET_DISTRIBUTION == BECKMANN
#define Microfacet_D Beckmann_D
#endif

// Select G functions (masking/shadowing) depending on selected distribution
#if MICROFACET_DISTRIBUTION == GGX
#define Smith_G_Lambda Smith_G_Lambda_GGX
#elif MICROFACET_DISTRIBUTION == BECKMANN
#define Smith_G_Lambda Smith_G_Lambda_Beckmann_Walter
#endif

#ifndef Smith_G1
// Define version of G1 optimized specifically for selected NDF
#if MICROFACET_DISTRIBUTION == GGX
#define Smith_G1 Smith_G1_GGX
#elif MICROFACET_DISTRIBUTION == BECKMANN
#define Smith_G1 Smith_G1_Beckmann_Walter
#endif
#endif

// Select default specular and diffuse BRDF functions
#if SPECULAR_BRDF == MICROFACET
#define evalSpecular evalMicrofacet
#define sampleSpecular sampleSpecularMicrofacet
#if MICROFACET_DISTRIBUTION == GGX
#if USE_WALTER_GGX_SAMPLING
#define sampleSpecularHalfVector sampleGGXWalter
#else
#define sampleSpecularHalfVector sampleGGXVNDF
#endif
#else
#define sampleSpecularHalfVector sampleBeckmannWalter
#endif
#elif SPECULAR_BRDF == PHONG
#define evalSpecular evalPhong
#define sampleSpecular sampleSpecularPhong
#define sampleSpecularHalfVector samplePhong
#else
#define evalSpecular evalVoid
#define sampleSpecular sampleSpecularVoid
#define sampleSpecularHalfVector sampleSpecularHalfVectorVoid
#endif

#if MICROFACET_DISTRIBUTION == GGX
#if USE_WALTER_GGX_SAMPLING
#define specularSampleWeight specularSampleWeightGGXWalter
#else
#define specularSampleWeight specularSampleWeightGGXVNDF
#endif
#if USE_WALTER_GGX_SAMPLING
#define specularPdf sampleWalterReflectionPdf
#else
#define specularPdf sampleGGXVNDFReflectionPdf
#endif
#else
#define specularSampleWeight specularSampleWeightBeckmannWalter
#define specularPdf sampleWalterReflectionPdf
#endif

#if DIFFUSE_BRDF == LAMBERTIAN
#define evalDiffuse evalLambertian
#define diffuseTerm lambertian
#elif DIFFUSE_BRDF == OREN_NAYAR
#define evalDiffuse evalOrenNayar
#define diffuseTerm orenNayar
#elif DIFFUSE_BRDF == DISNEY
#define evalDiffuse evalDisneyDiffuse
#define diffuseTerm disneyDiffuse
#elif DIFFUSE_BRDF == FROSTBITE
#define evalDiffuse evalFrostbiteDisneyDiffuse
#define diffuseTerm frostbiteDisneyDiffuse
#else
#define evalDiffuse evalVoid
#define evalIndirectDiffuse evalIndirectVoid
#define diffuseTerm none
#endif

inline float rsqrt(float x) { return 1.0f / sqrtf(x); }
inline float saturate(float x) { return clamp(x, 0.0f, 1.0f); }

struct MaterialProperties
{
	float3 baseColor;
	float metalness;

	float3 emissive;
	float roughness;

	float transmissivness = 0.f;
	float reflectance = 0.5f;
	float opacity = 1.f;
};

struct BrdfData
{
	// Material properties
	float3 specularF0;
	float3 diffuseReflectance;

	// Roughnesses
	float roughness;    //< perceptively linear roughness (artist's input)
	float alpha;        //< linear roughness - often 'alpha' in specular BRDF equations
	float alphaSquared; //< alpha squared - pre-calculated value commonly used in BRDF equations

	// Commonly used terms for BRDF evaluation
	float3 F; //< Fresnel term

	// Vectors
	float3 V; //< Direction to viewer (or opposite direction of incident ray)
	float3 N; //< Shading normal
	float3 H; //< Half vector (microfacet normal)
	float3 L; //< Direction to light (or direction of reflecting ray)

	float NdotL;
	float NdotV;

	float LdotH;
	float NdotH;
	float VdotH;

	// True when V/L is backfacing wrt. shading normal N
	bool Vbackfacing;
	bool Lbackfacing;
};

class BRDF
{
private:
	static BRDF* brdf_Instance;

	BRDF();
public:
	~BRDF();

	BRDF(const BRDF&) = delete;
	BRDF& operator=(const BRDF&) = delete;

	// Get the instance
	static BRDF* getInstance()
	{
		if (brdf_Instance == nullptr)
		{
			brdf_Instance = new BRDF();
		}
		return brdf_Instance;
	}

	float luminance(float3 rgb);
	
	float3 baseColorToSpecularF0(const float3 baseColor, const float metalness, const float reflectance = 0.5f);

	float3 baseColorToDiffuseReflectance(float3 baseColor, float metalness);

	float3 evalVoid(const BrdfData data);

	// Calculates rotation quaternion from input vector to the vector (0, 0, 1)
	// Input vector must be normalized!
	float4 getRotationToZAxis(float3 input);
	
	// Returns the quaternion with inverted rotation
	float4 invertRotation(float4 q);

	float3 rotatePoint(float4 q, float3 v);

	// Samples a direction within a hemisphere oriented along +Z axis with a cosine-weighted distribution 
	// Source: "Sampling Transformations Zoo" in Ray Tracing Gems by Shirley et al.
	float3 sampleHemisphere(float2 u, float pdf);

	float3 sampleHemisphere(float2 u);

	// Schlick's approximation to Fresnel term
	// f90 should be 1.0, except for the trick used by Schuler (see 'shadowedF90' function)
	float3 evalFresnelSchlick(float3 f0, float f90, float NdotS);

	// Schlick's approximation to Fresnel term calculated using spherical gaussian approximation
	// Source: https://seblagarde.wordpress.com/2012/06/03/spherical-gaussien-approximation-for-blinn-phong-phong-and-fresnel/ by Lagarde
	float3 evalFresnelSchlickSphericalGaussian(float3 f0, float f90, float NdotV);

	float3 evalFresnel(float3 f0, float f90, float NdotS);

	float shadowedF90(float3 F0);

	float lambertian(const BrdfData data);

	float3 evalLambertian(const BrdfData data);

	float Smith_G_a(float alpha, float NdotS);

	// Lambda function for Smith G term derived for GGX distribution
	float Smith_G_Lambda_GGX(float a);

	float Smith_G_Lambda_Beckmann_Walter(float a);

	// Smith G1 term (masking function)
	// This non-optimized version uses NDF specific lambda function (G_Lambda) resolved bia macro based on selected NDF
	float Smith_G1_General(float a);

	// Smith G1 term (masking function) optimized for GGX distribution (by substituting G_Lambda_GGX into G1)
	float Smith_G1_GGX(float a);

	// Smith G1 term (masking function) further optimized for GGX distribution (by substituting G_a into G1_GGX)
	float Smith_G1_GGX(float alpha, float NdotS, float alphaSquared, float NdotSSquared);

	// Smith G2 term (masking-shadowing function)
	// Height correlated version - non-optimized, uses G_Lambda functions for selected NDF
	float Smith_G2_Height_Correlated(float alpha, float NdotL, float NdotV);

	float Smith_G2_Separable_GGX_Lagarde(float alphaSquared, float NdotL, float NdotV);

	float Smith_G2_Height_Correlated_GGX_Lagarde(float alphaSquared, float NdotL, float NdotV);

	float Smith_G2_Height_Correlated_GGX_Hammon(float alpha, float NdotL, float NdotV);

	float Smith_G2_Over_G1_Height_Correlated(float alpha, float alphaSquared, float NdotL, float NdotV);

	// Evaluates G2 for selected configuration (GGX/Beckmann, optimized/non-optimized, separable/height-correlated)
	// Note that some paths aren't optimized too much...
	// Also note that when USE_OPTIMIZED_G2 is specified, returned value will be: G2 / (4 * NdotL * NdotV) if GG-X is selected
	float Smith_G2(float alpha, float alphaSquared, float NdotL, float NdotV);

	float Beckmann_D(float alphaSquared, float NdotH);
	
	float GGX_D(float alphaSquared, float NdotH);

	// Samples a microfacet normal for the GGX distribution using VNDF method.
	// Source: "Sampling the GGX Distribution of Visible Normals" by Heitz
	// Source: "Sampling Visible GGX Normals with Spherical Caps" by Dupuy & Benyoub
	// Random variables 'u' must be in <0;1) interval
	// PDF is 'G1(NdotV) * D'
	float3 sampleGGXVNDF(float3 Ve, float2 alpha2D, float2 u);

	// PDF of sampling a reflection vector L using 'sampleGGXVNDF'.
	// Note that PDF of sampling given microfacet normal is (G1 * D) when vectors are in local space (in the hemisphere around shading normal). 
	// Remaining terms (1.0f / (4.0f * NdotV)) are specific for reflection case, and come from multiplying PDF by jacobian of reflection operator
	float sampleGGXVNDFReflectionPdf(float alpha, float alphaSquared, float NdotH, float NdotV, float LdotH);

	// "Walter's trick" is an adjustment of alpha value for Walter's sampling to reduce maximal weight of sample to about 4
	// Source: "Microfacet Models for Refraction through Rough Surfaces" by Walter et al., page 8
	 float waltersTrick(float alpha, float NdotV);

	// PDF of sampling a reflection vector L using 'sampleBeckmannWalter' or 'sampleGGXWalter'.
	// Note that PDF of sampling given microfacet normal is (D * NdotH). Remaining terms (1.0f / (4.0f * LdotH)) are specific for
	// reflection case, and come from multiplying PDF by jacobian of reflection operator
	 float sampleWalterReflectionPdf(float alpha, float alphaSquared, float NdotH, float NdotV, float LdotH);

	// Samples a microfacet normal for the Beckmann distribution using walter's method.
	// Source: "Microfacet Models for Refraction through Rough Surfaces" by Walter et al.
	// PDF is 'D * NdotH'
	 float3 sampleBeckmannWalter(float3 Vlocal, float2 alpha2D, float2 u);

	// Samples a microfacet normal for the GG-X distribution using walter's method.
	// Source: "Microfacet Models for Refraction through Rough Surfaces" by Walter et al.
	// PDF is 'D * NdotH'
	 float3 sampleGGXWalter(float3 Vlocal, float2 alpha2D, float2 u);

	// Weight for the reflection ray sampled from GGX distribution using VNDF method
	 float specularSampleWeightGGXVNDF(float alpha, float alphaSquared, float NdotL, float NdotV, float HdotL, float NdotH);

	// Weight for the reflection ray sampled from Beckmann distribution using Walter's method
	 float specularSampleWeightBeckmannWalter(float alpha, float alphaSquared, float NdotL, float NdotV, float HdotL, float NdotH);

	// Weight for the reflection ray sampled from GGX distribution using Walter's method
	 float specularSampleWeightGGXWalter(float alpha, float alphaSquared, float NdotL, float NdotV, float HdotL, float NdotH);

	// Samples a reflection ray from the rough surface using selected microfacet distribution and sampling method
	// Resulting weight includes multiplication by cosine (NdotL) term
	 float3 sampleSpecularMicrofacet(float3 Vlocal, float alpha, float alphaSquared, float3 specularF0, float2 u, float3 weight);

	// Evaluates microfacet specular BRDF
	float3 evalMicrofacet(const BrdfData data);

	BrdfData prepareBRDFData(float3 N, float3 L, float3 V, MaterialProperties material);
	 
	// This is an entry point for evaluation of all other BRDFs based on selected configuration (for direct light)
	float3 evalCombinedBRDF(float3 N, float3 L, float3 V, MaterialProperties material);

	// This is an entry point for evaluation of all other BRDFs based on selected configuration (for indirect light)
	bool evalIndirectCombinedBRDF(float2 u, float3 shadingNormal, float3 geometryNormal, float3 V, MaterialProperties material, const int brdfType, float3& rayDirection, float3& sampleWeight);

	float getBrdfProbability(MaterialProperties material, float3 V, float3 shadingNormal);

	float3 srgbToLinear(float3 c);

};