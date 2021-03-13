#include "Pathtracer.h"

#include <spdlog/spdlog.h>

#include <glm/gtx/norm.hpp>

#include "Scene.h"

namespace rt
{
	Pathtracer::Pathtracer()
	{
		m_01 = std::uniform_real_distribution<float>();
	}


	glm::vec3 Pathtracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		return std::get<0>(TraceRecursive(params, ray, scene, 5));
	}

	std::tuple<glm::vec3, glm::vec3> Pathtracer::GenerateCoordinateSystem(const glm::vec3& n)
	{
		glm::vec3 t;

		if (glm::abs(n.x) > glm::abs(n.y))
		{
			t = glm::normalize(glm::vec3(n.z, 0.0f, -n.x));
		}
		else
		{
			t = glm::normalize(glm::vec3(0.0f, -n.z, n.y));
		}

		return { t, glm::cross(n, t) };

	}

	std::tuple<glm::vec3, float> Pathtracer::TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion)
	{
		if (recursion == 0)
		{
			return { scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f), 0.0f };
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

				const glm::vec3 N = result.Normal;
				const auto [T, B] = GenerateCoordinateSystem(N);

				// TODO double check this thing but I think it's good now
				//const float z = 1.0f - m_01(m_E) * 2.0f;
				const float z = m_01(m_E);
				const float r = glm::sqrt(1.0f - z * z);
				const float phi = glm::pi<float>() * 2.0f * m_01(m_E);
				glm::vec3 tangentSample = { r * glm::cos(phi), r * glm::sin(phi), z };

				const auto reflectDir = glm::reflect(ray.Direction, result.Normal);
				const auto hemiDir = tangentSample.x * T + tangentSample.y * B + tangentSample.z * N;

				const auto dir = glm::normalize(glm::mix(reflectDir, hemiDir, roughness));

				Ray reflectedRay = {
					result.Position + dir * 0.01f,
					dir
				};


				const auto cosTheta = glm::max(0.0f, glm::dot(reflectedRay.Direction, result.Normal));

				const auto [radiance, distance] = TraceRecursive(params, reflectedRay, scene, recursion - 1);

				auto color = emission + glm::mix(albedo, glm::vec3(1.0f), metallic) * radiance * cosTheta * 2.0f;

				return { color, glm::distance(ray.Origin, result.Position) };
			}
			else
			{	
				return { scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f), 0.0f };
			}
		}
	}
}