#include "Raytracer.h"

#include <spdlog\spdlog.h>

#include <glm/gtx/norm.hpp>

#include "Scene.h"

namespace rt
{

	glm::vec3 Raytracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		return TraceRecursive(params, ray, scene, 5);
	}


	std::optional<ScanLine> Raytracer::GetNextScanline()
	{
		auto result = m_NextScanline;

		m_NextScanline.Number++;

		if (m_NextScanline.Number == m_ScanlineCount)
		{
			m_NextScanline.Number = 0;
			m_NextScanline.Iteration++;
		}

		return result;
	}

	void Raytracer::OnBeforeRun(const ViewParameters& params, const Scene& scene)
	{
		m_NextScanline = { 0, 0 };
		m_ScanlineCount = params.Width;
	}

	glm::vec3 Raytracer::TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion) 
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
				const auto emission = node->Material.Emission->Sample(result.UV);
				const float roughness = node->Material.Roughness->Sample(result.UV).r;

				// Reflected ray on hemisphere

				const glm::vec3 N = result.Normal;
				glm::vec3 T = glm::cross(N, glm::vec3(0.0f, 0.0f, 1.0f));
				if (glm::length2(T) == 0.0f)
					T = glm::cross(N, glm::vec3(1.0f, 0.0f, 0.0f));
				glm::vec3 B = glm::cross(N, T);

				const float alpha = m_Rand.Next(0.0f, glm::pi<float>() * 2.0f);
				const float beta = m_Rand.Next(0.0f, glm::pi<float>() / 2.0f);
		
				glm::vec3 tangentSample = {
					glm::sin(alpha) * glm::sin(beta),
					glm::cos(alpha) * glm::sin(beta),
					glm::cos(beta)
				};

				auto reflectDir = glm::reflect(ray.Direction, result.Normal);
				auto hemiDir = tangentSample.x * T + tangentSample.y * B + tangentSample.z * N;
				auto dir = glm::normalize(glm::mix(reflectDir, hemiDir, roughness));
				Ray reflectedRay = {
					result.Position + dir * 0.01f,
					dir
				};

				
				auto cosTheta = glm::max(0.0f, glm::dot(reflectedRay.Direction, result.Normal));

				auto radiance = TraceRecursive(params, reflectedRay, scene, recursion - 1);
				
				auto color = emission + albedo * radiance * cosTheta;

				return color;

			}
			else
			{
				return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
			}
		}
	}
}