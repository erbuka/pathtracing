#include "Raytracer.h"

#include <spdlog\spdlog.h>

#include <glm/gtx/norm.hpp>

#include "Scene.h"

namespace rt
{
	glm::vec3 Raytracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		auto [result, node] = scene.CastRay(ray);

		if (result.Hit)
		{
			auto albedo = node->Material.Color->Sample(result.UV);
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


			auto color = albedo * directLighting;

			return color;

		}
		else
		{
			return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
		}
	}
}