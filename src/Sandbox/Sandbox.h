#include <cinttypes>
#include <vector>
#include <future>
#include <map>
#include <string_view>
#include <json.hpp>

#include "Scene.h"
#include "DebugRaytracer.h"
#include "Raytracer.h"


struct GLFWwindow;
struct ImFont;

namespace sandbox
{

	enum class SandboxState
	{
		Idle,
		Rendering,
		Result
	};

	class Sandbox {
	public:

		enum class MouseButton : uint32_t {
			Left = 0,
			Right,
			Other
		};


		bool Start();

		std::pair<float, float> GetWindowSize() const;

		void OnWheel(float offsetX, float offsetY);
		void OnMousePressed(MouseButton button);
		void OnMouseReleased(MouseButton button);
		void OnMouseMoved(const glm::vec2& pos);

		glm::vec2 GetMousePostion() const;

	private:

		rt::ViewParameters GetDebugiViewParameters() const;
		rt::ViewParameters GetViewParameters() const;

		void Initialize();
		void Update();
		void RenderGUI();

		void LoadSceneDefinitions();
		void LoadScene(const nlohmann::json& sceneDef);

		void ImageToTexture(const rt::Image& image, uint32_t texId);

		struct Camera {
			float MoveSpeed = 1.5f;
			float RotateSpeed = 3.0f;
			float Alpha = -glm::pi<float>() / 2.0f;
			float Beta = 0.0f;
			float Distance = 3.0f;
			glm::vec3 LookAt = { 0.0f, 0.0f, 0.0f };
			glm::vec3 GetDirection() const;
			glm::vec3 GetPosition() const;
		} m_Camera;

		struct {
			glm::vec2 Position = { 0.0f, 0.0f };
			std::map<MouseButton, bool> Button = {
				{ MouseButton::Left, false },
				{ MouseButton::Right, false },
				{ MouseButton::Other, false }
			};
		} m_Mouse;

		SandboxState m_State = SandboxState::Idle;

		std::vector<nlohmann::json> m_SceneDefs;

		rt::Scene m_Scene;

		rt::DebugRaytracer m_Debug;
		rt::Raytracer m_Raytracer;

		std::shared_ptr<rt::RaytracerResult> m_RenderResult = nullptr;
		uint32_t m_RenderTexture, m_DebugTexture;

		GLFWwindow* m_Window;
		std::vector<uint32_t> m_Pixels;

		ImFont* m_Font;

		bool m_Running = false;
		size_t m_CurrentIteration = 0;

	};
}