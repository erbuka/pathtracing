#include "Sandbox.h"


#include <chrono>
#include <fstream>
#include <filesystem>
#include <regex>
#include <sstream>

#include <spdlog\spdlog.h>
#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm\glm.hpp>
#include <glm\ext.hpp>
#include <glm\gtx\transform.hpp>

#include <MeshLoader.h>



#define RT_SANDBOX(window) ((Sandbox*)glfwGetWindowUserPointer(window))

namespace glm {
    void to_json(nlohmann::json& j, const glm::vec3& v) {
        j = nlohmann::json::array({ v.x, v.y, v.z });
    }

    void from_json(const nlohmann::json& j, glm::vec3& v) {
        v = { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
    }
}

namespace rt
{
    void to_json(nlohmann::json& j, const rt::SampleMode& s) {
        if (s == rt::SampleMode::Linear) j = "linear";
        else if (s == rt::SampleMode::Nearest) j = "nearest";
    }

    void from_json(const nlohmann::json& j, rt::SampleMode& s) {
        auto str = j.get<std::string>();
        if (str == "linear") s = rt::SampleMode::Linear;
        else if (str == "nearest") s = rt::SampleMode::Nearest;
    }
}

namespace sandbox
{

    static constexpr float s_FovY = glm::pi<float>() / 4.0f;


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
        m_Window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
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
        m_Camera.Distance = glm::max(m_Camera.Distance + m_Camera.MoveSpeed * offsetY, 0.1f);
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
            auto [w, h] = GetWindowSize();
            auto delta = (pos - m_Mouse.Position) / glm::vec2(w, h);
            m_Camera.Alpha += -delta.x * m_Camera.RotateSpeed;
            m_Camera.Beta += delta.y * m_Camera.RotateSpeed;
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
                m_GLRenderer->Render(m_Camera.GetPosition(), m_Camera.GetDirection(), s_FovY, width / height);
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
                        ss << "Pathtracing (" << s << " px)";

                        if (ImGui::MenuItem(ss.str().c_str()))
                        {
                            rt::ViewParameters params;
                            auto [vw, vh] = GetScaledWindowSize(s);

                            params.NumThreads = 4;
                            params.Width = vw;
                            params.Height = vh;
                            params.FovY = s_FovY;
                            params.Camera.Position = m_Camera.GetPosition();
                            params.Camera.Direction = m_Camera.GetDirection();

                            m_CurrentIteration = 0;
                            m_RenderResult = m_Pathtracer.Run(params, m_Scene, 0);
                            m_RenderResult->OnIterationEnd.Subscribe([&](const rt::Image& img, size_t iteration) {
                                std::lock_guard guard(m_ImageMutex);
                                m_TextureNeedsUpdate = true;
                                m_Image = img;
                                m_CurrentIteration = iteration + 1;
                            });
                            m_State = SandboxState::Rendering;
                        }
                    }

