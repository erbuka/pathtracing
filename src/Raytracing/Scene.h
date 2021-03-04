#pragma once

#include <array>
#include <vector>
#include <memory>
#include <map>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Common.h"


namespace rt {

	class Sampler2D;
	class Sampler3D;

	struct Vertex
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Normal = { 0.0f, 0.0f, 0.0f };
		glm::vec2 UV = { 0.0f, 0.0f };
	};

	struct RaycastResult
	{
		bool Hit = false;
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 UV;
	};


	class BoundingBox
	{
	public:

		glm::vec3 Min, Max;

		BoundingBox() : Min(), Max() {}
		BoundingBox(const glm::vec3& min, const glm::vec3& max) : Min(min), Max(max) {}

		/// Split this bouning box into 2 bounding boxes along the given axis at the given value
		void Split(Axis axis, float value, BoundingBox& left, BoundingBox& right) const;


		/// Returns the sturface of this bounding box
		float Surface() const;


		/// Tests if the given ray intersects this bounding box
		bool Intersect(const Ray& ray) const;
	};

	class Triangle
	{
	public:
		std::array<Vertex, 3> Vertices;

		const glm::vec3& GetFaceNormal() const { return m_FaceNormal; }
		glm::vec3 Baricentric(const glm::vec3& point) const;
		void Update();

	private:
		std::array<glm::vec3, 3> m_Edges;
		glm::vec3 m_FaceNormal;
		float m_D00, m_D01, m_D11;
		float m_InvDen;
	};

	enum class LightType
	{
		Directional, Point, Spot
	};

	struct Light
	{
		LightType Type = LightType::Directional;
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		glm::vec3 Direction = { 0.0f, 1.0f, 0.0f };
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Attenuation = 0.0f;
		float Angle = glm::pi<float>() / 4.0f;
	};

	struct Material
	{
		Material();
		std::shared_ptr<Sampler2D> Color;
	};

	class KDTreeNode
	{
	public:
		KDTreeNode() {}
		KDTreeNode(const std::vector<Triangle>& triangles, const BoundingBox& bounds, uint32_t depth);
			
		const std::vector<Triangle>& GetTriangles() const { return m_Triangles; }
		const BoundingBox& GetBounds() const { return m_Bounds; }
		const std::unique_ptr<KDTreeNode>& GetLeft() const { return m_Left; }
		const std::unique_ptr<KDTreeNode>& GetRight() const { return  m_Right; }

	private:
		uint32_t m_Depth = 0;
		BoundingBox m_Bounds;
		std::vector<Triangle> m_Triangles;
		std::unique_ptr<KDTreeNode> m_Left = nullptr;
		std::unique_ptr<KDTreeNode> m_Right = nullptr;
	};

	class Shape 
	{
	public:
		virtual RaycastResult Intersect(const Ray& ray) const = 0;
	};

	class Sphere : public Shape
	{
	public:
		RaycastResult Intersect(const Ray& ray) const override;
	};

	class Mesh: public Shape
	{
	public:

		
		const std::vector<Triangle>& GetTriangles() const { return m_Triangles; }
		Triangle& AddTriangle();
		RaycastResult Intersect(const Ray& ray) const override;
		void Compile();

	private:

		RaycastResult IntersectTriangle(const Ray& ray, const Triangle& triangle) const;
		void IntersectInternal(const Ray& ray, const std::unique_ptr<KDTreeNode>& node, RaycastResult& result, float& distance) const;

		BoundingBox m_BoundingBox;
		std::vector<Triangle> m_Triangles;
		std::unique_ptr<KDTreeNode> m_Tree;
	};

	class SceneNode
	{
	public:

		glm::mat4 Transform = glm::identity<glm::mat4>();
		Material Material;
		std::shared_ptr<Shape> Shape = nullptr;

		void LoadIdentity();
		void Translate(const glm::vec3& t);
		void Rotate(const glm::vec3& axis, float angle);
		void Scale(const glm::vec3& s);

		
	};


	class Scene 
	{
	public:

		std::vector<Light> Lights;
		std::shared_ptr<Sampler3D> Background = nullptr;
		std::vector<std::shared_ptr<SceneNode>> Nodes;
		std::tuple<RaycastResult, std::shared_ptr<SceneNode>> Scene::Intersect(const Ray& ray) const;
	};
}