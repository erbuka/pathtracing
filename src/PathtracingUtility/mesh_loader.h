#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace rt
{
	class mesh;

	namespace utility
	{
		/// <summary>
		/// Loads meshes from a Wavefront file
		/// </summary>
		/// <param name="file_name">The file</param>
		/// <returns>A map of the meshes found in the given file</returns>
		std::map<std::string, std::shared_ptr<rt::mesh>> load_meshes_from_wavefront(std::string_view file_name);
	}
}