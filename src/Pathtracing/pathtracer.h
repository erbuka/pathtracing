#pragma once

#include <random>

#include "rng.h"
#include "abstract_pathtracer.h"

namespace rt
{
	/// <summary>
	/// Full integrator pathtracer implementation
	/// </summary>
	class pathtracer : public abstract_pathtracer
	{
	public:
		glm::vec3 trace(const view_parameters& params, const ray& r, const scene& scene) override;
	
	private:
		static constexpr float s_epsilon = 1e-3f;
		rng m_rng;
		glm::vec3 trace_recursive(const view_parameters& params, const ray& r, const scene& scene, uint32_t recursion);

	};
}