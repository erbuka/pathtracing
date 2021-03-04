#include "Common.h"

namespace rt
{
	Ray operator*(const glm::mat4& m, const Ray& r)
	{
		auto origin = glm::vec3(m * glm::vec4(r.Origin, 1.0f));
		auto direction = glm::normalize(glm::vec3(m * glm::vec4(r.Direction, 0.0f)));
		return { origin, direction };
	}
}