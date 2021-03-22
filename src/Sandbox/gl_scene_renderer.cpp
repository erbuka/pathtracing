#include "gl_scene_renderer.h"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <scene.h>
#include <mesh_loader.h>

namespace rtsb
{

	static constexpr std::string_view s_vertex_shader = R"Shader(
		#version 330

		uniform mat4 uProjection;
		uniform mat4 uView;
		uniform mat4 uModel;

		layout(location = 0) in vec3 aPosition;
		layout(location = 1) in vec3 aNormal;
		layout(location = 2) in vec2 aUv;

		void main() {
			gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
		}
	)Shader";

	static constexpr std::string_view s_fragment_shader = R"Shader(
		#version 330
	
		uniform vec3 uColor;	
			
		out vec4 oColor;

		void main() {
			oColor = vec4(uColor, 1.0);
		}
	)Shader";


	gl_scene_renderer::gl_scene_renderer(const rt::scene& scene)
	{
		static constexpr std::array<glm::vec3, 7> s_colors = {
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(1.0f, 0.0f, 1.0f),
			glm::vec3(0.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
		};


		auto sphere_mesh = rt::utility::load_meshes_from_wavefront("res/meshes/sphere.obj")["sphere"];
		m_shere_vao = create_mesh(*sphere_mesh);

		m_program = load_program(s_vertex_shader, s_fragment_shader);

		size_t idx = 0;

		for (const auto& node : scene.nodes)
		{
			++idx;

			auto mesh = std::dynamic_pointer_cast<rt::mesh>(node->shape);
			auto sphere = std::dynamic_pointer_cast<rt::sphere>(node->shape);
			if (mesh != nullptr)
			{
				if (m_vao.find(mesh->id) == m_vao.end())
					m_vao[mesh->id] = create_mesh(*mesh);

				const auto& vao = m_vao[mesh->id];

				m_nodes.push_back({ 
					node->get_transform(), 
					s_colors[idx % s_colors.size()],
					vao.id,
					vao.vertex_count
				});

			}
			else if (sphere)
			{
				m_nodes.push_back({ 
					node->get_transform(),
					s_colors[idx % s_colors.size()],
					m_shere_vao.id,
					m_shere_vao.vertex_count
				});
			}
		
		}

	}



	gl_scene_renderer::~gl_scene_renderer()
	{

		glDeleteProgram(m_program.id);
		glDeleteShader(m_program.fs_id);
		glDeleteShader(m_program.vs_id);

		glDeleteVertexArrays(1, &(m_shere_vao.id));
		glDeleteBuffers(1, &(m_shere_vao.vb_id));

		for (const auto [key, val] : m_vao)
		{
			glDeleteVertexArrays(1, &(val.id));
			glDeleteBuffers(1, &(val.vb_id));
		}
	}


	void gl_scene_renderer::render(const glm::vec3& camera_pos, const glm::vec3& camera_dir, const float fov_y, const float aspect)
	{
		const auto projection = glm::perspective(fov_y, aspect, 0.1f, 100.0f);
		const auto view = glm::lookAt(camera_pos, camera_pos + camera_dir, glm::vec3(0.0f, 1.0f, 0.0f));

		const uint32_t program = m_program.id;

		glUseProgram(program);

		const auto projection_loc = glGetUniformLocation(program, "uProjection");
		const auto view_loc = glGetUniformLocation(program, "uView");
		const auto model_loc = glGetUniformLocation(program, "uModel");
		const auto color_loc = glGetUniformLocation(program, "uColor");
		
		glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));

		for (const auto& node : m_nodes)
		{ 
			glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(node.transform));
			glUniform3fv(color_loc, 1, glm::value_ptr(node.color));
			glBindVertexArray(node.vao_id);
			glDrawArrays(GL_TRIANGLES, 0, node.vertex_count);
			glBindVertexArray(0);
		}

	}


	gl_vao gl_scene_renderer::create_mesh(rt::mesh& mesh) const
	{
		std::vector<gl_vertex> vertices;
		vertices.reserve(mesh.get_triangles().size() * 3);

		for (const auto& triangle : mesh.get_triangles())
			for (const auto& v : triangle.vertices)
				vertices.push_back({ v.position, v.normal, v.uv });

		return create_vao(vertices);

	}

	gl_vao gl_scene_renderer::create_vao(const std::vector<gl_vertex>& vertices) const
	{
		uint32_t vao, vb;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vb);
		glBindBuffer(GL_ARRAY_BUFFER, vb);
		glBufferData(GL_ARRAY_BUFFER, sizeof(gl_vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(gl_vertex), (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(gl_vertex), (void*)12);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(gl_vertex), (void*)24);

		return { vao, vb, vertices.size() };
	}

	gl_program gl_scene_renderer::load_program(std::string_view vs_source, std::string_view fs_source) const
	{
		const auto load_shader = [](std::string_view source, GLenum type) -> uint32_t {
			const uint32_t shader = glCreateShader(type);

			const GLint len = source.size();
			const GLchar* data = source.data();

			glShaderSource(shader, 1, &data, &len);

			glCompileShader(shader);

			GLint compile_status;

			glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

			if (compile_status == GL_FALSE)
			{
				glDeleteShader(shader);
				spdlog::error("Shader compile error:\n{0}", source.data());
				return 0;
			}

			return shader;
		};
		
		const auto vs = load_shader(vs_source, GL_VERTEX_SHADER);
		const auto fs = load_shader(fs_source, GL_FRAGMENT_SHADER);

		const uint32_t program = glCreateProgram();

		glAttachShader(program, vs);
		glAttachShader(program, fs);

		GLint status;

		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			glDeleteShader(fs);
			glDeleteShader(vs);
			glDeleteProgram(program);
			spdlog::error("Can't link program");
		}


		glDetachShader(program, vs);
		glDetachShader(program, fs);

		return { program, vs, fs };

	}
}