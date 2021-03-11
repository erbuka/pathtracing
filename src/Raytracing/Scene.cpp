#include "Scene.h"

#include <fstream>
#include <regex>

#include <spdlog\/spdlog.h>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>

#include "Sampler.h"

namespace rt 
{

	size_t ObjectID::s_Next = 0;

	Ray operator*(const glm::mat4& m, const Ray& r)
	{
		auto origin = glm::vec3(m * glm::vec4(r.Origin, 1.0f));
		auto direction = glm::normalize(glm::vec3(m * glm::vec4(r.Direction, 0.0f)));
		return { origin, direction };
	}

	float BoundingBox::Surface() const
	{
		return 2 * ((Max.x - Min.x) * (Max.y - Min.y) + (Max.x - Min.x) * (Max.z - Min.z) + (Max.z - Min.z) * (Max.y - Min.y));
	}


	void BoundingBox::Split(Axis axis, float value, BoundingBox& left, BoundingBox& right) const
	{
		switch (axis)
		{
		case Axis::X:
			left.Min = { Min.x, Min.y, Min.z };
			left.Max = { value, Max.y, Max.z };
			right.Min = { value, Min.y, Min.z };
			right.Max = { Max.x, Max.y, Max.z };
			break;
		case Axis::Y:
			left.Min = { Min.x, Min.y, Min.z };
			left.Max = { Max.x, value, Max.z };
			right.Min = { Min.x, value, Min.z };
			right.Max = { Max.x, Max.y, Max.z };
			break;
		case Axis::Z:
			left.Min = { Min.x, Min.y, Min.z };
			left.Max = { Max.x, Max.y, value };
			right.Min = { Min.x, Min.y, value };
			right.Max = { Max.x, Max.y, Max.z };
			break;
		default:
			break;
		}
	}

	bool BoundingBox::Intersect(const Ray& ray) const
	{
		const float t1 = (Min.x - ray.Origin.x) / ray.Direction.x;
		const float t2 = (Max.x - ray.Origin.x) / ray.Direction.x;
		const float t3 = (Min.y - ray.Origin.y) / ray.Direction.y;
		const float t4 = (Max.y - ray.Origin.y) / ray.Direction.y;
		const float t5 = (Min.z - ray.Origin.z) / ray.Direction.z;
		const float t6 = (Max.z - ray.Origin.z) / ray.Direction.z;
		const float t7 = std::fmax(std::fmax(std::fmin(t1, t2), std::fmin(t3, t4)), std::fmin(t5, t6));
		const float t8 = std::fmin(std::fmin(std::fmax(t1, t2), std::fmax(t3, t4)), std::fmax(t5, t6));
		const float t9 = (t8 < 0 || t7 > t8) ? -1 : t7;
		if (t9 != -1) {
			//result.Point = ray.Direction * t9 + ray.Origin;
			//result.Hit = true;
			return true;
		}
		return false;
	}


	glm::vec3 Triangle::Baricentric(const glm::vec3& point) const
	{
		// Fast baricentric coordinates:
		// https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
		const glm::vec3 v2 = point - Vertices[0].Position;
		const float d20 = glm::dot(v2, m_Edges[0]);
		const float d21 = glm::dot(v2, m_Edges[1]);
		const float v = (m_D11 * d20 - m_D01 * d21) * m_InvDen;
		const float w = (m_D00 * d21 - m_D01 * d20) * m_InvDen;
		const float u = 1.0f - v - w;
		return { u, v, w };
	}

	void Triangle::Update()
	{
		m_FaceNormal = glm::normalize(glm::cross(Vertices[1].Position - Vertices[0].Position,
			Vertices[2].Position - Vertices[1].Position));

		m_Edges = {
			Vertices[1].Position - Vertices[0].Position,
			Vertices[2].Position - Vertices[0].Position,
			Vertices[2].Position - Vertices[1].Position
		};

		m_D00 = glm::dot(m_Edges[0], m_Edges[0]);
		m_D01 = glm::dot(m_Edges[0], m_Edges[1]);
		m_D11 = glm::dot(m_Edges[1], m_Edges[1]);

		m_InvDen = 1.0f / (m_D00 * m_D11 - m_D01 * m_D01);
	}
	

