#include "Sandbox.h"

#include <chrono>
#include <fstream>
#include <filesystem>
#include <regex>

#include <spdlog\spdlog.h>
#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm\glm.hpp>
#include <glm\ext.hpp>
#include <glm\gtx\transform.hpp>


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

    void to_json(nlohmann::json& j, const rt::LightType& t) {
        if (t == rt::LightType::Directional) j = "directional";
        else if (t == rt::LightType::Point) j = "point";
        else if (t == rt::LightType::Spot) j = "spot";
    }

    void from_json(const nlohmann::json& j, rt::LightType& t) {
        auto str = j.get<std::string>();
        if (str == "directional") t = rt::LightType::Directional;
        else if (str == "point") t = rt::LightType::Point;
        else if (str == "spot") t = rt::LightType::Spot;
    }
}

namespace sandbox
{

    std::map<std::string, std::shared_ptr<rt::Mesh>> LoadMeshesFromWavefront(std::string_view fileName)
    {
        std::map<std::string, std::shared_ptr<rt::Mesh>> result;

        std::ifstream is;
        is.open(fileName.data());

        if (!is.is_open())
        {
            spdlog::error("Can't open file: {0}", fileName.data());
            return result;
        }

        std::string line;
        std::string currentMeshName = "default";
        auto currentMesh = std::make_shared<rt::Mesh>();
        std::vector<glm::vec3> vertices, normals;
        std::vector<glm::vec2> uvs;

        const auto checkNewMesh = [&currentMesh, &result, &currentMeshName] {
            if (currentMesh->GetTriangles().size() > 0) {
                currentMesh->Compile();
                result[currentMeshName] = std::move(currentMesh);
                currentMesh = std::make_shared<rt::Mesh>();
            }
        };

        const std::regex rComment("#\\s+(.+)");
        const std::regex rVertex("v\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)");
        const std::regex rNormal("vn\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)");
        const std::regex rUv("vt\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)");
        const std::regex rObject("o\\s+(.+)");
        const std::regex rFace0("f\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)");
        const std::regex rFace1(
            "f\\s+"
            "([0-9]+)//([0-9]+)\\s+"
            "([0-9]+)//([0-9]+)\\s+"
            "([0-9]+)//([0-9]+)");
        const std::regex rFace2(
            "f\\s+"
            "([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)");
        const std::regex rFace3(
            "f\\s+"
            "([0-9]+)/([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)/([0-9]+)");

        while (is.good())
        {
            std::getline(is, line);
            std::smatch matches;

            if (std::regex_search(line, matches, rVertex))
            {
                vertices.push_back({
                    std::stof(matches.str(1)),
                    std::stof(matches.str(2)),
                    std::stof(matches.str(3))
                    });
            }
            else if (std::regex_search(line, matches, rNormal))
            {
                normals.push_back({
                    std::stof(matches.str(1)),
                    std::stof(matches.str(2)),
                    std::stof(matches.str(3))
                    });
            }
            else if (std::regex_search(line, matches, rUv))
            {
                uvs.push_back({
                    std::stof(matches.str(1)),
                    std::stof(matches.str(2)),
                    });
            }
            else if (std::regex_search(line, matches, rFace0))
            {
                auto& t = currentMesh->AddTriangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.Vertices[i].Position = vertices[std::stoi(matches.str(1 + i)) - 1];
                }

            }
            else if (std::regex_search(line, matches, rFace1))
            {
                auto& t = currentMesh->AddTriangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.Vertices[i].Position = vertices[std::stoi(matches.str(1 + i * 2)) - 1];
                    t.Vertices[i].Normal = normals[std::stoi(matches.str(2 + i * 2)) - 1];
                }

            }
            else if (std::regex_search(line, matches, rFace2))
            {
                auto& t = currentMesh->AddTriangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.Vertices[i].Position = vertices[std::stoi(matches.str(1 + i * 2)) - 1];
                    t.Vertices[i].UV = uvs[std::stoi(matches.str(2 + i * 2)) - 1];
                }

            }
            else if (std::regex_search(line, matches, rFace3))
            {
                auto& t = currentMesh->AddTriangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.Vertices[i].Position = vertices[std::stoi(matches.str(1 + i * 3)) - 1];
                    t.Vertices[i].UV = uvs[std::stoi(matches.str(2 + i * 3)) - 1];
                    t.Vertices[i].Normal = normals[std::stoi(matches.str(3 + i * 3)) - 1];
                }


            }
            else if (std::regex_search(line, matches, rObject))
            {
                checkNewMesh();
                currentMeshName = matches.str(1);
            }
            else if (std::regex_search(line, matches, rComment))
            {
                spdlog::info("Comment: {0}", matches.str(1));
            }
            else
            {
                spdlog::warn("Unable to parse: {0}", line);
            }


        }

        checkNewMesh();

        is.close();

        return result;

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
        m_Window = glfwCreateWindow(1280, 720, "Hello World", NULL, NULL);
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

    rt::ViewParameters sandbox::Sandbox::GetDebugiViewParameters() const
    {
        rt::ViewParameters params;

        const auto [width, height] = GetWindowSize();

        params.Width = width / 4;
        params.Height = height / 4;
        params.NumThreads = 7;
        params.Camera.Position = m_Camera.GetPosition();
        params.Camera.Direction = m_Camera.GetDirection();

        return params;
    }

    rt::ViewParameters sandbox::Sandbox::GetViewParameters() const
    {
        rt::ViewParameters params;

        const auto [width, height] = GetWindowSize();

        params.Width = width;
        params.Height = height;
        params.NumThreads = 7;
        params.Camera.Position = m_Camera.GetPosition();
        params.Camera.Direction = m_Camera.GetDirection();

        return params;
    }

    void Sandbox::Initialize()
    {

        // Load scene definitions
        LoadSceneDefinitions();

        // Render texture
        glGenTextures(1, &m_RenderTexture);
        glBindTexture(GL_TEXTURE_2D, m_RenderTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


        // Debug texture
        glGenTextures(1, &m_DebugTexture);
        glBindTexture(GL_TEXTURE_2D, m_DebugTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // ImGui

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        m_Font = io.Fonts->AddFontFromFileTTF("res/fonts/roboto.ttf", 24);

        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(2.0f);
        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

    }

    void Sandbox::Update()
    {

        const auto [width, height] = GetWindowSize();

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_State == SandboxState::Idle || m_State == SandboxState::Result)
        {
            if (m_State == SandboxState::Idle)
            {
                m_Debug.CurrentMode = rt::DebugRaytracer::Mode::Color;
                auto result = m_Debug.Run(GetDebugiViewParameters(), m_Scene);
                result->Wait();
                ImageToTexture(result->GetResult(), m_DebugTexture);
            }


            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, m_State == SandboxState::Idle ? m_DebugTexture : m_RenderTexture);

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
        else if (m_State == SandboxState::Rendering)
        {
            if (m_RenderResult->IsReady())
            {
                ImageToTexture(m_RenderResult->GetResult(), m_RenderTexture);
                m_State = SandboxState::Result;
                m_RenderResult = nullptr;
            }
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
                if (ImGui::MenuItem("Render"))
                {
                    m_RenderResult = m_Raytracer.Run(GetViewParameters(), m_Scene);
                    m_State = SandboxState::Rendering;
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
            ImGui::SetNextWindowPos({ 0.0f, 0.0f });
            ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            ImGui::Text("Rendering");
            ImGui::ProgressBar(m_RenderResult->GetProgressPercent());
            ImGui::End();
        }
        else if (m_State == SandboxState::Result)
        {
            ImGui::SetNextWindowPos({ 0.0f, 0.0f });
            ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            if (ImGui::Button("Back"))
            {
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
                for (auto [name, mesh] : LoadMeshesFromWavefront(meshDef["file"].get<std::string>()))
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

        if (sceneDef.contains("lights"))
        {
            for (const auto& lightDef : sceneDef["lights"])
            {
                rt::Light light;

                if (lightDef.contains("type"))
                    lightDef["type"].get_to(light.Type);

                if (lightDef.contains("direction"))
                {
                    lightDef["direction"].get_to(light.Direction);
                    light.Direction = glm::normalize(light.Direction);
                }


                if (lightDef.contains("position"))
                    lightDef["position"].get_to(light.Position);

                if (lightDef.contains("color"))
                    lightDef["color"].get_to(light.Color);


                if (lightDef.contains("angle"))
                {
                    lightDef["angle"].get_to(light.Angle);
                    light.Angle = glm::radians(light.Angle);
                }

                m_Scene.Lights.push_back(std::move(light));
            }
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
                        glm::rotate(glm::radians(angles.x), glm::vec3(1.0f, 0.0f, 1.0f));
                }


                if (nodeDef.contains("scale"))
                    node->Scale(nodeDef["scale"].get<glm::vec3>());

                if (nodeDef.contains("mesh"))
                    node->Shape = meshes.at(nodeDef["mesh"].get<std::string>());

                else if (nodeDef.contains("shape") && nodeDef["shape"].get<std::string>() == "sphere")
                    node->Shape = std::make_shared<rt::Sphere>();

                if (nodeDef.contains("material"))
                {
                    auto matDef = nodeDef["material"];

                    if (matDef.contains("albedo"))
                        node->Material.Albedo = samplers2D.at(matDef["albedo"].get<std::string>());


                    if (matDef.contains("specular"))
                        node->Material.Specular = samplers2D.at(matDef["specular"].get<std::string>());
                }

                m_Scene.Nodes.push_back(std::move(node));

            }
        }


    }

    void Sandbox::ImageToTexture(const rt::Image& image, uint32_t texId)
    {
        m_Pixels.resize(image.GetWidth() * image.GetHeight());

        for (size_t x = 0; x < image.GetWidth(); ++x) {
            for (size_t y = 0; y < image.GetHeight(); ++y) {
                auto color = image.GetPixel(x, y);

                // Tone mapping
                // TODO maybe control exposure somewhere or not
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

        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.GetWidth(), image.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_Pixels.data());

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