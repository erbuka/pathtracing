#pragma once

#include <AbstractPathtracer.h>

namespace rt::utility
{
	class DebugPathtracer : public AbstractPathtracer
	{
	public:

		enum class Mode
		{
			Albedo,
			Emission,
			Roughness,
			Metallic,
			Normal,
		};

		Mode CurrentMode = Mode::Albedo;

		glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) override;

		
	};
}