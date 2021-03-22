#pragma once

#include <glm/glm.hpp>

#include <string_view>
#include <cinttypes>
#include <vector>
#include <memory>

namespace rt {

	/// <summary>
	/// A 2D Sampler
	/// </summary>
	class sampler_2d
	{
	public:
		/// <summary>
		/// Samples at the given uv
		/// </summary>
		/// <param name="uv">Uv coordinates</param>
		/// <returns></returns>
		virtual glm::vec3 sample(const glm::vec2& uv) const = 0;

		/// <summary>
		/// Returns the average color for this sampler
		/// </summary>
		virtual glm::vec3 average() const = 0;
	};

	/// <summary>
	/// A 3D Sampler
	/// </summary>
	class sampler_3d
	{
	public:
		/// <summary>
		/// Samples in the direction given
		/// </summary>
		/// <param name="uvw">The direction to sample</param>
		/// <returns></returns>
		virtual glm::vec3 sample(const glm::vec3& uvw) const = 0;
	};

	/// <summary>
	/// A uniform color sampler
	/// </summary>
	class color_sampler : public sampler_2d, public sampler_3d
	{
	public:

		/// <summary>
		/// Constructs a new instance with the given color
		/// </summary>
		/// <param name="color">The color for this sampler</param>
		color_sampler(const glm::vec3& color) : m_color(color) {}

		glm::vec3 sample(const glm::vec2& uv) const override { return m_color; }
		glm::vec3 sample(const glm::vec3& uvw) const override { return m_color; }
		glm::vec3 average() const override { return m_color; }
	private:
		glm::vec3 m_color;
	};

	enum class sample_mode
	{
		linear, nearest
	};

	/// <summary>
	/// An image
	/// </summary>
	class image : public sampler_2d 
	{
	public:

		rt::sample_mode sample_mode = rt::sample_mode::linear;

		/// <summary>
		/// Constructs a new instance with 0x0 size
		/// </summary>
		image();

		/// <summary>
		/// Constructs a new instance
		/// </summary>
		/// <param name="width">width in pixels</param>
		/// <param name="height">height in pixels</param>
		image(size_t width, size_t height);


		/// <summary>
		/// Resize this image (data is lost)
		/// </summary>
		/// <param name="width">The new width in pixels</param>
		/// <param name="height">The new height in pixels</param>
		void resize(size_t width, size_t height);

		glm::vec3 average() const override;
		glm::vec3 sample(const glm::vec2& uv) const override;

		/// <summary>
		/// Sets the color of a pixel
		/// </summary>
		/// <param name="x">X position</param>
		/// <param name="y">Y position</param>
		/// <param name="color">The color</param>
		void set_pixel(size_t x, size_t y, const glm::vec3& color);

		/// <summary>
		/// Returns the color of a pixel
		/// </summary>
		/// <param name="x">X position</param>
		/// <param name="y">Y position</param>
		/// <returns>A color</returns>
		glm::vec3 get_pixel(size_t x, size_t y) const;
		glm::vec3 get_pixel(const glm::uvec2 xy) const;

		/// <summary>
		/// Returns the width if this image
		/// </summary>
		size_t get_width() const { return m_width; }

		/// <summary>
		/// Returns the height is this image
		/// </summary>
		/// <returns></returns>
		size_t get_height() const { return m_height; }

		void load(std::string_view fileName);

		/// <summary>
		/// Normalize this image in low dynamic range
		/// </summary>
		void to_ldr();

	private:
		uint32_t m_width, m_height;
		std::vector<glm::vec3> m_pixels;
	};

	/// <summary>
	/// An equirectangular sampler
	/// </summary>
	class equirectangular_map : public sampler_3d
	{
	public:
		/// <summary>
		/// Constructs a new instance
		/// </summary>
		/// <param name="image">The image to sample from</param>
		equirectangular_map(const std::shared_ptr<image>& image) : m_image(image) {}
		glm::vec3 sample(const glm::vec3& uvw) const override;
	private:
		std::shared_ptr<image> m_image;
	};

}