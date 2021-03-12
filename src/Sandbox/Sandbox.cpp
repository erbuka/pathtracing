#include "Sandbox.h"

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

#include <MeshLoader.h>
#include <SceneLoader.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define RT_SANDBOX(window) ((Sandbox*)glfwGetWindowUserPointer(window))

namespace sandbox
{

    static constexpr float s_FovY = glm::pi<float>() / 4.0f;

    Toast::Toast(const std::string& title, const std::string& message) :
        Title(title),
        Message(message)
    {
        EndTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) +
            std::chrono::milliseconds(2500);
    }


    static void GLFW_CursorPos(GLFWwindow* window, double xpos, double ypos)
    {
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse)
            return;

        RT_SANDBOX(window)->OnMouseMoved({ xpos, ypos });
    }

    static void GLFW_MouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        auto& io = ImGui::GetIO();
        
        if (io.WantCaptureMouse)
            return;

        Sandbox::MouseButton btn;

        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT: btn = Sandbox::MouseButton::Left; break;
        case GLFW_MOUSE_BUTTON_RIGHT: btn = Sandbox::MouseButton::Right; break;
        default: btn = Sandbox::MouseButton::Other; break;
        }

        if (action == GLFW_PRESS)
        {
            RT_SANDBOX(window)->OnMousePressed(btn);
        }
        else if (action == GLFW_RELEASE)
        {
            RT_SANDBOX(window)->OnMouseReleased(btn);
        }
    }

    static void GLFW_Scroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse)
            return;

        RT_SANDBOX(window)->OnWheel(xoffset, yoffset);
    }

    bool Sandbox::Start()
    {

        /* Initialize the library */
        if (!glfwInit()) {
            return false;
        }

        /* Create a windowed mode window and its OpenGL context */
        m_Window = glfwCreateWindow(1280, 768, "Hello World", NULL, NULL);
        if (!m_Window)
        {
            glfwTerminate();
            return false;
        }


        /* Make the window's context current */
        glfwMakeContextCurrent(m_Window);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        // Events
        glfwSetWindowUserPointer(m_Window, this);
        glfwSetScrollCallback(m_Window, &GLFW_Scroll);
        glfwSetMouseButtonCallback(m_Window, &GLFW_MouseButton);
        glfwSetCursorPosCallback(m_Window, &GLFW_CursorPos);

        Initialize();

        m_Running = true;

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(m_Window) && m_Running)
        {

            Update();
            RenderGUI();

            /* Swap front and back buffers */
            glfwSwapBuffers(m_Window);

            /* Poll for and process events */
            glfwPollEvents();
        }

        if (m_RenderResult)
        {
            m_RenderResult->Interrupt();
            m_RenderResult->Wait();
        }

        glfwTerminate();
        return 0;
    }


    std::pair<float, float> Sandbox::GetWindowSize() const
    {
        int32_t w, h;
        glfwGetWindowSize(m_Window, &w, &h);
        return { w, h };
    }

    std::pair<float, float> sandbox::Sandbox::GetScaledWindowSize(float width) const
    {
        auto [w, h] = GetWindowSize();
        return { glm::round(width), glm::round(h / w * width) };
    }

    void Sandbox::OnWheel(float offsetX, float offsetY)
    {
        auto& camera = m_Scene.Camera;
        float distance = glm::length(camera.Position - m_CameraSettings.LookAt);
        float offset = glm::max(distance + m_CameraSettings.MoveSpeed * offsetY, 0.1f);
        camera.Position = m_CameraSettings.LookAt - camera.GetDirection() * offset;
    }

    void Sandbox::OnMousePressed(MouseButton button)
    {
        m_Mouse.Position = GetMousePostion();
        m_Mouse.Button[button] = true;
    }

    void Sandbox::OnMouseReleased(MouseButton button)
    {
        m_Mouse.Button[button] = false;
    }

    void Sandbox::OnMouseMoved(const glm::vec2& pos)
    {
        if (m_State == SandboxState::Idle && m_Mouse.Button[MouseButton::Left])
        {
            auto& camera = m_Scene.Camera;
            auto [w, h] = GetWindowSize();
            auto delta = (pos - m_Mouse.Position) / glm::vec2(w, h);

            const float distance = glm::length(m_CameraSettings.LookAt - camera.Position);

            auto [beta, alpha] = SphericalAngles(camera.GetDirection());

            beta += delta.y * m_CameraSettings.RotateSpeed;
            alpha -= delta.x * m_CameraSettings.RotateSpeed;

            camera.SetDirection({
                glm::sin(alpha) * glm::sin(beta),
                glm::cos(beta),
                glm::cos(alpha) * glm::sin(beta)
                });

            camera.Position = m_CameraSettings.LookAt - camera.GetDirection() * distance;
        }
        m_Mouse.Position = pos;
    }

    glm::vec2 Sandbox::GetMousePostion() const
    {
        double x, y;
        glfwGetCursorPos(m_Window, &x, &y);
        return { x, y };
    }

    void Sandbox::Initialize()
    {
        // Load scene definitions
        LoadSceneDefinitions();

        // Render texture
        glGenTextures(1, &m_RenderTexture);
        glBindTexture(GL_TEXTURE_2D, m_RenderTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        m_Font = io.Fonts->AddFontFromFileTTF("res/fonts/roboto.ttf", 24);

        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

    }

    void Sandbox::Update()
    {

        const auto [width, height] = GetWindowSize();

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glUseProgram(0);


        if(m_State == SandboxState::Idle)
        {
            if (m_GLRenderer)
                m_GLRenderer->Render(m_Scene.Camera.Position, m_Scene.Camera.GetDirection(), s_FovY, width / height);
        }
        else
        {
            if (m_RenderResult->IsInterrupted())
                m_State = SandboxState::Result;

            UpdateTexture();

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, m_RenderTexture);

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

    void sandbox::Sandbox::RenderGUI()
    {

        m_Toasts.erase(std::remove_if(m_Toasts.begin(), m_Toasts.end(), [](const Toast& t) {
            return t.EndTime < std::chrono::system_clock::now().time_since_epoch();
        }), m_Toasts.end());

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(m_Font);

        if (m_State == SandboxState::Idle)
        {
            ImGui::BeginMainMenuBar();
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::BeginMenu("Render"))
                {
                    for (uint32_t s : {64, 128, 256, 512})
                    {
                        std::stringstream ss;
                        ss << s << " px";

                        if (ImGui::MenuItem(ss.str().c_str()))
                        {
                            rt::ViewParameters params;
                            auto [vw, vh] = GetScaledWindowSize(s);

                            params.NumThreads = 8;
                            params.Width = vw;
                            params.Height = vh;
                            params.FovY = s_FovY;

                            m_CurrentIteration = 0;
                            m_RenderResult = m_Pathtracer.Run(params, m_Scene, 0);
                            m_RenderResult->OnIterationEnd.Subscribe(this, &Sandbox::OnIterationEndHandler);
                            m_State = SandboxState::Rendering;
                        }
                    }

                    if (ImGui::MenuItem("Full size"))
                    {
                        rt::ViewParameters params;
                        auto [vw, vh] = GetWindowSize();

                        params.NumThreads = 8;
                        params.Width = vw;
                        params.Height = vh;
                        params.FovY = s_FovY;

                        m_CurrentIteration = 0;
                        m_RenderResult = m_Pathtracer.Run(params, m_Scene, 0);
                        m_RenderResult->OnIterationEnd.Subscribe([&](const rt::Image& img, size_t iteration) {
                            std::lock_guard guard(m_ImageMutex);
                            m_Image = img;
                            m_TextureNeedsUpdate = true;
                            m_CurrentIteration = iteration + 1;
                        });
                        m_State = SandboxState::Rendering;
                    }
                    

                    ImGui::Separator();

                    if (ImGui::MenuItem("Debug - Normals"))
                    {
                        rt::ViewParameters params;
                        auto [vw, vh] = GetWindowSize();

                        params.NumThreads = 4;
                        params.Width = vw;
                        params.Height = vh;
                        params.FovY = s_FovY;
                        m_Debug.CurrentMode = rt::utility::DebugRaytracer::Mode::Normal;
                        m_RenderResult = m_Debug.Run(params, m_Scene, 0);
                        m_RenderResult->OnIterationEnd.Subscribe(this, &Sandbox::OnIterationEndHandler);
                        m_State = SandboxState::Rendering;
                    }

                    if (ImGui::MenuItem("Debug - Albedo"))
                    {
                        rt::ViewParameters params;
                        auto [vw, vh] = GetWindowSize();

                        params.NumThreads = 4;
                        params.Width = vw;
                        params.Height = vh;
                        params.FovY = s_FovY;
                        m_Debug.CurrentMode = rt::utility::DebugRaytracer::Mode::Albedo;
                        m_RenderResult = m_Debug.Run(params, m_Scene, 0);
                        m_RenderResult->OnIterationEnd.Subscribe(this, &Sandbox::OnIterationEndHandler);
                        m_State = SandboxState::Rendering;
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Load Scene"))
                {
                    for (const auto& file : m_SceneFiles)
                    {
                        if (ImGui::MenuItem(file.filename().string().c_str()))
                        {
                            m_Scene = rt::utility::LoadScene(file.string());
                            m_GLRenderer = std::make_unique<GLSceneRenderer>(m_Scene);
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Quit"))
                {
                    m_Running = false;
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();

            auto eye = m_Scene.Camera.Position;
            auto direction = m_Scene.Camera.GetDirection();

            ImGui::SetNextWindowPos({ 10.0f, 40.0f });
            ImGui::SetNextWindowSize({ 300.0f, -1.0f });
            ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoDecoration);
            ImGui::InputFloat3("Eye", glm::value_ptr(eye), "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat3("Direction", glm::value_ptr(direction), "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::End();

        }
        else if (m_State == SandboxState::Rendering)
        {
            const auto iteration = m_RenderResult->Iteration.load();
            const float elapsedTime = m_RenderResult->GetElapsedTime();

            const uint32_t iterationsPerSecond = iteration / elapsedTime;

            ImGui::SetNextWindowPos({ 10.0f, 10.0f });
            ImGui::SetNextWindowSize({ 300.0f, -1.0f });
            ImGui::Begin("Render", nullptr, ImGuiWindowFlags_NoDecoration);
            ImGui::Text("Elapsted Time: %.2f", m_RenderResult->GetElapsedTime());
            ImGui::Text("Iteration #%d, %d it/sec", iteration, iterationsPerSecond);
            ImGui::ProgressBar(m_RenderResult->Progress);
            if (ImGui::Button("Interrupt", { -1.0f, 0.0f })) {
                m_RenderResult->Interrupt();
            }
            ImGui::End();
        }
        else if (m_State == SandboxState::Result)
        {
            ImGui::SetNextWindowPos({ 10.0f, 10.0f });
            ImGui::SetNextWindowSize({ 300.0f, -1.0f });
            ImGui::Begin("Render", nullptr, ImGuiWindowFlags_NoDecoration);
            if (ImGui::Button("Save", { -1.0f, 0.0f }))
            {
                SaveImage();
            }
            if (ImGui::Button("Back", { -1.0f, 0.0f }))
            {
                m_RenderResult = nullptr;
                m_State = SandboxState::Idle;
            }
            ImGui::End();
        }

        const auto [width, height] = GetWindowSize();
        constexpr float toastWidth = 350.0f;
        constexpr float toastPadding = 10.0f;

        ImVec2 pos = { width - (toastWidth + toastPadding), toastPadding };
        size_t index = 0;
        for (const auto& toast : m_Toasts)
        {
            std::stringstream ss;
            ss << "Toast-" << index++;
            ImGui::SetNextWindowSize({ toastWidth, -1.0f });
            ImGui::SetNextWindowPos(pos);
            ImGui::Begin(ss.str().c_str(), nullptr, ImGuiWindowFlags_NoDecoration);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), toast.Title.c_str());
            ImGui::TextWrapped(toast.Message.c_str());
            pos.y += ImGui::GetWindowSize().y + toastPadding;
            ImGui::End();
        }

        ImGui::PopFont();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void sandbox::Sandbox::LoadSceneDefinitions()
    {
        namespace fs = std::filesystem;

        for (const auto& entry : fs::directory_iterator("res/scenes"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                m_SceneFiles.push_back(entry.path());
            }
        }

    }

    void Sandbox::UpdateTexture()
    {
        std::lock_guard guard(m_ImageMutex);
        
        if (m_TextureNeedsUpdate)
        {

            auto& image = m_Image;

            m_Pixels.resize(image.GetWidth() * image.GetHeight());

            for (size_t x = 0; x < image.GetWidth(); ++x) {
                for (size_t y = 0; y < image.GetHeight(); ++y) {
                    auto color = image.GetPixel(x, y);
                    
                    
                    // Tone mapping
                    color = glm::vec3(1.0f) - glm::exp(-color);

                    // Gamma correction
                    color = glm::pow(color, glm::vec3(1.0f / 2.2f));

                    m_Pixels[y * image.GetWidth() + x] =
                        ((uint32_t(color.r * 255) << 0)) |
                        ((uint32_t(color.g * 255) << 8)) |
                        ((uint32_t(color.b * 255) << 16)) |
                        ((uint32_t(255) << 24));

                }
            }

            glBindTexture(GL_TEXTURE_2D, m_RenderTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.GetWidth(), image.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_Pixels.data());
            m_TextureNeedsUpdate = false;
        }
    }

	void sandbox::Sandbox::SaveImage()
	{
        const size_t w = m_Image.GetWidth();
        const size_t h = m_Image.GetHeight();
        std::vector<uint32_t> data;
        auto now = std::chrono::system_clock::now();
        std::stringstream path;

        data.resize(w * h);
        glBindTexture(GL_TEXTURE_2D, m_RenderTexture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        
        path << "res/screenshots/" << std::hex << now.time_since_epoch().count() << ".png";
        stbi_write_png(path.str().c_str(), w, h, 4, data.data(), 0);

        m_Toasts.emplace_back("Screenshot Saved", path.str());

	}

    void sandbox::Sandbox::OnIterationEndHandler(const rt::Image& image, size_t iteration)
    {
        std::lock_guard guard(m_ImageMutex);
        m_Image = image;
        m_TextureNeedsUpdate = true;
        m_CurrentIteration = iteration + 1;
    }

    std::tuple<float, float> Sandbox::SphericalAngles(const glm::vec3& dir)
    {
        const float beta = glm::acos(glm::dot(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
        //float beta = glm::atan(glm::sqrt(dir.x * dir.x + dir.z * dir.z) / dir.y);
        float alpha = std::atan2(dir.x, dir.z);
        return { beta, alpha };
    }

}