#pragma once

#include <random>

#include "RNG.h"
#include "AbstractRaytracer.h"


namespace rt
{
	class Pathtracer : public AbstractRaytracer
	{
	public:
		glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) override;
	
	private:

		RNG m_RNG;

		std::tuple<glm::vec3, float> TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion);

	};
}