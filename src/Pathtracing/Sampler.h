#pragma once

#include <glm/glm.hpp>

#include <string_view>
#include <cinttypes>
#include <vector>
#include <memory>

#include "Common.h"

namespace rt {

	/// <summary>
	/// A 2D Sampler
	/// </summary>
	class Sampler2D
	{
	public:
		/// <summary>
		/// Samples at the given uv
		/// </summary>
		/// <param name="uv">Uv coordinates</param>
		/// <returns></returns>
		virtual glm::vec3 Sample(const glm::vec2& uv) const = 0;

		/// <summary>
		/// Returns the average color for this sampler
		/// </summary>
		virtual glm::vec3 Average() const = 0;
	};

	/// <summary>
	/// A 3D Sampler
	/// </summary>
	class Sampler3D
	{
	public:
		/// <summary>
		/// Samples in the direction given
		/// </summary>
		/// <param name="uvw">The direction to sample</param>
		/// <returns></returns>
		virtual glm::vec3 Sample(const glm::vec3& uvw) const = 0;
	};

	/// <summary>
	/// A uniform color sampler
	/// </summary>
	class ColorSampler : public Sampler2D, public Sampler3D
	{
	public:

		/// <summary>
		/// Constructs a new instance with the given color
		/// </summary>
		/// <param name="color">The color for this sampler</param>
		ColorSampler(const glm::vec3& color) : m_Color(color) {}

		glm::vec3 Sample(const glm::vec2& uv) const override { return m_Color; }
		glm::vec3 Sample(const glm::vec3& uvw) const override { return m_Color; }
		glm::vec3 Average() const override { return m_Color; }
	private:
		glm::vec3 m_Color;
	};

	enum class SampleMode
	{
		Linear, Nearest
	};

	/// <summary>
	/// An image
	/// </summary>
	class Image : public Sampler2D 
	{
	public:

		rt::SampleMode SampleMode = rt::SampleMode::Linear;

		/// <summary>
		/// Constructs a new instance with 0x0 size
		/// </summary>
		Image();

		/// <summary>
		/// Constructs a new instance
		/// </summary>
		/// <param name="width">Width in pixels</param>
		/// <param name="height">Height in pixels</param>
		Image(size_t width, size_t height);


		/// <summary>
		/// Resize this image (data is lost)
		/// </summary>
		/// <param name="width">The new width in pixels</param>
		/// <param name="height">The new height in pixels</param>
		void Resize(size_t width, size_t height);

		glm::vec3 Average() const override;
		glm::vec3 Sample(const glm::vec2& uv) const override;

		/// <summary>
		/// Sets the color of a pixel
		/// </summary>
		/// <param name="x">X position</param>
		/// <param name="y">Y position</param>
		/// <param name="color">The color</param>
		void SetPixel(size_t x, size_t y, const glm::vec3& color);

		/// <summary>
		/// Returns the color of a pixel
		/// </summary>
		/// <param name="x">X position</param>
		/// <param name="y">Y position</param>
		/// <returns>A color</returns>
		glm::vec3 GetPixel(size_t x, size_t y) const;
		glm::vec3 GetPixel(const glm::uvec2 xy) const;

		/// <summary>
		/// Returns the width if this image
		/// </summary>
		size_t GetWidth() const { return m_Width; }

		/// <summary>
		/// Returns the height is this image
		/// </summary>
		/// <returns></returns>
		size_t GetHeight() const { return m_Height; }

		void Load(std::string_view fileName);

		/// <summary>
		/// Normalize this image in low dynamic range
		/// </summary>
		void ToLDR();

	private:
		uint32_t m_Width, m_Height;
		std::vector<glm::vec3> m_Pixels;
	};

	/// <summary>
	/// An equirectangular sampler
	/// </summary>
	class EquirectangularMap : public Sampler3D
	{
	public:
		/// <summary>
		/// Constructs a new instance
		/// </summary>
		/// <param name="image">The image to sample from</param>
		EquirectangularMap(const std::shared_ptr<Image>& image) : m_Image(image) {}
		glm::vec3 Sample(const glm::vec3& uvw) const override;
	private:
		std::shared_ptr<Image> m_Image;
	};

}