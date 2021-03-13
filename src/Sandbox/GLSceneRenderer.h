#pragma once

#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace rt
{
	class Scene;
	class Mesh;
}


namespace sandbox
{

	struct GLVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 Uv;
	};

	struct GLSceneNode
	{
		glm::mat4 Transform = glm::identity<glm::mat4>();
		glm::vec3 Color = glm::vec3(1.0f);
		uint32_t VaoID = 0;
		size_t VertexCount = 0;
	};

	struct GLVao
	{
		uint32_t ID = 0;
		uint32_t VbID = 0;
		size_t VertexCount = 0;
	};

	struct GLProgram
	{
		uint32_t ID = 0;
		uint32_t VsID = 0;
		uint32_t FsID = 0;
	};


	class GLSceneRenderer
	{
	public:
		GLSceneRenderer(const rt::Scene& scene);
		~GLSceneRenderer();
		
		void Render(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const float fovY, const float aspect);

	private:
		GLProgram m_Program;

		GLVao m_ShereVao;
		std::unordered_map<size_t, GLVao> m_Vao;
		
		std::vector<GLSceneNode> m_Nodes;

		GLVao CreateMesh(rt::Mesh& mesh) const;
		GLVao CreateVao(const std::vector<GLVertex>& vertices) const;
		GLProgram LoadProgram(std::string_view vsSource, std::string_view fsSource) const;
	};
}