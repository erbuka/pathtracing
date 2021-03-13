#pragma once

#include "AbstractRaytracer.h"

namespace rt::utility
{
	class DebugRaytracer : public AbstractRaytracer
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