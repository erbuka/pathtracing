#include "Pathtracer.h"

#include <spdlog/spdlog.h>

#include <glm/gtx/norm.hpp>

#include "Scene.h"

namespace rt
{

	glm::vec3 Pathtracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		return TraceRecursive(params, ray, scene, 5);
	}

	glm::vec3 Pathtracer::TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion)
	{
		if (recursion == 0)
		{
			return scene.Background->Sample(ray.Direction);
		}
		else
		{
			const auto [result, node] = scene.CastRay(ray);

			if (result.Hit)
			{
				const auto albedo = node->Material.Albedo->Sample(result.UV);
				const auto emission = node->Material.Emission->Sample(result.UV);
				const float roughness = node->Material.Roughness->Sample(result.UV).r;
				const float metallic = node->Material.Metallic->Sample(result.UV).r;

				// Reflected ray on hemisphere

				const auto hemiDir = m_RNG.Hemisphere(result.Normal);
				const auto reflectDir = glm::reflect(ray.Direction, result.Normal);

				const auto dir = glm::normalize(glm::mix(reflectDir, hemiDir, roughness));

				Ray reflectedRay = {
					result.Position + dir * s_Epsilon,
					dir
				};


				const auto cosTheta = glm::max(0.0f, glm::dot(reflectedRay.Direction, result.Normal));

				const auto radiance = TraceRecursive(params, reflectedRay, scene, recursion - 1);

				auto color = emission + glm::mix(albedo, glm::vec3(1.0f), metallic) * radiance * cosTheta * 2.0f;

				return color;
			}
			else
			{	
				return scene.Background->Sample(ray.Direction);
			}
		}
	}
}