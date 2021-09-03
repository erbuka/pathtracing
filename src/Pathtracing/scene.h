#pragma once

#include <array>
#include <vector>
#include <memory>
#include <map>

#include <glm/glm.hpp>
#include <glm/ext.hpp>


namespace rt {

	class sampler_2d;
	class sampler_3d;

	enum class axis : uint32_t { X = 0, Y = 1, Z = 2 };

	struct ray 
	{
		glm::vec3 origin;
		glm::vec3 direction;
	};

	ray operator*(const glm::mat4& m, const ray& r);

	struct camera
	{
	public:
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };

		const glm::vec3& get_direction() const { return m_direction; }
		void set_direction(const glm::vec3& dir) { m_direction = glm::normalize(dir); }

	private:
		glm::vec3 m_direction = { 0.0f, 0.0f, -1.0f };
	};


	struct vertex
	{
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 normal = { 0.0f, 0.0f, 0.0f };
		glm::vec2 uv = { 0.0f, 0.0f };
	};

	struct raycast_result
	{
		bool hit = false;
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	/// <summary>
	/// A axis aligned bounding box
	/// </summary>
	class bounding_box
	{
	public:

		/// <summary>
		/// Minimum and maximum point
		/// </summary>
		glm::vec3 min, max;

		bounding_box() : min(), max() {}
		bounding_box(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

		/// <summary>
		/// Split this bouning box into 2 bounding boxes along the given axis at the given value
		/// </summary>
		/// <param name="axis">The axis</param>
		/// <param name="value">The value to split at</param>
		/// <param name="left">The left part of the split</param>
		/// <param name="right">The right part of the split</param>
		void split(axis axis, float value, bounding_box& left, bounding_box& right) const;

		/// <summary>
		/// Returns the strface of this bounding box
		/// </summary>
		float surface() const;

		/// <summary>
		/// Tests if the given ray intersects this bounding box
		/// </summary>
		/// <param name="ray">The ray</param>
		/// <returns>true of the ray intersects this bounding box</returns>
		bool intersect(const ray& ray) const;
	};

	/// <summary>
	/// A triangle
	/// </summary>
	class triangle
	{
	public:
		/// <summary>
		/// The vertices
		/// </summary>
		std::array<vertex, 3> vertices;

		/// <summary>
		/// Get the face normal (ai, cross product between 2 edges)
		/// </summary>
		/// <returns></returns>
		const glm::vec3& get_face_normal() const { return m_face_normal; }

		/// <summary>
		/// Compute baricentric coordinates
		/// </summary>
		/// <param name="point">The point</param>
		/// <returns>The baricentric coordinates for the given point</returns>
		glm::vec3 baricentric(const glm::vec3& point) const;

		/// <summary>
		/// Updates internal values that are used for computing intersection faster.
		/// Must be called when vertices are updated
		/// </summary>
		void update();

	private:
		std::array<glm::vec3, 3> m_edges;
		glm::vec3 m_face_normal;
		float m_d00, m_d01, m_d11;
		float m_inv_den;
	};


	/// <summary>
	/// A Material
	/// </summary>
	struct material
	{
		material();
		std::shared_ptr<sampler_2d> albedo;
		std::shared_ptr<sampler_2d> emission;
		std::shared_ptr<sampler_2d> roughness;
		std::shared_ptr<sampler_2d> metallic;
	};

	/// <summary>
	/// A KD-tree for triangles. Internally used by Mesh to optimize intersection tests.
	/// </summary>
	class kd_tree_node
	{
	public:
		/// <summary>
		/// Constructs and empty node
		/// </summary>
		kd_tree_node() {}

		/// <summary>
		/// Recursively constructs a tree
		/// </summary>
		/// <param name="triangles">The triangles</param>
		/// <param name="bounds">A bounding box that contains all the triangles</param>
		/// <param name="depth">The depth of this node</param>
		kd_tree_node(const std::vector<triangle>& triangles, const bounding_box& bounds, uint32_t depth);

		/// <summary>
		/// Returns the maximum depth of the tree
		/// </summary>
		const uint32_t get_max_depth() const;

		/// <summary>
		/// Returns the triangles contained in this node
		/// </summary>
		const std::vector<triangle>& get_triangles() const { return m_triangles; }
		
		/// <summary>
		/// Returns the bounds of this node
		/// </summary>
		const bounding_box& get_bounds() const { return m_bounds; }

		/// <summary>
		/// Returns the left child, or nullptr if this node is a leaf
		/// </summary>
		const std::unique_ptr<kd_tree_node>& get_left() const { return m_left; }
		
		/// <summary>
		/// Returns the right child, or nullptr if this node is a leaf
		/// </summary>
		const std::unique_ptr<kd_tree_node>& get_right() const { return  m_right; }

	private:
		uint32_t m_depth = 0;
		bounding_box m_bounds;
		std::vector<triangle> m_triangles;
		std::unique_ptr<kd_tree_node> m_left = nullptr;
		std::unique_ptr<kd_tree_node> m_right = nullptr;
	};

	class object_id
	{
	public:
		const size_t id;
		object_id(): id(s_next++) {}
	private:
		static size_t s_next;
	};

	/// <summary>
	/// A generic shape
	/// </summary>
	class shape 
	{
	public:
		/// <summary>
		/// Optimize the shape before rendering
		/// </summary>
		virtual void compile() = 0;

		/// <summary>
		/// Intersection test with this shape
		/// </summary>
		/// <param name="ray">The ray, in local coordinates</param>
		/// <returns>The result of the intersection</returns>
		virtual raycast_result intersect(const ray& ray) const = 0;

		/// <summary>
		/// Get the local bounds of this shape
		/// </summary>
		/// <returns></returns>
		virtual const bounding_box& get_bounds() const = 0;
	};

	/// <summary>
	/// A sphere
	/// </summary>
	class sphere : public shape
	{
	public:
		void compile() override {}
		raycast_result intersect(const ray& ray) const override;
		const bounding_box& get_bounds() const override { return m_Bounds; }
	private:
		bounding_box m_Bounds = bounding_box({ -1, -1, -1 }, { 1, 1, 1 });
	};

	/// <summary>
	/// A triangle mesh
	/// </summary>
	class mesh: public shape, public object_id
	{
	private:
		

		raycast_result intersect_triangle(const ray& ray, const triangle& triangle) const;
		void intersect_internal(const ray& ray, const std::unique_ptr<kd_tree_node>& node, raycast_result& result, float& distance) const;

		bounding_box m_bounds;
		std::vector<triangle> m_triangles;
		std::unique_ptr<kd_tree_node> m_tree;
	
	public:
		const bounding_box& get_bounds() const override { return m_bounds; }
		
		/// <summary>
		/// Returns the triangles of this mesh
		/// </summary>
		const std::vector<triangle>& get_triangles() const { return m_triangles; }

		/// <summary>
		/// Adds a new triangle
		/// </summary>
		/// <returns>A reference to the new triangle</returns>
		triangle& add_triangle();


		raycast_result intersect(const ray& ray) const override;
		
		void compile() override;
		
		/// <summary>
		/// Returns the current KD-tree for this mesh
		/// </summary>
		const std::unique_ptr<kd_tree_node>& get_kd_tree() const { return m_tree; }
	};

	/// <summary>
	/// A scene node
	/// </summary>
	class scene_node
	{

	private:
		glm::mat4 m_transform = glm::identity<glm::mat4>();
		glm::mat4 m_inv_transform = glm::identity<glm::mat4>();
		glm::mat4 m_normal_transform = glm::identity<glm::mat4>();
		void update_matrices();

	public:

		/// <summary>
		/// The material
		/// </summary>
		rt::material material;

		/// <summary>
		/// The shape
		/// </summary>
		std::shared_ptr<rt::shape> shape = nullptr;
		
		/// <summary>
		/// Sets the current transform to an identity matrix
		/// </summary>
		void load_identity();

		/// <summary>
		/// Translate this node
		/// </summary>
		/// <param name="t">Translation</param>
		void translate(const glm::vec3& t);

		/// <summary>
		/// Rotate this node
		/// </summary>
		/// <param name="axis">The axis</param>
		/// <param name="angle">The angle of rotation (radians)</param>
		void rotate(const glm::vec3& axis, float angle);

		/// <summary>
		/// Scale this node
		/// </summary>
		/// <param name="s">The scale factor</param>
		void scale(const glm::vec3& s);

		/// <summary>
		/// Post-multiply the current transform by the given matrix
		/// </summary>
		/// <param name="mat">The matrix to multiply</param>
		void multiply(const glm::mat4& mat);

		/// <summary>
		/// Returns the current transform
		/// </summary>
		const glm::mat4& get_transform() const { return m_transform; }
		
		/// <summary>
		/// Returns the inverse of the current transform
		/// </summary>
		const glm::mat4& get_inverse_transform() const { return m_inv_transform; }
		
		/// <summary>
		/// Returns the transform to apply to normal vectors (inverse-transpose of the current transform)
		/// </summary>
		/// <returns></returns>
		const glm::mat4& get_normal_transform() const { return m_normal_transform; }
	};

	/// <summary>
	/// A scene
	/// </summary>
	class scene 
	{
	public:
		
		scene();

		/// <summary>
		/// The camera to use when rendering
		/// </summary>
		rt::camera camera;

		/// <summary>
		/// The scene's background
		/// </summary>
		std::shared_ptr<sampler_3d> background = nullptr;

		/// <summary>
		/// The scene's nodes
		/// </summary>
		std::vector<std::shared_ptr<scene_node>> nodes;

		/// <summary>
		/// Cast a ray on the scene
		/// </summary>
		/// <param name="ray">The ray</param>
		/// <param name="return_on_first_hit">If true, the function returns as soon as some node is hit</param>
		/// <param name="avoid_nodes">If non-emtpy, this nodes will not be considered for intersection tests</param>
		/// <returns>The closest intersection</returns>
		std::tuple<raycast_result, std::shared_ptr<scene_node>> cast_ray(const ray& ray, bool return_on_first_hit = false, const std::vector<std::shared_ptr<scene_node>>& avoid_nodes = {}) const;
		
		/// <summary>
		/// Get all the nodes that emit light. This takes into account the Emission property of the material
		/// </summary>
		const std::vector<std::shared_ptr<scene_node>>& get_light_sources() const { return m_light_sources; }

		void compile();

	private:
		std::vector<std::shared_ptr<scene_node>> m_light_sources;
	};
}