#include "DebugRaytracer.h"

#include "Scene.h"
#include "Sampler.h"

namespace rt::utility
{
	glm::vec3 DebugRaytracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		auto [result, node] = scene.CastRay(ray);

		if (result.Hit)
		{
			switch (CurrentMode)
			{
			case DebugRaytracer::Mode::Albedo:
				return node->Material.Albedo->Sample(result.UV);
			case DebugRaytracer::Mode::Emission:
				return node->Material.Emission->Sample(result.UV);
			case DebugRaytracer::Mode::Metallic:
				return node->Material.Metallic->Sample(result.UV);
			case DebugRaytracer::Mode::Roughness:
				return node->Material.Roughness->Sample(result.UV);
			case DebugRaytracer::Mode::Normal:
				return result.Normal * 0.5f + 0.5f;
			}
		}
		else
		{
			return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
		}
	}
}