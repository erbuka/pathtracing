#include "DebugPathtracer.h"

#include "Scene.h"
#include "Sampler.h"

namespace rt::utility
{
	glm::vec3 DebugPathtracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		auto [result, node] = scene.CastRay(ray);

		if (result.Hit)
		{
			switch (CurrentMode)
			{
			case DebugPathtracer::Mode::Albedo:
				return node->Material.Albedo->Sample(result.UV);
			case DebugPathtracer::Mode::Emission:
				return node->Material.Emission->Sample(result.UV);
			case DebugPathtracer::Mode::Metallic:
				return node->Material.Metallic->Sample(result.UV);
			case DebugPathtracer::Mode::Roughness:
				return node->Material.Roughness->Sample(result.UV);
			case DebugPathtracer::Mode::Normal:
				return result.Normal * 0.5f + 0.5f;

			}
		}
		else
		{
			return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
		}
	}
}