#pragma once

#include "AbstractRaytracer.h"

namespace rt
{
	class Raytracer : public AbstractRaytracer
	{
	public:
		glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) override;
	};
}