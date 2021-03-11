#include "MeshLoader.h"

#include <fstream>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <regex>

#include "Scene.h"

namespace rt::utility
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

}