#pragma once

#include <string_view>

#include <Scene.h>

namespace rt::utility
{
	/// <summary>
	/// Load a scene from file
	/// </summary>
	/// <param name="fileName">A JSON file</param>
	/// <returns>The scene</returns>
	Scene LoadScene(std::string_view fileName);
}