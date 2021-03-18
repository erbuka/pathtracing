#pragma once

#include <random>

#include <glm/glm.hpp>

namespace rt
{
	/// <summary>
	/// Utility class for random rumbers
	/// </summary>
	class RNG
	{
	public:
		/// <summary>
		/// Gets a number in range [0, 1] inclusive
		/// </summary>
		/// <returns></returns>
		float Next() { return m_01(m_E); }

		/// <summary>
		/// Gets a random vector on the hemisphere on the direction of the normal
		/// </summary>
		/// <param name="n">The normal</param>
		/// <returns>A random unit vector</returns>
		glm::vec3 Hemisphere(const glm::vec3& n);

	private:
		std::mt19937 m_E = std::mt19937(42);
		std::uniform_real_distribution<float> m_01;
	};
}