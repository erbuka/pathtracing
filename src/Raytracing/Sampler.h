#pragma once

#include <glm\glm.hpp>

#include <string_view>
#include <cinttypes>
#include <vector>
#include <memory>

#include "Common.h"

namespace rt {

	class Sampler2D
	{
	public:
		virtual glm::vec3 Sample(const glm::vec2& uv) const = 0;
	};

	class Sampler3D
	{
	public:
		virtual glm::vec3 Sample(const glm::vec3& uvw) const = 0;
	};

	class ColorSampler : public Sampler2D, public Sampler3D
	{
	public:
		ColorSampler(const glm::vec3& color) : m_Color(color) {}
		glm::vec3 Sample(const glm::vec2& uv) const override { return m_Color; }
		glm::vec3 Sample(const glm::vec3& uvw) const override { return m_Color; }
	private:
		glm::vec3 m_Color;
	};

	enum class SampleMode
	{
		Linear, Nearest
	};

	class Image : public Sampler2D 
	{
	public:

		SampleMode SampleMode = SampleMode::Linear;

		Image();
		Image(size_t width, size_t height);

		void Resize(size_t width, size_t height);

		glm::vec3 Sample(const glm::vec2& uv) const override;

		void SetPixel(size_t x, size_t y, const glm::vec3& color);
		glm::vec3 GetPixel(size_t x, size_t y) const;
		glm::vec3 GetPixel(const glm::uvec2 xy) const;

		size_t GetWidth() const { return m_Width; }
		size_t GetHeight() const { return m_Height; }

		void Load(std::string_view fileName);

	private:
		uint32_t m_Width, m_Height;
		std::vector<glm::vec3> m_Pixels;
	};

	class EquirectangularMap : public Sampler3D
	{
	public:
		EquirectangularMap(const std::shared_ptr<Image>& image) : m_Image(image) {}
		glm::vec3 Sample(const glm::vec3& uvw) const override;
	private:
		std::shared_ptr<Image> m_Image;
	};

}