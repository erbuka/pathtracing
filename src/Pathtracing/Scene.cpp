#include "scene.h"

#include <fstream>
#include <regex>

#include <spdlog/spdlog.h>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>

#include "sampler.h"

namespace rt 
{

	size_t object_id::s_next = 0;

	ray operator*(const glm::mat4& m, const ray& r)
	{
		auto origin = glm::vec3(m * glm::vec4(r.origin, 1.0f));
		auto direction = glm::normalize(glm::vec3(m * glm::vec4(r.direction, 0.0f)));
		return { origin, direction };
	}

	float bounding_box::surface() const
	{
		return 2 * ((max.x - min.x) * (max.y - min.y) + (max.x - min.x) * (max.z - min.z) + (max.z - min.z) * (max.y - min.y));
	}


	void bounding_box::split(axis axis, float value, bounding_box& left, bounding_box& right) const
	{
		switch (axis)
		{
		case axis::X:
			left.min = { min.x, min.y, min.z };
			left.max = { value, max.y, max.z };
			right.min = { value, min.y, min.z };
			right.max = { max.x, max.y, max.z };
			break;
		case axis::Y:
			left.min = { min.x, min.y, min.z };
			left.max = { max.x, value, max.z };
			right.min = { min.x, value, min.z };
			right.max = { max.x, max.y, max.z };
			break;
		case axis::Z:
			left.min = { min.x, min.y, min.z };
			left.max = { max.x, max.y, value };
			right.min = { min.x, min.y, value };
			right.max = { max.x, max.y, max.z };
			break;
		default:
			break;
		}
	}

	bool bounding_box::intersect(const ray& ray) const
	{
		const float t1 = (min.x - ray.origin.x) / ray.direction.x;
		const float t2 = (max.x - ray.origin.x) / ray.direction.x;
		const float t3 = (min.y - ray.origin.y) / ray.direction.y;
		const float t4 = (max.y - ray.origin.y) / ray.direction.y;
		const float t5 = (min.z - ray.origin.z) / ray.direction.z;
		const float t6 = (max.z - ray.origin.z) / ray.direction.z;
		const float t7 = std::fmax(std::fmax(std::fmin(t1, t2), std::fmin(t3, t4)), std::fmin(t5, t6));
		const float t8 = std::fmin(std::fmin(std::fmax(t1, t2), std::fmax(t3, t4)), std::fmax(t5, t6));
		const float t9 = (t8 < 0 || t7 > t8) ? -1 : t7;
		if (t9 != -1) {
			//result.Point = ray.direction * t9 + ray.origin;
			//result.hit = true;
			return true;
		}
		return false;
	}


	glm::vec3 triangle::baricentric(const glm::vec3& point) const
	{
		// Fast baricentric coordinates:
		// https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
		const glm::vec3 v2 = point - vertices[0].position;
		const float d20 = glm::dot(v2, m_edges[0]);
		const float d21 = glm::dot(v2, m_edges[1]);
		const float v = (m_d11 * d20 - m_d01 * d21) * m_inv_den;
		const float w = (m_d00 * d21 - m_d01 * d20) * m_inv_den;
		const float u = 1.0f - v - w;
		return { u, v, w };
	}

	void triangle::update()
	{
		m_face_normal = glm::normalize(glm::cross(vertices[1].position - vertices[0].position,
			vertices[2].position - vertices[1].position));

		m_edges = {
			vertices[1].position - vertices[0].position,
			vertices[2].position - vertices[0].position,
			vertices[2].position - vertices[1].position
		};

		m_d00 = glm::dot(m_edges[0], m_edges[0]);
		m_d01 = glm::dot(m_edges[0], m_edges[1]);
		m_d11 = glm::dot(m_edges[1], m_edges[1]);

		m_inv_den = 1.0f / (m_d00 * m_d11 - m_d01 * m_d01);
	}
	

	triangle& mesh::add_triangle()
	{
		return m_triangles.emplace_back();
	}
	
	raycast_result mesh::intersect(const ray& ray) const
	{
		raycast_result result;
		float distance = std::numeric_limits<float>::max();
		intersect_internal(ray, m_tree, result, distance);
		return result;
	}

	void mesh::compile()
	{
		auto& min = m_bounds.min;
		auto& max = m_bounds.max;

		for (auto& t : m_triangles) 
		{
			t.update();

			for (auto& v : t.vertices)
			{
				min = glm::min(min, v.position);
				max = glm::max(max, v.position);
			}

		}

		m_tree = std::make_unique<kd_tree_node>(m_triangles, m_bounds, 0);

	}

