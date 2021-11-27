#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace rt
{
	class scene;
	class mesh;
}


namespace rtsb
{

	struct gl_vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct gl_scene_node
	{
		glm::mat4 transform = glm::identity<glm::mat4>();
		glm::vec3 color = glm::vec3(1.0f);
		uint32_t vao_id = 0;
		size_t vertex_count = 0;
	};

	struct gl_vao
	{
		uint32_t id = 0;
		uint32_t vb_id = 0;
		size_t vertex_count = 0;
	};

	struct gl_program
	{
		uint32_t id = 0;
		uint32_t vs_id = 0;
		uint32_t fs_id = 0;
	};


	class gl_scene_renderer
	{
	public:
		gl_scene_renderer(const rt::scene& scene);
		~gl_scene_renderer();
		
		void render(const glm::vec3& camera_pos, const glm::vec3& camera_dir, const float fov_y, const float aspect);

	private:
		gl_program m_program;

		gl_vao m_shere_vao;
		std::unordered_map<size_t, gl_vao> m_vao;
		
		std::vector<gl_scene_node> m_nodes;

		gl_vao create_mesh(rt::mesh& mesh) const;
		gl_vao create_vao(const std::vector<gl_vertex>& vertices) const;
		gl_program load_program(std::string_view vs_source, std::string_view fs_source) const;
	};
}
