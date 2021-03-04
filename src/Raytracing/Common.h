#pragma once

#include <glm/glm.hpp>

namespace rt {

	enum class Axis : uint32_t { X = 0, Y = 1, Z = 2 };


	struct Ray {
		glm::vec3 Origin;
		glm::vec3 Direction;
	};

	Ray operator*(const glm::mat4& m, const Ray& r);

}