	raycast_result mesh::intersect_triangle(const ray& ray, const triangle& t) const
	{
		raycast_result result;

		auto l = ray.origin - t.vertices[0].position;
		float distance = glm::dot(l, t.get_face_normal());

		if (distance < 0)
		{
			// Ray origin "behind" the triangle plane
			return result;
		}

		float cosine = glm::dot(ray.direction, t.get_face_normal());

		// Check if the ray is never intersecting the triangle plane
		if (cosine >= 0)
		{
			return result;
		}


		// Project the ray on the triangle plane 
		auto projection = ray.origin + ray.direction * (distance / -cosine);

		// Use baricentric coordinates to check if the ray projection
		// is contained in the triangle
		auto bar = t.baricentric(projection);

		if (bar.x >= 0 && bar.y >= 0 && bar.z >= 0)
		{
			result.hit = true;
			result.position = projection;
			result.normal = glm::normalize(
				t.vertices[0].normal * bar.x +
				t.vertices[1].normal * bar.y +
				t.vertices[2].normal * bar.z);
			result.uv =
				bar.x * t.vertices[0].uv +
				bar.y * t.vertices[1].uv +
				bar.z * t.vertices[2].uv;
		}

		return result;
	}

	void mesh::intersect_internal(const ray& ray, const std::unique_ptr<kd_tree_node>& node, raycast_result& result, float& distance) const
	{
		if (node->get_bounds().intersect(ray))
		{
			for (auto& t : node->get_triangles())
			{
				auto r = intersect_triangle(ray, t);

				auto d = glm::length2(ray.origin - r.position);

				if (d < distance && r.hit)
				{
					result = r;
					distance = d;
				}
			}

			if (node->get_left() != nullptr)
				intersect_internal(ray, node->get_left(), result, distance);

			if (node->get_right() != nullptr)
				intersect_internal(ray, node->get_right(), result, distance);

		}
	}

	kd_tree_node::kd_tree_node(const std::vector<triangle>& triangles, const bounding_box& bounds, uint32_t depth) :
		m_bounds(bounds),
		m_depth(depth)
	{

		// Stop condition
		if (triangles.size() <= 1 || m_depth == 100)
		{
			for (auto& t : triangles)
				m_triangles.push_back(t);
			return;
		}



		// Select the split axis (round-robin)
		uint32_t uAxis = depth % 3;


		struct
		{
			bounding_box LeftBounds, RightBounds;
			std::vector<triangle> LeftTris, RightTris;
		} result;

		// Take the median of all points as split point
		float median = 0;

		for (auto& t : triangles)
		{
			median += t.vertices[0].position[uAxis];
			median += t.vertices[1].position[uAxis];
			median += t.vertices[2].position[uAxis];
		}

		median /= 3 * triangles.size();

		m_bounds.split(static_cast<axis>(uAxis), median, result.LeftBounds, result.RightBounds);

		// Test every triangle in both left and right bounding boxes
		for (auto& t : triangles)
		{

			if (t.vertices[0].position[uAxis] <= median || t.vertices[1].position[uAxis] <= median || t.vertices[2].position[uAxis] <= median)
			{
				result.LeftTris.push_back(t);
			}

			if (t.vertices[0].position[uAxis] >= median || t.vertices[1].position[uAxis] >= median || t.vertices[2].position[uAxis] >= median)
			{
				result.RightTris.push_back(t);
			}
		}


		// Check that not too many triangles are in common (> 50%)
		// between the subdivisions 
		if (result.LeftTris.size() + result.RightTris.size() > 1.5 * triangles.size())
		{
			// If so, subdiving is not efficent anymore
			m_triangles = triangles;
		}
		else
		{
			// Subidivide

			if (result.LeftTris.size() > 0)
				m_left = std::make_unique<kd_tree_node>(result.LeftTris, result.LeftBounds, depth + 1);

			if (result.RightTris.size() > 0)
				m_right = std::make_unique<kd_tree_node>(result.RightTris, result.RightBounds, depth + 1);
		}
	}

	const uint32_t kd_tree_node::get_max_depth() const
	{
		const auto d0 = m_left ? m_left->get_max_depth() : m_depth;
		const auto d1 = m_right ? m_left->get_max_depth() : m_depth;
		return std::max(d0, d1);
	}