	Triangle& Mesh::AddTriangle()
	{
		m_Triangles.push_back({});
		return m_Triangles.back();
	}
	
	RaycastResult Mesh::Intersect(const Ray& ray) const
	{
		RaycastResult result;
		float distance = std::numeric_limits<float>::max();
		IntersectInternal(ray, m_Tree, result, distance);
		return result;
	}

	void Mesh::Compile()
	{
		auto& min = m_BoundingBox.Min;
		auto& max = m_BoundingBox.Max;

		for (auto& t : m_Triangles) 
		{
			t.Update();

			for (auto& v : t.Vertices)
			{
				min = glm::min(min, v.Position);
				max = glm::max(max, v.Position);
			}

		}

		m_Tree = std::make_unique<KDTreeNode>(m_Triangles, m_BoundingBox, 0);

	}

	RaycastResult Mesh::IntersectTriangle(const Ray& ray, const Triangle& t) const
	{
		RaycastResult result;

		auto l = ray.Origin - t.Vertices[0].Position;
		float distance = glm::dot(l, t.GetFaceNormal());

		if (distance < 0)
		{
			// Ray origin "behind" the triangle plane
			return result;
		}

		float cosine = glm::dot(ray.Direction, t.GetFaceNormal());

		// Check if the ray is never intersecting the triangle plane
		if (cosine >= 0)
		{
			return result;
		}


		// Project the ray on the triangle plane 
		auto projection = ray.Origin + ray.Direction * (distance / -cosine);

		// Use baricentric coordinates to check if the ray projection
		// is contained in the triangle
		auto bar = t.Baricentric(projection);

		if (bar.x >= 0 && bar.y >= 0 && bar.z >= 0)
		{
			result.Hit = true;
			result.Position = projection;
			result.Normal = glm::normalize(
				t.Vertices[0].Normal * bar.x +
				t.Vertices[1].Normal * bar.y +
				t.Vertices[2].Normal * bar.z);
			result.UV =
				bar.x * t.Vertices[0].UV +
				bar.y * t.Vertices[1].UV +
				bar.z * t.Vertices[2].UV;
		}

		return result;
	}

	void Mesh::IntersectInternal(const Ray& ray, const std::unique_ptr<KDTreeNode>& node, RaycastResult& result, float& distance) const
	{
		if (node->GetBounds().Intersect(ray))
		{
			for (auto& t : node->GetTriangles())
			{
				auto r = IntersectTriangle(ray, t);

				auto d = glm::length2(ray.Origin - r.Position);

				if (d < distance && r.Hit)
				{
					result = r;
					distance = d;
				}
			}

			if (node->GetLeft() != nullptr)
				IntersectInternal(ray, node->GetLeft(), result, distance);

			if (node->GetRight() != nullptr)
				IntersectInternal(ray, node->GetRight(), result, distance);

		}
	}

	KDTreeNode::KDTreeNode(const std::vector<Triangle>& triangles, const BoundingBox& bounds, uint32_t depth) :
		m_Bounds(bounds),
		m_Depth(depth)
	{

		// Stop condition
		if (triangles.size() <= 1 || m_Depth == 100)
		{
			for (auto& t : triangles)
				m_Triangles.push_back(t);
			return;
		}



		// Select the split axis (round-robin)
		uint32_t uAxis = depth % 3;


		struct
		{
			BoundingBox LeftBounds, RightBounds;
			std::vector<Triangle> LeftTris, RightTris;
		} result;

		// Take the median of all points as split point
		float median = 0;

		for (auto& t : triangles)
		{
			median += t.Vertices[0].Position[uAxis];
			median += t.Vertices[1].Position[uAxis];
			median += t.Vertices[2].Position[uAxis];
		}

		median /= 3 * triangles.size();

		m_Bounds.Split(static_cast<Axis>(uAxis), median, result.LeftBounds, result.RightBounds);

		// Test every triangle in both left and right bounding boxes
		for (auto& t : triangles)
		{

			if (t.Vertices[0].Position[uAxis] <= median || t.Vertices[1].Position[uAxis] <= median || t.Vertices[2].Position[uAxis] <= median)
			{
				result.LeftTris.push_back(t);
			}

			if (t.Vertices[0].Position[uAxis] >= median || t.Vertices[1].Position[uAxis] >= median || t.Vertices[2].Position[uAxis] >= median)
			{
				result.RightTris.push_back(t);
			}
		}


		// Check that not too many triangles are in common (> 50%)
		// between the subdivisions 
		if (result.LeftTris.size() + result.RightTris.size() > 1.5 * triangles.size())
		{
			// If so, subdiving is not efficent anymore
			m_Triangles = triangles;
		}
		else
		{
			// Subidivide

			if (result.LeftTris.size() > 0)
				m_Left = std::make_unique<KDTreeNode>(result.LeftTris, result.LeftBounds, depth + 1);

			if (result.RightTris.size() > 0)
				m_Right = std::make_unique<KDTreeNode>(result.RightTris, result.RightBounds, depth + 1);
		}
	}

