#include "Raytracer.h"

#include <spdlog\spdlog.h>

#include <glm/gtx/norm.hpp>

#include "Scene.h"

namespace rt
{
	glm::vec3 Raytracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		return TraceRecursive(params, ray, scene, 3);
	}
	glm::vec3 Raytracer::TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion) const
	{
		if (recursion == 0)
		{
			return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
		}
		else
		{
			auto [result, node] = scene.CastRay(ray);

			if (result.Hit)
			{
				const auto albedo = node->Material.Albedo->Sample(result.UV);
				const float kS = node->Material.Specular->Sample(result.UV).r;
				const float kD = 1.0f - kS;
				const auto R = glm::reflect(ray.Direction, result.Normal);

				glm::vec3 directLighting(0.0f);


				for (const auto& light : scene.Lights)
				{
					if (light.Type == LightType::Directional)
					{
						auto [shadowResult, _] = scene.CastRay({ result.Position, light.Direction }, true, { node });
						if (!shadowResult.Hit)
						{
							directLighting += light.Color * glm::max(0.0f, glm::dot(light.Direction, result.Normal));
						}
					}
					else if (light.Type == LightType::Point)
					{
						auto lightVector = light.Position - result.Position;
						auto lvn = glm::normalize(lightVector);
						auto [shadowResult, _] = scene.CastRay({ result.Position, lvn }, true, { node });
						if (!shadowResult.Hit)
						{
							float attenuation = 1.0f / (1.0f + glm::length2(lightVector) * light.Attenuation);
							directLighting += light.Color * glm::max(0.0f, glm::dot(lvn, result.Normal)) * attenuation;
						}
					}
					else if (light.Type == LightType::Spot)
					{
						auto lightVector = light.Position - result.Position;
						auto lvn = glm::normalize(lightVector);
						if (glm::max(0.0f, glm::dot(-lvn, light.Direction)) >= glm::cos(light.Angle / 2.0f))
						{
							auto [shadowResult, _] = scene.CastRay({ result.Position, lvn }, true, { node });
							if (!shadowResult.Hit)
							{
								float attenuation = 1.0f / (1.0f + glm::length2(lightVector) * light.Attenuation);
								directLighting += light.Color * glm::max(0.0f, glm::dot(lvn, result.Normal)) * attenuation;
							}
						}
					}
				}


				const Ray reflectedRay = { result.Position + R * 0.001f, R };

				auto specular = TraceRecursive(params, reflectedRay, scene, recursion - 1);

				auto color = albedo * (kD * directLighting + kS * specular);

				return color;

			}
			else
			{
				return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
			}
		}
	}
}