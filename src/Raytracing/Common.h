#pragma once

#include <random>

#include <glm/glm.hpp>

namespace rt {

	enum class Axis : uint32_t { X = 0, Y = 1, Z = 2 };


	struct Ray {
		glm::vec3 Origin;
		glm::vec3 Direction;
	};

	Ray operator*(const glm::mat4& m, const Ray& r);

    class Random
    {
    public:
        Random() { m_R01 = std::uniform_real_distribution<double>(0.0, 1.0); }

        float Next() { return m_R01(m_E); }
        float Next(float min, float max) { return min + m_R01(m_E) * (max - min); }

    private:
        std::uniform_real_distribution<double> m_R01;
        std::mt19937 m_E;
    };

}