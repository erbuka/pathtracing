#pragma once

#include <array>
#include <vector>
#include <memory>
#include <map>

#include <glm/glm.hpp>
#include <glm/ext.hpp>


namespace rt {

	class Sampler2D;
	class Sampler3D;

	enum class Axis : uint32_t { X = 0, Y = 1, Z = 2 };

	struct Ray 
	{
		glm::vec3 Origin;
		glm::vec3 Direction;
	};

	Ray operator*(const glm::mat4& m, const Ray& r);

	struct Camera
	{
	public:
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };

		const glm::vec3& GetDirection() const { return m_Direction; }
		void SetDirection(const glm::vec3& dir) { m_Direction = glm::normalize(dir); }

	private:
		glm::vec3 m_Direction = { 0.0f, 0.0f, -1.0f };
	};


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


	struct Material
	{
		Material();
		std::shared_ptr<Sampler2D> Albedo;
		std::shared_ptr<Sampler2D> Emission;
		std::shared_ptr<Sampler2D> Roughness;
		std::shared_ptr<Sampler2D> Metallic;
	};

	class KDTreeNode
	{
	public:
		KDTreeNode() {}
		KDTreeNode(const std::vector<Triangle>& triangles, const BoundingBox& bounds, uint32_t depth);

		const uint32_t GetMaxDepth() const;
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

	class ObjectID
	{
	public:
		const size_t ID;
		ObjectID(): ID(s_Next++) {}
	private:
		static size_t s_Next;
	};

	class Shape 
	{
	public:
		virtual void Compile() = 0;
		virtual RaycastResult Intersect(const Ray& ray) const = 0;
		virtual const BoundingBox& GetBounds() const = 0;
	};

	class Sphere : public Shape
	{
	public:
		void Compile() override {}
		RaycastResult Intersect(const Ray& ray) const override;
		const BoundingBox& GetBounds() const override { return m_Bounds; }
	private:
		BoundingBox m_Bounds = BoundingBox({ -1, -1, -1 }, { 1, 1, 1 });
	};

	class Mesh: public Shape, public ObjectID
	{
	private:

		RaycastResult IntersectTriangle(const Ray& ray, const Triangle& triangle) const;
		void IntersectInternal(const Ray& ray, const std::unique_ptr<KDTreeNode>& node, RaycastResult& result, float& distance) const;

		BoundingBox m_Bounds;
		std::vector<Triangle> m_Triangles;
		std::unique_ptr<KDTreeNode> m_Tree;
	
	public:
		const BoundingBox& GetBounds() const override { return m_Bounds; }
		const std::vector<Triangle>& GetTriangles() const { return m_Triangles; }
		Triangle& AddTriangle();
		RaycastResult Intersect(const Ray& ray) const override;
		void Compile() override;
		const std::unique_ptr<KDTreeNode>& GetKDTree() const { return m_Tree; }
	};

	class SceneNode
	{

	private:
		glm::mat4 m_Transform = glm::identity<glm::mat4>();
		glm::mat4 m_InvTransform = glm::identity<glm::mat4>();
		glm::mat4 m_NormalTransform = glm::identity<glm::mat4>();
		void UpdateMatrices();

	public:

		rt::Material Material;
		std::shared_ptr<rt::Shape> Shape = nullptr;

		void LoadIdentity();
		void Translate(const glm::vec3& t);
		void Rotate(const glm::vec3& axis, float angle);
		void Scale(const glm::vec3& s);
		void Multiply(const glm::mat4& mat);

		const glm::mat4& GetTransform() const { return m_Transform; }
		const glm::mat4& GetInverseTransform() const { return m_InvTransform; }
		const glm::mat4& GetNormalTransform() const { return m_NormalTransform; }
	};
	
	class Scene 
	{
	public:
		
		Scene();

		rt::Camera Camera;
		std::shared_ptr<Sampler3D> Background = nullptr;
		std::vector<std::shared_ptr<SceneNode>> Nodes;
		std::tuple<RaycastResult, std::shared_ptr<SceneNode>> CastRay(const Ray& ray, bool returnOnFirstHit = false, const std::vector<std::shared_ptr<SceneNode>>& avoidNodes = {}) const;
		
		const std::vector<std::shared_ptr<SceneNode>>& GetLightSources() const { return m_LightSources; }

		void Compile();

	private:
		std::vector<std::shared_ptr<SceneNode>> m_LightSources;
	};
}