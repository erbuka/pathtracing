#include "pathtracer.h"

#include <spdlog/spdlog.h>

#include <glm/gtx/norm.hpp>

#include "scene.h"

namespace rt
{

	glm::vec3 pathtracer::trace(const view_parameters& params, const ray& r, const scene& scene)
	{
		return trace_recursive(params, r, scene, 5);
	}

	glm::vec3 pathtracer::trace_recursive(const view_parameters& params, const ray& r, const scene& scene, uint32_t recursion)
	{
		if (recursion == 0)
		{
			// Could return the background, but it wouldn't be correct
			return glm::vec3(0.0f);
		}
		else
		{
			// Cast the ray on the scene
			const auto [result, node] = scene.cast_ray(r);

			if (result.hit)
			{
				// Gather material properties
				const auto albedo = node->material.albedo->sample(result.uv);
				const auto emission = node->material.emission->sample(result.uv);
				const float roughness = node->material.roughness->sample(result.uv).r;
				const float metallic = node->material.metallic->sample(result.uv).r;

				// Compute a random ray on the hemisphere + a perfect reflection ray
				const auto hemi_dir = m_rng.hemisphere(result.normal);
				const auto reflect_dir = glm::reflect(r.direction, result.normal);

				// Mix the perfect reflection and the random ray based on the roughness of the material
				// This approach is not described anywhere, but works pretty well for handling the roughness
				const auto dir = glm::normalize(glm::mix(reflect_dir, hemi_dir, roughness));

				ray reflected_ray = {
					result.position + dir * s_epsilon,
					dir
				};

				// Compute the lighting (Lambert BRDF)
				const auto cos_theta = glm::max(0.0f, glm::dot(reflected_ray.direction, result.normal));

				const auto radiance = trace_recursive(params, reflected_ray, scene, recursion - 1);

				// The albedo is mixed with white based on the metalness of the surface. A metallic surface
				// should only reflect light
				auto color = emission + glm::mix(albedo, glm::vec3(1.0f), metallic) * radiance * cos_theta * 2.0f;

				return color;
			}
			else
			{	
				// If nothing is hit, sample the background
				return scene.background->sample(r.direction);
			}
		}
	}
}