	void SceneNode::LoadIdentity()
	{
		Transform = glm::identity<glm::mat4>();
	}

	void SceneNode::Translate(const glm::vec3& t)
	{
		Transform *= glm::translate(t);
	}

	void SceneNode::Rotate(const glm::vec3& axis, float angle)
	{
		Transform *= glm::rotate(angle, axis);
	}

	void SceneNode::Scale(const glm::vec3& s)
	{
		Transform *= glm::scale(s);
	}

	void Scene::Compile()
	{
		std::for_each(Nodes.begin(), Nodes.end(), [](std::shared_ptr<SceneNode>& n) {
			if (n->Shape)
				n->Shape->Compile();
		});
	}

	std::tuple<RaycastResult, std::shared_ptr<SceneNode>> Scene::CastRay(const Ray& ray, bool returnOnFirstHit, const std::vector<std::shared_ptr<SceneNode>>& avoidNodes) const
	{
		float distance = std::numeric_limits<float>::max();
		RaycastResult raycastResult;
		std::shared_ptr<SceneNode> sceneNode = nullptr;
			
		for (const auto& node : Nodes)
		{
			if (!avoidNodes.empty() && std::find(avoidNodes.begin(), avoidNodes.end(), node) != avoidNodes.end())
			{
				continue;
			}

			// Ray is transformed by the inverse tranform of the node
			// The intersection test is performed in local coordinates, because
			// transforming the ray is faster than transforming all the vertices
			auto r0 = node->Shape->Intersect(glm::inverse(node->Transform) * ray);
			if (r0.Hit)
			{

				// Transform to world coordinates
				r0.Position = node->Transform * glm::vec4(r0.Position, 1.0f);
				r0.Normal = glm::normalize(node->Transform * glm::vec4(r0.Normal, 0.0f));
				
				if (returnOnFirstHit)
					return { r0, node };
				
				float d0 = glm::length2(r0.Position - ray.Origin);


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

	RaycastResult Sphere::Intersect(const Ray& ray) const
	{
		RaycastResult result;

		float projection = glm::dot((glm::vec3(0.0f, 0.0f, 0.0f) - ray.Origin), ray.Direction);
		float squaredDistance = glm::dot(ray.Origin, ray.Origin) - projection * projection;

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

		result.Hit = true;
		result.Position = ray.Origin + ray.Direction * (t1 >= 0.0f ? t1 : t2);
		result.Normal = glm::normalize(result.Position);
		result.UV = {
			std::atan2(result.Normal.x, result.Normal.z) / glm::pi<float>() + 0.5f,
			result.Normal.y * 0.5f + 0.5f
		};


		return result;
	}
	
	Material::Material()
	{
		Albedo = std::make_shared<ColorSampler>(glm::vec3(1.0f, 1.0f, 1.0f));
		Emission = std::make_shared<ColorSampler>(glm::vec3(0.0f));
		Roughness = std::make_shared<ColorSampler>(glm::vec3(1.0f));
		Metallic = std::make_shared<ColorSampler>(glm::vec3(0.0f));
	}

}