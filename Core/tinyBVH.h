#pragma once
#define NO_DOUBLE_PRECISION_SUPPORT
#define TINYBVH_USE_CUSTOM_VECTOR_TYPES
namespace tinybvh
{
	using bvhint2 = Tmpl8::int2;
	using bvhint3 = Tmpl8::int3;
	using bvhuint2 = Tmpl8::uint2;
	using bvhvec2 = Tmpl8::float2;
	using bvhvec3 = Tmpl8::float3;
	using bvhvec4 = Tmpl8::float4;
}
#include "tiny_bvh.h"