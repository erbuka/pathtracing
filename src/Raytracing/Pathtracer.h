#pragma once

#include <random>

#include "AbstractRaytracer.h"


namespace rt
{
	class Pathtracer : public AbstractRaytracer
	{
	public:

		Pathtracer();
		glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) override;
	
	private:


		std::mt19937 m_E = std::mt19937(42);
		std::uniform_real_distribution<float> m_01;

		std::tuple<glm::vec3, glm::vec3> GenerateCoordinateSystem(const glm::vec3& n);
		std::tuple<glm::vec3, float> TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion);

	};
}