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
		/// <summary>
		/// Loads meshes from a Wavefront file
		/// </summary>
		/// <param name="fileName">The file</param>
		/// <returns>A map of the meshes found in the given file</returns>
		std::map<std::string, std::shared_ptr<rt::Mesh>> LoadMeshesFromWavefront(std::string_view fileName);
	}
}