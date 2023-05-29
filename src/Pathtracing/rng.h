#pragma once

#include <random>

#include <glm/glm.hpp>

namespace rt
{
	/// <summary>
	/// Utility class for random rumbers
	/// </summary>
	class rng
	{
	private:
		static thread_local std::mt19937 m_e;
		static thread_local std::uniform_real_distribution<float> m_01;
	public:

		rng() = delete;

		/// <summary>
		/// Gets a number in range [0, 1] inclusive
		/// </summary>
		/// <returns></returns>
		static float next() { return m_01(m_e); }

		template<typename T>
		static T next(const T min, const T max) {
			return static_cast<T>(min + (max - min) * next());
		}

		/// <summary>
		/// Gets a random vector on the hemisphere on the direction of the normal
		/// </summary>
		/// <param name="n">The normal</param>
		/// <returns>A random unit vector</returns>
		static glm::vec3 hemisphere(const glm::vec3& n);

		static void seed(const std::uint32_t s) { m_e.seed(s); }

	};
}