                    if (ImGui::MenuItem("Pathtracing (full size)"))
                    {
                        rt::ViewParameters params;
                        auto [vw, vh] = GetWindowSize();

                        params.NumThreads = 4;
                        params.Width = vw;
                        params.Height = vh;
                        params.FovY = s_FovY;
                        params.Camera.Position = m_Camera.GetPosition();
                        params.Camera.Direction = m_Camera.GetDirection();

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
                    
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Load Scene"))
                {
                    for (const auto& sceneDef : m_SceneDefs)
                    {
                        if (ImGui::MenuItem(sceneDef["name"].get<std::string>().c_str()))
                        {
                            LoadScene(sceneDef);
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
        }
        else if (m_State == SandboxState::Rendering)
        {
            const auto iteration = m_RenderResult->Iteration.load();
            ImGui::SetNextWindowPos({ 0.0f, 0.0f });
            //ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            ImGui::Begin("Test", nullptr);
            ImGui::Text("Rendering");
            ImGui::ProgressBar(m_RenderResult->Progress);
            ImGui::Text("Iteration #%d", iteration);
            if (ImGui::Button("Interrupt")) {
                m_RenderResult->Interrupt();
            }
            ImGui::End();
        }
        else if (m_State == SandboxState::Result)
        {
            ImGui::SetNextWindowPos({ 0.0f, 0.0f });
            ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            if (ImGui::Button("Back"))
            {
                m_RenderResult = nullptr;
                m_State = SandboxState::Idle;
            }
            ImGui::End();
        }


        ImGui::PopFont();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void sandbox::Sandbox::LoadSceneDefinitions()
    {
        using json = nlohmann::json;
        namespace fs = std::filesystem;

        for (const auto& entry : fs::directory_iterator("res/scenes"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                std::ifstream is(entry);
                    
                if (!is.is_open())
                {
                    spdlog::error("Can't open file: {0}", entry.path().string());
                    continue;
                }
                
                json sceneDef;
                is >> sceneDef;
                is.close();

                m_SceneDefs.push_back(sceneDef);

            }
        }

    }

    void sandbox::Sandbox::LoadScene(const nlohmann::json& sceneDef)
    {
        m_Scene = rt::Scene();
            
        std::map<std::string, std::shared_ptr<rt::Mesh>> meshes;
        std::map<std::string, std::shared_ptr<rt::Sampler2D>> samplers2D;
        std::map<std::string, std::shared_ptr<rt::Sampler3D>> samplers3D;

        if (sceneDef.contains("meshes"))
        {
            for (const auto& meshDef : sceneDef["meshes"])
            {
                const auto ids = meshDef["ids"].get<std::vector<std::string>>();
                for (auto [name, mesh] : rt::utility::LoadMeshesFromWavefront(meshDef["file"].get<std::string>()))
                {
                    if (std::find(ids.begin(), ids.end(), name) != ids.end())
                    {
                        meshes[name] = std::move(mesh);
                    }
                }

            }
        }

        if (sceneDef.contains("samplers"))
        {
            for (const auto& samplerDef : sceneDef["samplers"])
            {
                if (samplerDef.contains("file"))
                {
                    const auto id = samplerDef["id"].get<std::string>();
                    auto image = std::make_shared<rt::Image>();
                    image->Load(samplerDef["file"].get<std::string>());

                    if (samplerDef.contains("mode"))
                        samplerDef["mode"].get_to(image->SampleMode);

                    std::string type = samplerDef.contains("type") ? samplerDef["type"].get<std::string>() : "image";

                    if (type == "image")
                    {
                        samplers2D[id] = std::move(image);
                    }
                    else if (type == "equirectangular")
                    {
                        samplers3D[id] = std::make_shared<rt::EquirectangularMap>(image);
                    }
                    else
                    {
                        spdlog::error("Unkwnown sampler type: {0}", type);
                    }
                }
                else if (samplerDef.contains("color"))
                {
                    auto id = samplerDef["id"].get<std::string>();
                    auto sampler = std::make_shared<rt::ColorSampler>(samplerDef["color"].get<glm::vec3>());


                    samplers2D[id] = sampler;
                    samplers3D[id] = sampler;
                }
            }
        }

        if (sceneDef.contains("background"))
        {
            auto background = sceneDef["background"];

            if (background.contains("color"))
                m_Scene.Background = samplers3D.at(background["color"].get<std::string>());

        }

        if (sceneDef.contains("nodes"))
        {
            for (const auto& nodeDef : sceneDef["nodes"])
            {
                auto node = std::make_shared<rt::SceneNode>();

                if (nodeDef.contains("translate"))
                    node->Translate(nodeDef["translate"].get<glm::vec3>());

                if (nodeDef.contains("rotate"))
                {
                    glm::vec3 angles = nodeDef["rotate"].get<glm::vec3>();
                    node->Transform *=
                        glm::rotate(glm::radians(angles.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
                        glm::rotate(glm::radians(angles.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                        glm::rotate(glm::radians(angles.x), glm::vec3(1.0f, 0.0f, 0.0f));
                }

                if (nodeDef.contains("scale"))
                    node->Scale(nodeDef["scale"].get<glm::vec3>());

                if (nodeDef.contains("mesh"))
                    node->Shape = meshes.at(nodeDef["mesh"].get<std::string>());

                else if (nodeDef.contains("shape") && nodeDef["shape"].get<std::string>() == "sphere")
                    node->Shape = std::make_shared<rt::Sphere>();

                if (nodeDef.contains("material"))
                {
                    const auto matDef = nodeDef["material"];

                    if (matDef.contains("albedo"))
                        node->Material.Albedo = samplers2D.at(matDef["albedo"].get<std::string>());

                    if (matDef.contains("emission"))
                        node->Material.Emission = samplers2D.at(matDef["emission"].get<std::string>());

                    if (matDef.contains("roughness"))
                        node->Material.Roughness = samplers2D.at(matDef["roughness"].get<std::string>());

                    if (matDef.contains("metallic"))
                        node->Material.Metallic = samplers2D.at(matDef["metallic"].get<std::string>());

                }

                m_Scene.Nodes.push_back(std::move(node));

            }
        }

        m_GLRenderer = std::make_unique<GLSceneRenderer>(m_Scene);

    }

    void Sandbox::UpdateTexture()
    {
        if (m_TextureNeedsUpdate)
        {
            std::lock_guard guard(m_ImageMutex);

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

    glm::vec3 Sandbox::Camera::GetDirection() const
    {
        return glm::normalize(glm::vec3(
            std::cos(Beta) * std::cos(Alpha),
            std::sin(Beta),
            std::cos(Beta) * std::sin(Alpha)
            ));
    }

    glm::vec3 sandbox::Sandbox::Camera::GetPosition() const
    {
        return LookAt - GetDirection() * Distance;
    }
}