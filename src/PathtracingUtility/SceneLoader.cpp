#include "SceneLoader.h"

#include "MeshLoader.h"

#include <fstream>
#include <json.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <Sampler.h>
#include <spdlog/spdlog.h>

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


namespace rt::utility
{
	Scene LoadScene(std::string_view fileName)
	{
        using json = nlohmann::json;
        
        Scene result;
        json sceneDef;
        std::ifstream is(fileName.data());

        if (!is.good())
        {
            spdlog::error("Can't open file: {0}", fileName.data());
            return result;
        }

        is >> sceneDef;


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

        if (sceneDef.contains("camera"))
        {
            auto camera = sceneDef["camera"];

            if (camera.contains("position"))
                camera["position"].get_to(result.Camera.Position);


            if (camera.contains("direction"))
            {
                result.Camera.SetDirection(camera["direction"].get<glm::vec3>());
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

                    if (samplerDef.contains("ldr") && samplerDef["ldr"].get<bool>())
                        image->ToLDR();

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
                result.Background = samplers3D.at(background["color"].get<std::string>());

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
                    node->Multiply(
                        glm::rotate(glm::radians(angles.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
                        glm::rotate(glm::radians(angles.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                        glm::rotate(glm::radians(angles.x), glm::vec3(1.0f, 0.0f, 0.0f)));
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

                result.Nodes.push_back(std::move(node));

            }
        }


        return result;
	}
}