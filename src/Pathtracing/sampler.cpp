#include "sampler.h"

#include <numeric>

#include <spdlog/spdlog.h>

#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace rt {
	image::image() : image(0, 0)
	{
	}
	image::image(size_t width, size_t height) :
		m_width(width),
		m_height(height)
	{
		m_pixels.resize(width * height);
	}
	void image::resize(size_t width, size_t height)
	{
		m_width = width;
		m_height = height;
		m_pixels.resize(width * height);
	}
	glm::vec3 image::average() const
	{
		glm::vec3 avg(0.0f);
		for (size_t x = 0; x < m_width; ++x)
			for (size_t y = 0; y < m_height; ++y)
				avg += get_pixel({ x, y });
		return avg / float(m_width * m_height);
	}
	glm::vec3 image::sample(const glm::vec2& uv) const
	{
		const auto uv0 = glm::fract(uv);

		const float x = uv0.x * m_width;
		const float y = uv0.y * m_height;

		if (sample_mode == sample_mode::linear)
		{
			const uint32_t x0 = uint32_t(glm::floor(x)) % m_width;
			const uint32_t x1 = uint32_t(glm::ceil(x)) % m_width;
			const uint32_t y0 = uint32_t(glm::floor(y)) % m_height;
			const uint32_t y1 = uint32_t(glm::ceil(y)) % m_height;

			const auto v0 = glm::mix(get_pixel(x0, y0), get_pixel(x1, y0), x - x0);
			const auto v1 = glm::mix(get_pixel(x0, y1), get_pixel(x1, y1), x - x0);
			return glm::mix(v0, v1, y - y0);
		}
		else
		{
			const uint32_t ix = uint32_t(glm::round(x)) % m_width;
			const uint32_t iy = uint32_t(glm::round(y)) % m_height;
			return get_pixel(ix, iy);
		}

	}
	void image::set_pixel(size_t x, size_t y, const glm::vec3& color)
	{
		m_pixels[y * m_width + x] = color;
	}
	
	glm::vec3 image::get_pixel(size_t x, size_t y) const
	{
		return m_pixels[y * m_width + x];
	}

	glm::vec3 image::get_pixel(const glm::uvec2 xy) const
	{
		return get_pixel(xy.x, xy.y);
	}


	void image::load(std::string_view fileName)
	{
		int32_t w, h, channels;
		stbi_set_flip_vertically_on_load_thread(true);
		float * data = stbi_loadf(fileName.data(), &w, &h, &channels, 3);

		if (data)
		{
			m_width = w;
			m_height = h;
			m_pixels.resize(size_t(w) * h);
			std::memcpy(m_pixels.data(), data, sizeof(float) * w * h * 3);
			stbi_image_free(data);
		}
		else
		{
			spdlog::error("Can't load image: {0}", fileName);
		}
	}
	
	void image::to_ldr()
	{
		float max = 0.0f;

		for (const auto& p : m_pixels)
			max = std::max({ max, p.x, p.y, p.z });

		if (max > 1.0f)
		{
			std::for_each(m_pixels.begin(), m_pixels.end(), [](auto& p) { 
				p = glm::vec3(1.0f) - glm::exp(-p);
			});

		}

	}


	glm::vec3 equirectangular_map::sample(const glm::vec3& uvw) const
	{
		const auto normal = glm::normalize(uvw);
		const glm::vec2 uv = {
			std::atan2(normal.x, normal.z) / (2.0f * glm::pi<float>()) + 0.5f,
			std::asin(normal.y) / glm::pi<float>() - 0.5f
		};
		return m_image->sample(uv);
	}
}
