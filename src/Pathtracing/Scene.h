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

	/// <summary>
	/// A axis aligned bounding box
	/// </summary>
	class BoundingBox
	{
	public:

		/// <summary>
		/// Minimum and maximum point
		/// </summary>
		glm::vec3 Min, Max;

		BoundingBox() : Min(), Max() {}
		BoundingBox(const glm::vec3& min, const glm::vec3& max) : Min(min), Max(max) {}

		/// <summary>
		/// Split this bouning box into 2 bounding boxes along the given axis at the given value
		/// </summary>
		/// <param name="axis">The axis</param>
		/// <param name="value">The value to split at</param>
		/// <param name="left">The left part of the split</param>
		/// <param name="right">The right part of the split</param>
		void Split(Axis axis, float value, BoundingBox& left, BoundingBox& right) const;

		/// <summary>
		/// Returns the sturface of this bounding box
		/// </summary>
		float Surface() const;

		/// <summary>
		/// Tests if the given ray intersects this bounding box
		/// </summary>
		/// <param name="ray">The ray</param>
		/// <returns>true of the ray intersects this bounding box</returns>
		bool Intersect(const Ray& ray) const;
	};

	/// <summary>
	/// A triangle
	/// </summary>
	class Triangle
	{
	public:
		/// <summary>
		/// The vertices
		/// </summary>
		std::array<Vertex, 3> Vertices;

		/// <summary>
		/// Get the face normal (ai, cross product between 2 edges)
		/// </summary>
		/// <returns></returns>
		const glm::vec3& GetFaceNormal() const { return m_FaceNormal; }

		/// <summary>
		/// Compute baricentric coordinates
		/// </summary>
		/// <param name="point">The point</param>
		/// <returns>The baricentric coordinates for the given point</returns>
		glm::vec3 Baricentric(const glm::vec3& point) const;

		/// <summary>
		/// Updates internal values that are used for computing intersection faster.
		/// Must be called when vertices are updated
		/// </summary>
		void Update();

	private:
		std::array<glm::vec3, 3> m_Edges;
		glm::vec3 m_FaceNormal;
		float m_D00, m_D01, m_D11;
		float m_InvDen;
	};


	/// <summary>
	/// A Material
	/// </summary>
	struct Material
	{
		Material();
		std::shared_ptr<Sampler2D> Albedo;
		std::shared_ptr<Sampler2D> Emission;
		std::shared_ptr<Sampler2D> Roughness;
		std::shared_ptr<Sampler2D> Metallic;
	};

	/// <summary>
	/// A KD-tree for triangles. Internally used by Mesh to optimize intersection tests.
	/// </summary>
	class KDTreeNode
	{
	public:
		/// <summary>
		/// Constructs and empty node
		/// </summary>
		KDTreeNode() {}

		/// <summary>
		/// Recursively constructs a tree
		/// </summary>
		/// <param name="triangles">The triangles</param>
		/// <param name="bounds">A bounding box that contains all the triangles</param>
		/// <param name="depth">The depth of this node</param>
		KDTreeNode(const std::vector<Triangle>& triangles, const BoundingBox& bounds, uint32_t depth);

		/// <summary>
		/// Returns the maximum depth of the tree
		/// </summary>
		const uint32_t GetMaxDepth() const;

		/// <summary>
		/// Returns the triangles contained in this node
		/// </summary>
		const std::vector<Triangle>& GetTriangles() const { return m_Triangles; }
		
		/// <summary>
		/// Returns the bounds of this node
		/// </summary>
		const BoundingBox& GetBounds() const { return m_Bounds; }

		/// <summary>
		/// Returns the left child, or nullptr if this node is a leaf
		/// </summary>
		const std::unique_ptr<KDTreeNode>& GetLeft() const { return m_Left; }
		
		/// <summary>
		/// Returns the right child, or nullptr if this node is a leaf
		/// </summary>
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

	/// <summary>
	/// A generic shape
	/// </summary>
	class Shape 
	{
	public:
		/// <summary>
		/// Optimize the shape before rendering
		/// </summary>
		virtual void Compile() = 0;

		/// <summary>
		/// Intersection test with this shape
		/// </summary>
		/// <param name="ray">The ray, in local coordinates</param>
		/// <returns>The result of the intersection</returns>
		virtual RaycastResult Intersect(const Ray& ray) const = 0;

		/// <summary>
		/// Get the local bounds of this shape
		/// </summary>
		/// <returns></returns>
		virtual const BoundingBox& GetBounds() const = 0;
	};

	/// <summary>
	/// A sphere
	/// </summary>
	class Sphere : public Shape
	{
	public:
		void Compile() override {}
		RaycastResult Intersect(const Ray& ray) const override;
		const BoundingBox& GetBounds() const override { return m_Bounds; }
	private:
		BoundingBox m_Bounds = BoundingBox({ -1, -1, -1 }, { 1, 1, 1 });
	};

	/// <summary>
	/// A triangle mesh
	/// </summary>
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
		
		/// <summary>
		/// Returns the triangles of this mesh
		/// </summary>
		const std::vector<Triangle>& GetTriangles() const { return m_Triangles; }

		/// <summary>
		/// Adds a new triangle
		/// </summary>
		/// <returns>A reference to the new triangle</returns>
		Triangle& AddTriangle();


		RaycastResult Intersect(const Ray& ray) const override;
		
		void Compile() override;
		
		/// <summary>
		/// Returns the current KD-tree for this mesh
		/// </summary>
		const std::unique_ptr<KDTreeNode>& GetKDTree() const { return m_Tree; }
	};

	/// <summary>
	/// A scene node
	/// </summary>
	class SceneNode
	{

	private:
		glm::mat4 m_Transform = glm::identity<glm::mat4>();
		glm::mat4 m_InvTransform = glm::identity<glm::mat4>();
		glm::mat4 m_NormalTransform = glm::identity<glm::mat4>();
		void UpdateMatrices();

	public:

		/// <summary>
		/// The material
		/// </summary>
		rt::Material Material;

		/// <summary>
		/// The shape
		/// </summary>
		std::shared_ptr<rt::Shape> Shape = nullptr;
		
		/// <summary>
		/// Sets the current transform to an identity matrix
		/// </summary>
		void LoadIdentity();

		/// <summary>
		/// Translate this node
		/// </summary>
		/// <param name="t">Translation</param>
		void Translate(const glm::vec3& t);

		/// <summary>
		/// Rotate this node
		/// </summary>
		/// <param name="axis">The axis</param>
		/// <param name="angle">The angle of rotation (radians)</param>
		void Rotate(const glm::vec3& axis, float angle);

		/// <summary>
		/// Scale this node
		/// </summary>
		/// <param name="s">The scale factor</param>
		void Scale(const glm::vec3& s);

		/// <summary>
		/// Post-multiply the current transform by the given matrix
		/// </summary>
		/// <param name="mat">The matrix to multiply</param>
		void Multiply(const glm::mat4& mat);

		/// <summary>
		/// Returns the current transform
		/// </summary>
		const glm::mat4& GetTransform() const { return m_Transform; }
		
		/// <summary>
		/// Returns the inverse of the current transform
		/// </summary>
		const glm::mat4& GetInverseTransform() const { return m_InvTransform; }
		
		/// <summary>
		/// Returns the transform to apply to normal vectors (inverse-transpose of the current transform)
		/// </summary>
		/// <returns></returns>
		const glm::mat4& GetNormalTransform() const { return m_NormalTransform; }
	};

	/// <summary>
	/// A scene
	/// </summary>
	class Scene 
	{
	public:
		
		Scene();

		/// <summary>
		/// The camera to use when rendering
		/// </summary>
		rt::Camera Camera;

		/// <summary>
		/// The scene's background
		/// </summary>
		std::shared_ptr<Sampler3D> Background = nullptr;

		/// <summary>
		/// The scene's nodes
		/// </summary>
		std::vector<std::shared_ptr<SceneNode>> Nodes;

		/// <summary>
		/// Cast a ray on the scene
		/// </summary>
		/// <param name="ray">The ray</param>
		/// <param name="returnOnFirstHit">If true, the function returns as soon as some node is hit</param>
		/// <param name="avoidNodes">If non-emtpy, this nodes will not be considered for intersection tests</param>
		/// <returns>The closest intersection</returns>
		std::tuple<RaycastResult, std::shared_ptr<SceneNode>> CastRay(const Ray& ray, bool returnOnFirstHit = false, const std::vector<std::shared_ptr<SceneNode>>& avoidNodes = {}) const;
		
		/// <summary>
		/// Get all the nodes that emit light. This takes into account the Emission property of the material
		/// </summary>
		const std::vector<std::shared_ptr<SceneNode>>& GetLightSources() const { return m_LightSources; }

		void Compile();

	private:
		std::vector<std::shared_ptr<SceneNode>> m_LightSources;
	};
}