#pragma once

#include <string_view>

#include <scene.h>

namespace rt::utility
{
	/// <summary>
	/// Load a scene from file
	/// </summary>
	/// <param name="file_name">A JSON file</param>
	/// <returns>The scene</returns>
	scene load_scene(std::string_view file_name);
}