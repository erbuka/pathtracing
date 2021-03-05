#include "DebugRaytracer.h"

#include "Scene.h"
#include "Sampler.h"

namespace rt
{
	glm::vec3 DebugRaytracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		auto [result, node] = scene.CastRay(ray);

		if (result.Hit)
		{
			switch (CurrentMode)
			{
			case rt::DebugRaytracer::Mode::Color:
				return node->Material.Albedo->Sample(result.UV);
			case rt::DebugRaytracer::Mode::Normal:
				return result.Normal * 0.5f + 0.5f;
			default:
				break;
			}
		}
		else
		{
			switch (CurrentMode)
			{
			case rt::DebugRaytracer::Mode::Color:
				return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
			case rt::DebugRaytracer::Mode::Normal:
				return glm::vec3(0.0f);
			default:
				break;
			}
		}

	}
}