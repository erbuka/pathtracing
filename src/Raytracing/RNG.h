#pragma once

#include <random>

#include <glm/glm.hpp>

namespace rt
{
	class RNG
	{
	public:
		float Next() { return m_01(m_E); }
		glm::vec3 Hemisphere(const glm::vec3& n);

	private:
		std::mt19937 m_E = std::mt19937(42);
		std::uniform_real_distribution<float> m_01;
	};
}