	void scene_node::update_matrices()
	{
		m_inv_transform = glm::inverse(m_transform);
		m_normal_transform = glm::transpose(m_inv_transform);
	}

	void scene_node::load_identity()
	{
		m_transform = glm::identity<glm::mat4>();
		update_matrices();
	}

	void scene_node::translate(const glm::vec3& t)
	{
		m_transform *= glm::translate(t);
		update_matrices();
	}

	void scene_node::rotate(const glm::vec3& axis, float angle)
	{
		m_transform *= glm::rotate(angle, axis);
		update_matrices();
	}

	void scene_node::scale(const glm::vec3& s)
	{
		m_transform *= glm::scale(s);
		update_matrices();
	}

	void scene_node::multiply(const glm::mat4& mat)
	{
		m_transform *= mat;
		update_matrices();
	}

	void scene::compile()
	{

		std::for_each(nodes.begin(), nodes.end(), [](std::shared_ptr<scene_node>& n) {
			if (n->shape)
				n->shape->compile();
		});
		
		m_light_sources.clear();

		for (auto& n : nodes)
		{
			const auto avg = n->material.emission->average();
			if (avg.r + avg.g + avg.b > 0.0f)
				m_light_sources.push_back(n);
		}

	}

	scene::scene()
	{
		background = std::make_shared<color_sampler>(glm::vec3{ 0.0f, 0.0f, 0.0f });
	}

	std::tuple<raycast_result, std::shared_ptr<scene_node>> scene::cast_ray(const ray& ray, bool return_on_first_hit, const std::vector<std::shared_ptr<scene_node>>& avoid_nodes) const
	{
		float distance = std::numeric_limits<float>::max();
		raycast_result raycastResult;
		std::shared_ptr<scene_node> sceneNode = nullptr;
			
		for (const auto& node : nodes)
		{
			if (!avoid_nodes.empty() && std::find(avoid_nodes.begin(), avoid_nodes.end(), node) != avoid_nodes.end())
			{
				continue;
			}

			// Ray is transformed by the inverse tranform of the node
			// The intersection test is performed in local coordinates, because
			// transforming the ray is faster than transforming all the vertices
			auto r0 = node->shape->intersect(node->get_inverse_transform() * ray);
			if (r0.hit)
			{
				// Transform to world coordinates
				r0.position = node->get_transform() * glm::vec4(r0.position, 1.0f);

				// The vec3 cast is needed otherwise it would normalize as a vec4
				r0.normal = glm::normalize(glm::vec3(node->get_normal_transform() * glm::vec4(r0.normal, 0.0f)));
				
				if (return_on_first_hit)
					return { r0, node };
				
				float d0 = glm::length2(r0.position - ray.origin);


				if (d0 < distance) 
				{
					distance = d0;
					raycastResult = r0;
					sceneNode = node;
				}
			}


		}

		return { raycastResult, std::move(sceneNode) };

	}

	raycast_result sphere::intersect(const ray& ray) const
	{
		raycast_result result;

		float projection = glm::dot((glm::vec3(0.0f, 0.0f, 0.0f) - ray.origin), ray.direction);
		float squaredDistance = glm::dot(ray.origin, ray.origin) - projection * projection;

		if (squaredDistance > 1.0f)
		{
			// No hit
			return result;
		}


		float offset = std::sqrt(1.0f - squaredDistance);

		float t1 = projection - offset;
		float t2 = projection + offset;

		if (t1 < 0 && t2 < 0)
		{
			// No intersection. The ray is going in the opposite direction
			return result;
		}


		// It could be 1 or 2 intersections
		// t1 is the closest, but might be negative if the ray origin is
		// inside the sphere

		result.hit = true;
		result.position = ray.origin + ray.direction * (t1 >= 0.0f ? t1 : t2);
		result.normal = glm::normalize(result.position);
		result.uv = {
			std::atan2(result.normal.x, result.normal.z) / glm::pi<float>() + 0.5f,
			result.normal.y * 0.5f + 0.5f
		};


		return result;
	}
	
	material::material()
	{
		albedo = std::make_shared<color_sampler>(glm::vec3(1.0f, 1.0f, 1.0f));
		emission = std::make_shared<color_sampler>(glm::vec3(0.0f));
		roughness = std::make_shared<color_sampler>(glm::vec3(1.0f));
		metallic = std::make_shared<color_sampler>(glm::vec3(0.0f));
	}

}