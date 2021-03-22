#include "debug_pathtracer.h"

#include "scene.h"
#include "sampler.h"

namespace rt::utility
{
	glm::vec3 debug_pathtracer::trace(const view_parameters& params, const ray& ray, const scene& scene)
	{
		auto [result, node] = scene.cast_ray(ray);

		if (result.hit)
		{
			switch (current_mode)
			{
			case debug_pathtracer::mode::albedo:
				return node->material.albedo->sample(result.uv);
			case debug_pathtracer::mode::emission:
				return node->material.emission->sample(result.uv);
			case debug_pathtracer::mode::metallic:
				return node->material.metallic->sample(result.uv);
			case debug_pathtracer::mode::roughness:
				return node->material.roughness->sample(result.uv);
			case debug_pathtracer::mode::normal:
				return result.normal * 0.5f + 0.5f;

			}
		}
		else
		{
			return scene.background ? scene.background->sample(ray.direction) : glm::vec3(0.0f);
		}
	}
}