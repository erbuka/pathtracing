#include "Raytracer.h"

#include "Scene.h"

namespace rt
{
	glm::vec3 Raytracer::Trace(const ViewParameters& params, const Ray& ray, const Scene& scene)
	{
		auto [result, node] = scene.Intersect(ray);

		if (result.Hit)
		{
			return node->Material.Color->Sample(result.UV);
		}
		else
		{
			return scene.Background ? scene.Background->Sample(ray.Direction) : glm::vec3(0.0f);
		}
	}
}