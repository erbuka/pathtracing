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
		static constexpr float s_Epsilon = 1e-3f;
		RNG m_RNG;
		glm::vec3 TraceRecursive(const ViewParameters& params, const Ray& ray, const Scene& scene, uint32_t recursion);

	};
}