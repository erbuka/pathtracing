#include "GLSceneRenderer.h"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <Scene.h>
#include <MeshLoader.h>

namespace sandbox
{

	static constexpr std::string_view s_VertexShader = R"Shader(
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

	static constexpr std::string_view s_FragmentShader = R"Shader(
		#version 330
	
		uniform vec3 uColor;	

		out vec4 oColor;

		void main() {
			oColor = vec4(uColor, 1.0);
		}
	)Shader";


	GLSceneRenderer::GLSceneRenderer(const rt::Scene& scene)
	{
		static constexpr std::array<glm::vec3, 7> s_Colors = {
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(1.0f, 0.0f, 1.0f),
			glm::vec3(0.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
		};

		auto sphereMesh = rt::utility::LoadMeshesFromWavefront("res/meshes/sphere.obj")["sphere"];
		m_ShereVao = CreateMesh(*sphereMesh);

		m_Program = LoadProgram(s_VertexShader, s_FragmentShader);

		size_t idx = 0;

		for (const auto& node : scene.Nodes)
		{
			++idx;

			auto mesh = std::dynamic_pointer_cast<rt::Mesh>(node->Shape);
			auto sphere = std::dynamic_pointer_cast<rt::Sphere>(node->Shape);
			if (mesh != nullptr)
			{
				if (m_Vao.find(mesh->ID) == m_Vao.end())
					m_Vao[mesh->ID] = CreateMesh(*mesh);

				const auto& vao = m_Vao[mesh->ID];

				m_Nodes.push_back({ 
					node->Transform, 
					s_Colors[idx % s_Colors.size()],
					vao.ID,
					vao.VertexCount
				});

			}
			else if (sphere)
			{
				m_Nodes.push_back({ 
					node->Transform,
					s_Colors[idx % s_Colors.size()],
					m_ShereVao.ID,
					m_ShereVao.VertexCount
				});
			}
		
		}

	}



	GLSceneRenderer::~GLSceneRenderer()
	{

		glDeleteProgram(m_Program.ID);
		glDeleteShader(m_Program.FsID);
		glDeleteShader(m_Program.VsID);

		glDeleteVertexArrays(1, &(m_ShereVao.ID));
		glDeleteBuffers(1, &(m_ShereVao.VbID));

		for (const auto [key, val] : m_Vao)
		{
			glDeleteVertexArrays(1, &(val.ID));
			glDeleteBuffers(1, &(val.VbID));
		}
	}


	void GLSceneRenderer::Render(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const float fovY, const float aspect)
	{
		const auto projection = glm::perspective(fovY, aspect, 0.1f, 100.0f);
		const auto view = glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0.0f, 1.0f, 0.0f));

		const uint32_t program = m_Program.ID;

		glUseProgram(program);

		const auto projectionLoc = glGetUniformLocation(program, "uProjection");
		const auto viewLoc = glGetUniformLocation(program, "uView");
		const auto modelLoc = glGetUniformLocation(program, "uModel");
		const auto colorLoc = glGetUniformLocation(program, "uColor");
		
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		for (const auto& node : m_Nodes)
		{ 
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(node.Transform));
			glUniform3fv(colorLoc, 1, glm::value_ptr(node.Color));
			glBindVertexArray(node.VaoID);
			glDrawArrays(GL_TRIANGLES, 0, node.VertexCount);
			glBindVertexArray(0);
		}

	}

	GLVao GLSceneRenderer::CreateMesh(rt::Mesh& mesh) const
	{
		std::vector<GLVertex> vertices;
		vertices.reserve(mesh.GetTriangles().size() * 3);

		for (const auto& triangle : mesh.GetTriangles())
			for (const auto& v : triangle.Vertices)
				vertices.push_back({ v.Position, v.Normal, v.UV });

		uint32_t vao, vb;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vb);
		glBindBuffer(GL_ARRAY_BUFFER, vb);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLVertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)12);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)24);

		return { vao, vb, vertices.size() };

	}

	GLProgram GLSceneRenderer::LoadProgram(std::string_view vsSource, std::string_view fsSource) const
	{
		const auto loadShader = [](std::string_view source, GLenum type) -> uint32_t {
			const uint32_t shader = glCreateShader(type);

			const GLint len = source.size();
			const GLchar* data = source.data();

			glShaderSource(shader, 1, &data, &len);

			glCompileShader(shader);

			GLint compileStatus;

			glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

			if (compileStatus == GL_FALSE)
			{
				glDeleteShader(shader);
				spdlog::error("Shader compile error:\n{0}", source.data());
				return 0;
			}

			return shader;
		};
		
		const auto vs = loadShader(vsSource, GL_VERTEX_SHADER);
		const auto fs = loadShader(fsSource, GL_FRAGMENT_SHADER);

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