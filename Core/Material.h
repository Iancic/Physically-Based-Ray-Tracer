#pragma once
#include "../template/tmpl8math.h"

class Material
{

public:

	Material();
	~Material();

	float3 baseColor; // Color used for diffuse, subsurface, metallic and transmission
	float metalness; // Blends between dielectric and metal

	float3 emissive; // Color of light emission from the surface
	float roughness; // Specifies the the roughness of the microfacet

	float transmissivness; 
	float reflectance; 
	float opacity;

	// --- Old System ---
	enum class TYPE
	{
		DIFFUSE,
		METAL,
		DIELECTRIC,
		EMMISIVE,
	};
	TYPE materialType;
	// ------------------

private:

};