#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace rt
{
	class Mesh;

	namespace utility
	{
		std::map<std::string, std::shared_ptr<rt::Mesh>> LoadMeshesFromWavefront(std::string_view fileName);
	}
}