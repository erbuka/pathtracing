#pragma once

#include "AbstractRaytracer.h"

namespace rt
{
	class DebugRaytracer : public AbstractRaytracer
	{
	public:

		enum class Mode
		{
			Color,
			Normal
		};

		Mode CurrentMode = Mode::Color;

		glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) override;

	};
}