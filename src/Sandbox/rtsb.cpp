#include "rtsb.h"

#include <fstream>
#include <regex>
#include <sstream>

#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>

#include <mesh_loader.h>
#include <scene_loader.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define RT_SANDBOX(window) ((sandbox*)glfwGetWindowUserPointer(window))

namespace rtsb
{


    static constexpr float s_fov_y = glm::pi<float>() / 4.0f;

    toast::toast(const std::string& title, const std::string& message) :
        title(title),
        message(message)
    {
        end_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) +
            std::chrono::milliseconds(2500);
    }

    static void gl_debug_message(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {

        if (type == GL_DEBUG_TYPE_ERROR)
        {
            spdlog::error("OpenGL Error severity: {0:x}, message: {1}", severity, message);
        }
    }

    static void glfw_cursor_pos(GLFWwindow* window, double xpos, double ypos)
    {
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse)
            return;

        RT_SANDBOX(window)->on_mouse_moved({ xpos, ypos });
    }

    static void glfw_mouse_button(GLFWwindow* window, int button, int action, int mods)
    {
        auto& io = ImGui::GetIO();
        
        if (io.WantCaptureMouse)
            return;

        sandbox::mouse_button btn;

        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT: btn = sandbox::mouse_button::left; break;
        case GLFW_MOUSE_BUTTON_RIGHT: btn = sandbox::mouse_button::right; break;
        default: btn = sandbox::mouse_button::other; break;
        }

        if (action == GLFW_PRESS)
        {
            RT_SANDBOX(window)->on_mouse_pressed(btn);
        }
        else if (action == GLFW_RELEASE)
        {
            RT_SANDBOX(window)->on_mouse_released(btn);
        }
    }

    static void glfw_scroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse)
            return;

        RT_SANDBOX(window)->on_wheel(xoffset, yoffset);
    }

    bool sandbox::start()
    {

        /* Initialize the library */
        if (!glfwInit()) {
            return false;
        }

        /* Create a windowed mode window and its OpenGL context */
        m_window = glfwCreateWindow(1280, 768, "Pathtracing - Sandbox", NULL, NULL);
        if (!m_window)
        {
            glfwTerminate();
            return false;
        }


        /* Make the window's context current */
        glfwMakeContextCurrent(m_window);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        spdlog::info("OpenGL Version: {0}", (const char*)glGetString(GL_VERSION));

        glDebugMessageCallback(&gl_debug_message, nullptr);

        // Events
        glfwSetWindowUserPointer(m_window, this);
        glfwSetScrollCallback(m_window, &glfw_scroll);
        glfwSetMouseButtonCallback(m_window, &glfw_mouse_button);
        glfwSetCursorPosCallback(m_window, &glfw_cursor_pos);

        initialize();

        m_running = true;

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(m_window) && m_running)
        {

            update();
            render_gui();

            /* Swap front and back buffers */
            glfwSwapBuffers(m_window);

            /* Poll for and process events */
            glfwPollEvents();
        }

        if (m_render_result)
        {
            m_render_result->interrupt();
            m_render_result->wait();
        }

        glfwTerminate();
        return 0;
    }


    std::pair<float, float> sandbox::get_window_size() const
    {
        int32_t w, h;
        glfwGetWindowSize(m_window, &w, &h);
        return { w, h };
    }

    std::pair<float, float> rtsb::sandbox::get_scaled_window_size(float width) const
    {
        auto [w, h] = get_window_size();
        return { glm::round(width), glm::round(h / w * width) };
    }

    void sandbox::on_wheel(float offset_x, float offset_y)
    {
        auto& camera = m_scene.camera;
        float distance = glm::length(camera.position - m_camera_settings.look_at);
        float offset = glm::max(distance + m_camera_settings.move_speed * offset_y, 0.1f);
        camera.position = m_camera_settings.look_at - camera.get_direction() * offset;
    }

    void sandbox::on_mouse_pressed(mouse_button button)
    {
        m_mouse.position = get_mouse_postion();
        m_mouse.button[button] = true;
    }

    void sandbox::on_mouse_released(mouse_button button)
    {
        m_mouse.button[button] = false;
    }

    void sandbox::on_mouse_moved(const glm::vec2& pos)
    {
        if (m_state == sandbox_state::idle && m_mouse.button[mouse_button::left])
        {
            auto& camera = m_scene.camera;
            auto [w, h] = get_window_size();
            auto delta = (pos - m_mouse.position) / glm::vec2(w, h);

            const float distance = glm::length(m_camera_settings.look_at - camera.position);

            auto [beta, alpha] = spherical_angles(camera.get_direction());

            beta += delta.y * m_camera_settings.rotate_speed;
            alpha -= delta.x * m_camera_settings.rotate_speed;

            camera.set_direction({
                glm::sin(alpha) * glm::sin(beta),
                glm::cos(beta),
                glm::cos(alpha) * glm::sin(beta)
                });

            camera.position = m_camera_settings.look_at - camera.get_direction() * distance;
        }
        m_mouse.position = pos;
    }

    glm::vec2 sandbox::get_mouse_postion() const
    {
        double x, y;
        glfwGetCursorPos(m_window, &x, &y);
        return { x, y };
    }

    void sandbox::initialize()
    {
        // Load scene definitions
        load_scene_definitions();

        // Render texture
        glGenTextures(1, &m_render_texture);
        glBindTexture(GL_TEXTURE_2D, m_render_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        m_font = io.Fonts->AddFontFromFileTTF("res/fonts/roboto.ttf", 24);

        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

    }

    void sandbox::update()
    {

        const auto [width, height] = get_window_size();

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glUseProgram(0);


        if(m_state == sandbox_state::idle)
        {
            if (m_gl_renderer)
                m_gl_renderer->render(m_scene.camera.position, m_scene.camera.get_direction(), s_fov_y, width / height);
        }
        else
        {
            if (m_render_result->is_interrupted())
                m_state = sandbox_state::result;

            update_texture();

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, m_render_texture);

            glBegin(GL_QUADS);
            {
                glColor3f(1, 1, 1);
                glTexCoord2d(0, 1); glVertex2f(-1, -1);
                glTexCoord2d(1, 1); glVertex2f(1, -1);
                glTexCoord2d(1, 0); glVertex2f(1, 1);
                glTexCoord2d(0, 0); glVertex2f(-1, 1);
            }
            glEnd();
        }


    }

    void rtsb::sandbox::render_gui()
    {

        const rt::trace_parameters renderTraceParams = {
            std::thread::hardware_concurrency() - 1,
            0,
            16
        };

        const rt::trace_parameters debugTraceParams = {
            std::thread::hardware_concurrency() - 1,
            1,
            1
        };

        m_toasts.erase(std::remove_if(m_toasts.begin(), m_toasts.end(), [](const toast& t) {
            return t.end_time < std::chrono::system_clock::now().time_since_epoch();
        }), m_toasts.end());

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(m_font);

        if (m_state == sandbox_state::idle)
        {
            ImGui::BeginMainMenuBar();
            if (ImGui::BeginMenu("File"))
            {



                if (ImGui::BeginMenu("Render"))
                {
                    const auto modes = {
                        std::make_tuple("Pathtracer", static_cast<rt::abstract_pathtracer*>(&m_pathtracer)),
                    };

                    const auto debug_modes = {
                        std::make_tuple("Albedo", rt::utility::debug_pathtracer::mode::albedo),
                        std::make_tuple("Normals", rt::utility::debug_pathtracer::mode::normal),
                    };

                    for (const auto& [title, renderer] : modes)
                    {
                        if (ImGui::BeginMenu(title))
                        {
                            for (const size_t s : { 64, 128, 256, 512, 1024 })
                            {
                                std::stringstream ss;
                                ss << s << " px";

                                if (ImGui::MenuItem(ss.str().c_str()))
                                {
                                    rt::view_parameters view_params;
                                    auto [vw, vh] = get_scaled_window_size(s);

                                    view_params.width = vw;
                                    view_params.height = vh;
                                    view_params.fov_y = s_fov_y;

                                    m_render_stats.current_iteration = 0;
                                    m_render_result = renderer->run(view_params, renderTraceParams, m_scene);
                                    m_render_result->on_iteration_end.subscribe(this, &sandbox::on_iteration_end_handler);
                                    m_state = sandbox_state::rendering;
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }


                    if (ImGui::BeginMenu("Debug"))
                    {
                        for (const auto& [title, mode] : debug_modes)
                        {
                            if (ImGui::MenuItem(title))
                            {
                                rt::view_parameters view_params;
                                auto [vw, vh] = get_window_size();

                                view_params.width = vw;
                                view_params.height = vh;
                                view_params.fov_y = s_fov_y;
                                m_debug.current_mode = mode;
                                m_render_result = m_debug.run(view_params, debugTraceParams, m_scene);
                                m_render_result->on_iteration_end.subscribe(this, &sandbox::on_iteration_end_handler);
                                m_state = sandbox_state::rendering;
                            }
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Load Scene"))
                {
                    for (const auto& file : m_scene_files)
                    {
                        if (ImGui::MenuItem(file.filename().string().c_str()))
                        {
                            m_scene = rt::utility::load_scene(file.string());
                            m_gl_renderer = std::make_unique<gl_scene_renderer>(m_scene);
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Quit"))
                {
                    m_running = false;
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();

            auto eye = m_scene.camera.position;
            auto direction = m_scene.camera.get_direction();

            ImGui::SetNextWindowPos({ 10.0f, 40.0f });
            ImGui::SetNextWindowSize({ 300.0f, -1.0f });
            ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoDecoration);
            ImGui::InputFloat3("Eye", glm::value_ptr(eye), "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat3("Direction", glm::value_ptr(direction), "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::End();

        }
        else if (m_state == sandbox_state::rendering)
        {
            const auto iteration = m_render_result->iteration.load();

            ImGui::SetNextWindowPos({ 10.0f, 10.0f });
            ImGui::SetNextWindowSize({ 300.0f, -1.0f });
            ImGui::Begin("Render", nullptr, ImGuiWindowFlags_NoDecoration);
            ImGui::Text("Elapsted Time: %.2f", m_render_result->get_elapsed_time());
            ImGui::Text("%.2f spp/second", m_render_stats.ssp_per_second);
            ImGui::Text("iteration #%d", iteration);
            ImGui::ProgressBar(m_render_result->progress);
            if (ImGui::Button("Interrupt", { -1.0f, 0.0f })) {
                m_render_result->interrupt();
            }
            ImGui::End();
        }
        else if (m_state == sandbox_state::result)
        {
            ImGui::SetNextWindowPos({ 10.0f, 10.0f });
            ImGui::SetNextWindowSize({ 300.0f, -1.0f });
            ImGui::Begin("Render", nullptr, ImGuiWindowFlags_NoDecoration);
            if (ImGui::Button("Save", { -1.0f, 0.0f }))
            {
                save_image();
            }
            if (ImGui::Button("Back", { -1.0f, 0.0f }))
            {
                m_render_result = nullptr;
                m_state = sandbox_state::idle;
            }
            ImGui::End();
        }

        const auto [width, height] = get_window_size();
        constexpr float toastWidth = 350.0f;
        constexpr float toastPadding = 10.0f;

        ImVec2 pos = { width - (toastWidth + toastPadding), toastPadding };
        size_t index = 0;
        for (const auto& toast : m_toasts)
        {
            std::stringstream ss;
            ss << "Toast-" << index++;
            ImGui::SetNextWindowSize({ toastWidth, -1.0f });
            ImGui::SetNextWindowPos(pos);
            ImGui::Begin(ss.str().c_str(), nullptr, ImGuiWindowFlags_NoDecoration);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), toast.title.c_str());
            ImGui::TextWrapped(toast.message.c_str());
            pos.y += ImGui::GetWindowSize().y + toastPadding;
            ImGui::End();
        }

        ImGui::PopFont();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void rtsb::sandbox::load_scene_definitions()
    {
        namespace fs = std::filesystem;

        for (const auto& entry : fs::directory_iterator("res/scenes"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                m_scene_files.push_back(entry.path());
            }
        }

    }

    void sandbox::update_texture()
    {
        std::lock_guard guard(m_image_mutex);
        
        if (m_texture_needs_update)
        {

            auto& image = m_image;

            m_pixels.resize(image.get_width() * image.get_height());

            for (size_t x = 0; x < image.get_width(); ++x) {
                for (size_t y = 0; y < image.get_height(); ++y) {
                    auto color = image.get_pixel(x, y);
                    
                    
                    // Tone mapping
                    color = glm::vec3(1.0f) - glm::exp(-color);

                    // Gamma correction
                    color = glm::pow(color, glm::vec3(1.0f / 2.2f));

                    m_pixels[y * image.get_width() + x] =
                        ((uint32_t(color.r * 255) << 0)) |
                        ((uint32_t(color.g * 255) << 8)) |
                        ((uint32_t(color.b * 255) << 16)) |
                        ((uint32_t(255) << 24));

                }
            }

            glBindTexture(GL_TEXTURE_2D, m_render_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.get_width(), image.get_height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data());
            m_texture_needs_update = false;
        }
    }

	void rtsb::sandbox::save_image()
	{
        const size_t w = m_image.get_width();
        const size_t h = m_image.get_height();
        std::vector<uint32_t> data;
        auto now = std::chrono::system_clock::now();
        std::stringstream path;

        data.resize(w * h);
        glBindTexture(GL_TEXTURE_2D, m_render_texture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        
        path << "res/screenshots/" << std::hex << now.time_since_epoch().count() << ".png";
        stbi_write_png(path.str().c_str(), w, h, 4, data.data(), 0);

        m_toasts.emplace_back("Screenshot Saved", path.str());

	}

    void rtsb::sandbox::on_iteration_end_handler(const rt::image& image, const uint64_t& iteration)
    {
        std::lock_guard guard(m_image_mutex);
        m_image = image;
        m_texture_needs_update = true;
        m_render_stats.current_iteration = iteration + 1;
        m_render_stats.ssp_per_second = m_render_result->samples_per_pixel.load() / m_render_result->get_elapsed_time();
    }

    std::tuple<float, float> sandbox::spherical_angles(const glm::vec3& dir)
    {
        const float beta = glm::acos(glm::dot(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
        //float beta = glm::atan(glm::sqrt(dir.x * dir.x + dir.z * dir.z) / dir.y);
        float alpha = std::atan2(dir.x, dir.z);
        return { beta, alpha };
    }

}