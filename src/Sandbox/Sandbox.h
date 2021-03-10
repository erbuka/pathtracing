#include <cinttypes>
#include <vector>
#include <chrono>
#include <future>
#include <map>
#include <string_view>
#include <json.hpp>
#include <string>
#include <filesystem>

#include <Scene.h>
#include <DebugRaytracer.h>
#include <Pathtracer.h>

#include "GLSceneRenderer.h"


struct GLFWwindow;
struct ImFont;

namespace sandbox
{
	class GLSceneRenderer;

	enum class SandboxState
	{
		Idle,
		Rendering,
		Result
	};

	struct Toast
	{
		std::string Title;
		std::string Message;
		std::chrono::milliseconds EndTime;
		Toast(const std::string& title, const std::string& message);
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
		std::pair<float, float> GetScaledWindowSize(float width) const;

		void OnWheel(float offsetX, float offsetY);
		void OnMousePressed(MouseButton button);
		void OnMouseReleased(MouseButton button);
		void OnMouseMoved(const glm::vec2& pos);

		glm::vec2 GetMousePostion() const;

	private:

		struct CameraSettings {
			float MoveSpeed = 1.5f;
			float RotateSpeed = 3.0f;
			glm::vec3 LookAt = { 0.0f, 0.0f, 0.0f };
		} m_CameraSettings;

		struct {
			glm::vec2 Position = { 0.0f, 0.0f };
			std::map<MouseButton, bool> Button = {
				{ MouseButton::Left, false },
				{ MouseButton::Right, false },
				{ MouseButton::Other, false }
			};
		} m_Mouse;

		SandboxState m_State = SandboxState::Idle;

		std::vector<std::filesystem::path> m_SceneFiles;

		rt::Scene m_Scene;

		rt::utility::DebugRaytracer m_Debug;
		rt::Pathtracer m_Pathtracer;
		std::unique_ptr<GLSceneRenderer> m_GLRenderer;

		rt::Image m_Image;

		std::shared_ptr<rt::RaytracerResult> m_RenderResult = nullptr;
		uint32_t m_RenderTexture;

		std::mutex m_ImageMutex;

		GLFWwindow* m_Window;
		std::vector<uint32_t> m_Pixels;

		std::vector<Toast> m_Toasts;

		ImFont* m_Font;

		bool m_Running = false;
		bool m_TextureNeedsUpdate = false;
		size_t m_CurrentIteration = 0;

		void Initialize();
		void Update();
		void RenderGUI();

		void LoadSceneDefinitions();

		void UpdateTexture();
		void SaveImage();

		std::tuple<float, float> SphericalAngles(const glm::vec3& dir);

	};
}