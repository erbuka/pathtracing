#include "scene_loader.h"

#include "mesh_loader.h"

#include <fstream>
#include <json.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <sampler.h>
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
    void to_json(nlohmann::json& j, const rt::sample_mode& s) {
        if (s == rt::sample_mode::linear) j = "linear";
        else if (s == rt::sample_mode::nearest) j = "nearest";
    }

    void from_json(const nlohmann::json& j, rt::sample_mode& s) {
        auto str = j.get<std::string>();
        if (str == "linear") s = rt::sample_mode::linear;
        else if (str == "nearest") s = rt::sample_mode::nearest;
    }
}


namespace rt::utility
{
	scene load_scene(std::string_view file_name)
	{
        using json = nlohmann::json;
        
        scene result;
        json scene_def;
        std::ifstream is(file_name.data());

        if (!is.good())
        {
            spdlog::error("Can't open file: {0}", file_name.data());
            return result;
        }

        is >> scene_def;


        std::map<std::string, std::shared_ptr<rt::mesh>> meshes;
        std::map<std::string, std::shared_ptr<rt::sampler_2d>> samplers_2d;
        std::map<std::string, std::shared_ptr<rt::sampler_3d>> samplers_3d;

        if (scene_def.contains("meshes"))
        {
            for (const auto& mesh_def : scene_def["meshes"])
            {
                const auto ids = mesh_def["ids"].get<std::vector<std::string>>();
                for (auto [name, mesh] : load_meshes_from_wavefront(mesh_def["file"].get<std::string>()))
                {
                    if (std::find(ids.begin(), ids.end(), name) != ids.end())
                    {
                        meshes[name] = std::move(mesh);
                    }
                }

            }
        }

        if (scene_def.contains("camera"))
        {
            auto camera = scene_def["camera"];

            if (camera.contains("position"))
                camera["position"].get_to(result.camera.position);


            if (camera.contains("direction"))
            {
                result.camera.set_direction(camera["direction"].get<glm::vec3>());
            }

        }

        if (scene_def.contains("samplers"))
        {
            for (const auto& sampler_def : scene_def["samplers"])
            {
                if (sampler_def.contains("file"))
                {
                    const auto id = sampler_def["id"].get<std::string>();
                    auto image = std::make_shared<rt::image>();
                    image->load(sampler_def["file"].get<std::string>());

                    if (sampler_def.contains("ldr") && sampler_def["ldr"].get<bool>())
                        image->to_ldr();

                    if (sampler_def.contains("mode"))
                        sampler_def["mode"].get_to(image->sample_mode);

                    std::string type = sampler_def.contains("type") ? sampler_def["type"].get<std::string>() : "image";

                    if (type == "image")
                    {
                        samplers_2d[id] = std::move(image);
                    }
                    else if (type == "equirectangular")
                    {
                        samplers_3d[id] = std::make_shared<rt::equirectangular_map>(image);
                    }
                    else
                    {
                        spdlog::error("Unkwnown sampler type: {0}", type);
                    }
                }
                else if (sampler_def.contains("color"))
                {
                    auto id = sampler_def["id"].get<std::string>();
                    auto sampler = std::make_shared<rt::color_sampler>(sampler_def["color"].get<glm::vec3>());


                    samplers_2d[id] = sampler;
                    samplers_3d[id] = sampler;
                }
            }
        }

        if (scene_def.contains("background"))
        {
            auto background = scene_def["background"];

            if (background.contains("color"))
                result.background = samplers_3d.at(background["color"].get<std::string>());

        }

        if (scene_def.contains("nodes"))
        {
            for (const auto& node_def : scene_def["nodes"])
            {
                auto node = std::make_shared<rt::scene_node>();

                if (node_def.contains("translate"))
                    node->translate(node_def["translate"].get<glm::vec3>());

                if (node_def.contains("rotate"))
                {
                    glm::vec3 angles = node_def["rotate"].get<glm::vec3>();
                    node->multiply(
                        glm::rotate(glm::radians(angles.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
                        glm::rotate(glm::radians(angles.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                        glm::rotate(glm::radians(angles.x), glm::vec3(1.0f, 0.0f, 0.0f)));
                }

                if (node_def.contains("scale"))
                    node->scale(node_def["scale"].get<glm::vec3>());

                if (node_def.contains("mesh"))
                    node->shape = meshes.at(node_def["mesh"].get<std::string>());

                else if (node_def.contains("shape") && node_def["shape"].get<std::string>() == "sphere")
                    node->shape = std::make_shared<rt::sphere>();

                if (node_def.contains("material"))
                {
                    const auto mat_def = node_def["material"];

                    if (mat_def.contains("albedo"))
                        node->material.albedo = samplers_2d.at(mat_def["albedo"].get<std::string>());

                    if (mat_def.contains("emission"))
                        node->material.emission = samplers_2d.at(mat_def["emission"].get<std::string>());

                    if (mat_def.contains("roughness"))
                        node->material.roughness = samplers_2d.at(mat_def["roughness"].get<std::string>());

                    if (mat_def.contains("metallic"))
                        node->material.metallic = samplers_2d.at(mat_def["metallic"].get<std::string>());

                }

                result.nodes.push_back(std::move(node));

            }
        }


        return result;
	}
}