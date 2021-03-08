#pragma once

#include <random>

#include "AbstractRaytracer.h"



namespace rt
{
	class Raytracer : public AbstractRaytracer
	{
	public:

		Raytracer();

		glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) override;
		std::optional<ScanLine> GetNextScanline() override;
		void OnBeforeRun(const ViewParameters& params, const Scene& scene) override;
	private:

		std::mt19937 m_E;
		std::uniform_real_distribution<float> m_Beta;
		std::uniform_real_distribution<float> m_Alpha;

		Random m_Rand;
		uint32_t m_ScanlineCount;
		ScanLine m_NextScanline;
		std::tuple<glm::vec3, float> TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion);

	};
}