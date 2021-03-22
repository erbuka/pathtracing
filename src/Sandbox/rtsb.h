#include <cinttypes>
#include <vector>
#include <chrono>
#include <future>
#include <map>
#include <string_view>
#include <json.hpp>
#include <string>
#include <filesystem>

#include <scene.h>
#include <debug_pathtracer.h>
#include <pathtracer.h>

#include "gl_scene_renderer.h"

struct GLFWwindow;
struct ImFont;

namespace rtsb
{
	class gl_scene_renderer;

	enum class sandbox_state
	{
		idle,
		rendering,
		result
	};

	struct toast
	{
		std::string title;
		std::string message;
		std::chrono::milliseconds end_time;
		toast(const std::string& title, const std::string& message);
	};

	class sandbox {
	public:

		enum class mouse_button : uint32_t {
			left = 0,
			right,
			other
		};


		bool start();

		std::pair<float, float> get_window_size() const;
		std::pair<float, float> get_scaled_window_size(float width) const;

		void on_wheel(float offset_x, float offset_y);
		void on_mouse_pressed(mouse_button button);
		void on_mouse_released(mouse_button button);
		void on_mouse_moved(const glm::vec2& pos);

		glm::vec2 get_mouse_postion() const;

	private:

		struct camera_settings {
			float move_speed = 1.5f;
			float rotate_speed = 3.0f;
			glm::vec3 look_at = { 0.0f, 0.0f, 0.0f };
		} m_camera_settings;

		struct {
			glm::vec2 position = { 0.0f, 0.0f };
			std::map<mouse_button, bool> button = {
				{ mouse_button::left, false },
				{ mouse_button::right, false },
				{ mouse_button::other, false }
			};
		} m_mouse;

		struct {
			uint64_t current_iteration = 0;
			float ssp_per_second = 0.0f; 
		} m_render_stats;

		sandbox_state m_state = sandbox_state::idle;

		std::vector<std::filesystem::path> m_scene_files;

		rt::scene m_scene;

		rt::utility::debug_pathtracer m_debug;
		rt::pathtracer m_pathtracer;
		std::unique_ptr<gl_scene_renderer> m_gl_renderer;

		rt::image m_image;

		std::shared_ptr<rt::pathtracer_result> m_render_result = nullptr;
		uint32_t m_render_texture;

		std::mutex m_image_mutex;

		GLFWwindow* m_window;
		std::vector<uint32_t> m_pixels;

		std::vector<toast> m_toasts;

		ImFont* m_font;

		bool m_running = false;
		bool m_texture_needs_update = false;

		void initialize();
		void update();
		void render_gui();

		void load_scene_definitions();

		void update_texture();
		void save_image();
		
		void on_iteration_end_handler(const rt::image& image, const uint64_t& iteration);

		std::tuple<float, float> spherical_angles(const glm::vec3& dir);

	};
}