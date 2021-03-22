#include "mesh_loader.h"

#include <fstream>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <regex>

#include "scene.h"

namespace rt::utility
{
    std::map<std::string, std::shared_ptr<rt::mesh>> load_meshes_from_wavefront(std::string_view file_name)
    {
        std::map<std::string, std::shared_ptr<rt::mesh>> result;

        std::ifstream is;
        is.open(file_name.data());

        if (!is.is_open())
        {
            spdlog::error("Can't open file: {0}", file_name.data());
            return result;
        }

        std::string line;
        std::string current_mesh_name = "default";
        auto current_mesh = std::make_shared<rt::mesh>();
        std::vector<glm::vec3> vertices, normals;
        std::vector<glm::vec2> uvs;

        const auto check_new_mesh = [&current_mesh, &result, &current_mesh_name] {
            if (current_mesh->get_triangles().size() > 0) {
                current_mesh->compile();
                result[current_mesh_name] = std::move(current_mesh);
                current_mesh = std::make_shared<rt::mesh>();
            }
        };


        const std::regex r_comment("#\\s+(.+)");
        const std::regex r_vertex("v\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)");
        const std::regex r_normal("vn\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)");
        const std::regex r_uv("vt\\s+(-?[\\d\.]+)\\s+(-?[\\d\.]+)");
        const std::regex r_object("[og]\\s+(.+)");
        const std::regex r_face0("f\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)");
        const std::regex r_face1(
            "f\\s+"
            "([0-9]+)//([0-9]+)\\s+"
            "([0-9]+)//([0-9]+)\\s+"
            "([0-9]+)//([0-9]+)");
        const std::regex r_face2(
            "f\\s+"
            "([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)");
        const std::regex r_face3(
            "f\\s+"
            "([0-9]+)/([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)/([0-9]+)\\s+"
            "([0-9]+)/([0-9]+)/([0-9]+)");

        while (is.good())
        {
            std::getline(is, line);
            std::smatch matches;

            if (std::regex_search(line, matches, r_vertex))
            {
                vertices.push_back({
                    std::stof(matches.str(1)),
                    std::stof(matches.str(2)),
                    std::stof(matches.str(3))
                    });
            }
            else if (std::regex_search(line, matches, r_normal))
            {
                normals.push_back({
                    std::stof(matches.str(1)),
                    std::stof(matches.str(2)),
                    std::stof(matches.str(3))
                    });
            }
            else if (std::regex_search(line, matches, r_uv))
            {
                uvs.push_back({
                    std::stof(matches.str(1)),
                    std::stof(matches.str(2)),
                    });
            }
            else if (std::regex_search(line, matches, r_face0))
            {
                auto& t = current_mesh->add_triangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.vertices[i].position = vertices[std::stoi(matches.str(1 + i)) - 1];
                }

            }
            else if (std::regex_search(line, matches, r_face1))
            {
                auto& t = current_mesh->add_triangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.vertices[i].position = vertices[std::stoi(matches.str(1 + i * 2)) - 1];
                    t.vertices[i].normal = normals[std::stoi(matches.str(2 + i * 2)) - 1];
                }

            }
            else if (std::regex_search(line, matches, r_face2))
            {
                auto& t = current_mesh->add_triangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.vertices[i].position = vertices[std::stoi(matches.str(1 + i * 2)) - 1];
                    t.vertices[i].uv = uvs[std::stoi(matches.str(2 + i * 2)) - 1];
                }

            }
            else if (std::regex_search(line, matches, r_face3))
            {
                auto& t = current_mesh->add_triangle();

                for (size_t i = 0; i < 3; ++i)
                {
                    t.vertices[i].position = vertices[std::stoi(matches.str(1 + i * 3)) - 1];
                    t.vertices[i].uv = uvs[std::stoi(matches.str(2 + i * 3)) - 1];
                    t.vertices[i].normal = normals[std::stoi(matches.str(3 + i * 3)) - 1];
                }


            }
            else if (std::regex_search(line, matches, r_object))
            {
                check_new_mesh();
                current_mesh_name = matches.str(1);
            }
            else if (std::regex_search(line, matches, r_comment))
            {
                spdlog::info("Comment: {0}", matches.str(1));
            }
            else
            {
                spdlog::warn("Unable to parse: {0}", line);
            }


        }

        check_new_mesh();

        is.close();

        return result;

    }

}