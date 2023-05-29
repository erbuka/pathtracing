#include "rng.h"


#include <glm/ext.hpp>

namespace rt
{

	thread_local std::mt19937 rng::m_e;
	thread_local std::uniform_real_distribution<float> rng::m_01;

	glm::vec3 rng::hemisphere(const glm::vec3& n)
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

		const float z = m_01(m_e);
		const float r = glm::sqrt(1.0f - z * z);
		const float phi = glm::pi<float>() * 2.0f * m_01(m_e);
		glm::vec3 tangent_sample = { r * glm::cos(phi), r * glm::sin(phi), z };

		const auto hemi_dir = tangent_sample.x * t + tangent_sample.y * b + tangent_sample.z * n;

		return hemi_dir;

	}
}