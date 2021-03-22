#include <vector>

#include <spdlog/spdlog.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <pathtracer.h>
#include <scene_loader.h>


int main(int argc, char** argv)
{
	uint32_t width = 512;
	uint32_t height = 512;
	size_t iterations = 10;
	size_t threads = 4;
	std::string scene_file;
	std::string out_file = "result.png";

	for (size_t i = 1; i < argc; ++i)
	{
		std::string param_name = argv[i];

		if (param_name == "--scene")
		{
			scene_file = argv[++i];
		}
		else if (param_name == "--out")
		{
			out_file = argv[++i];
		}
		else if (param_name == "--threads")
		{
			threads = std::stoull(argv[++i]);
		}
		else if (param_name == "--iterations")
		{
			iterations = std::stoull(argv[++i]);
		}
		else if (param_name == "--resolution")
		{
			width = std::stoull(argv[++i]);
			height = std::stoull(argv[++i]);
		}
		else
		{
			spdlog::error("Unknown parameter: {0}", param_name);
			return -1;
		}
	}

	auto scene = rt::utility::load_scene(scene_file);

	spdlog::info("Starting pathtracing");
	spdlog::info(" Scene: {0}", scene_file);
	spdlog::info(" Threads: {0}", threads);
	spdlog::info(" Viewport: {0} x {1} px", width, height);
		
	rt::pathtracer pathtracer;
	rt::view_parameters view_params;
	rt::trace_parameters trace_params;

	view_params.width = width;
	view_params.height = height;
	view_params.fov_y = glm::pi<float>() / 4.0f;

	trace_params.num_threads = threads;
	trace_params.iterations = iterations;
	trace_params.samples_per_iteration = 256;

	auto result = pathtracer.run(view_params, trace_params, scene);
	
	result->on_iteration_end.subscribe([trace_params, result, iterations](const rt::image& img, const uint64_t& iteration) {
		const float elapsed_time = result->get_elapsed_time();
		const auto samples = result->samples_per_pixel.load();
		const float eta = (trace_params.iterations - (iteration + 1)) * (elapsed_time / (iteration + 1));

		spdlog::info("Iteration completed: {0} / {1}, {2} spp/sec, ETA: {3:.2f}", iteration + 1, iterations, samples / elapsed_time, eta);
	});


	result->on_end.subscribe([out_file](const rt::image& image) {
		// Save image here
		std::vector<uint32_t> pixels(image.get_height() * image.get_width());

		for (uint32_t x = 0; x < image.get_width(); ++x)
		{
			for (uint32_t y = 0; y < image.get_height(); ++y)
			{
				auto color = image.get_pixel(x, y);

				// Tone mapping
				color = glm::vec3(1.0f) - glm::exp(-color);

				// Gamma correction
				color = glm::pow(color, glm::vec3(1.0f / 2.2f));

				pixels[y * image.get_width() + x] =
					((uint32_t(color.r * 255) << 0)) |
					((uint32_t(color.g * 255) << 8)) |
					((uint32_t(color.b * 255) << 16)) |
					((uint32_t(255) << 24));
			}
		}

		stbi_write_png(out_file.c_str(), image.get_width(), image.get_height(), 4, pixels.data(), 0);

		spdlog::info("image saved: {0}", out_file);

	});

	result->wait();

}