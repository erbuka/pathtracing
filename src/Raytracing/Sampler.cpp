#include "Sampler.h"

#include <spdlog\spdlog.h>

#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace rt {
	Image::Image() : Image(0, 0)
	{
	}
	Image::Image(size_t width, size_t height) :
		m_Width(width),
		m_Height(height)
	{
		m_Pixels.resize(width * height);
	}
	void Image::Resize(size_t width, size_t height)
	{
		m_Width = width;
		m_Height = height;
		m_Pixels.resize(width * height);
	}
	glm::vec3 Image::Sample(const glm::vec2& uv) const
	{
		const auto uv0 = glm::fract(uv);

		const float x = uv0.x * m_Width;
		const float y = uv0.y * m_Height;

		if (SampleMode == SampleMode::Linear)
		{
			const uint32_t x0 = uint32_t(glm::floor(x)) % m_Width;
			const uint32_t x1 = uint32_t(glm::ceil(x)) % m_Width;
			const uint32_t y0 = uint32_t(glm::floor(y)) % m_Height;
			const uint32_t y1 = uint32_t(glm::ceil(y)) % m_Height;

			const auto v0 = glm::mix(GetPixel(x0, y0), GetPixel(x1, y0), x - x0);
			const auto v1 = glm::mix(GetPixel(x0, y1), GetPixel(x1, y1), x - x0);
			return glm::mix(v0, v1, y - y0);
		}
		else
		{
			uint32_t ix = uint32_t(glm::round(x)) % m_Width;
			uint32_t iy = uint32_t(glm::round(y)) % m_Height;
			return GetPixel(ix, iy);
		}
		

	}
	void Image::SetPixel(size_t x, size_t y, const glm::vec3& color)
	{
		m_Pixels[y * m_Width + x] = color;
	}
	
	glm::vec3 Image::GetPixel(size_t x, size_t y) const
	{
		return m_Pixels[y * m_Width + x];
	}

	glm::vec3 Image::GetPixel(const glm::uvec2 xy) const
	{
		return GetPixel(xy.x, xy.y);
	}


	void Image::Load(std::string_view fileName)
	{
		int32_t w, h, channels;
		stbi_set_flip_vertically_on_load_thread(true);
		float * data = stbi_loadf(fileName.data(), &w, &h, &channels, 3);

		if (data)
		{
			m_Width = w;
			m_Height = h;
			m_Pixels.resize(size_t(w) * h);
			std::memcpy(m_Pixels.data(), data, sizeof(float) * w * h * 3);
			stbi_image_free(data);
		}
		else
		{
			spdlog::error("Can't load image: {0}", fileName);
		}
	}
	glm::vec3 EquirectangularMap::Sample(const glm::vec3& uvw) const
	{
		const auto normal = glm::normalize(uvw);
		const glm::vec2 uv = {
			std::atan2(normal.x, normal.z) / glm::pi<float>() + 0.5f,
			normal.y * 0.5f + 0.5f
		};
		return m_Image->Sample(uv);
	}
}
