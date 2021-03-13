#pragma once

#include <string_view>

#include <Scene.h>

namespace rt::utility
{
	Scene LoadScene(std::string_view fileName);
}