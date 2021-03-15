#include "RNG.h"


#include <glm/ext.hpp>

namespace rt
{
	glm::vec3 RNG::Hemisphere(const glm::vec3& n)
	{
		glm::vec3 t;

		if (glm::abs(n.x) > glm::abs(n.y))
		{
			t = glm::normalize(glm::vec3(n.z, 0.0f, -n.x));
		}
		else
		{
			t = glm::normalize(glm::vec3(0.0f, -n.z, n.y));
		}

		const glm::vec3 b = glm::cross(n, t);

		const float z = m_01(m_E);
		const float r = glm::sqrt(1.0f - z * z);
		const float phi = glm::pi<float>() * 2.0f * m_01(m_E);
		glm::vec3 tangentSample = { r * glm::cos(phi), r * glm::sin(phi), z };

		const auto hemiDir = tangentSample.x * t + tangentSample.y * b + tangentSample.z * n;

		return hemiDir;

	}
}