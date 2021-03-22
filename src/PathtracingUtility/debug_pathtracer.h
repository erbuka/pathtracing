#pragma once

#include <abstract_pathtracer.h>

namespace rt::utility
{
	/// <summary>
	/// A simple tracer that outputs useful debug information, like normals,
	/// albedo, emission, etc
	/// </summary>
	class debug_pathtracer : public abstract_pathtracer
	{
	public:

		enum class mode
		{
			albedo,
			emission,
			roughness,
			metallic,
			normal,
		};

		mode current_mode = mode::albedo;

		glm::vec3 trace(const view_parameters& params, const ray& ray, const scene& scene) override;

		
	};
}