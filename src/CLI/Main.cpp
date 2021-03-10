#include <vector>

#include <spdlog\spdlog.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <Pathtracer.h>
#include <SceneLoader.h>




int main(int argc, char** argv)
{
	uint32_t width = 512;
	uint32_t height = 512;
	size_t iterations = 10;
	size_t threads = 4;
	std::string sceneFile;
	std::string outFile = "result.png";

	for (size_t i = 1; i < argc; ++i)
	{
		std::string paramName = argv[i];

		if (paramName == "--scene")
		{
			sceneFile = argv[++i];
		}
		else if (paramName == "--out")
		{
			outFile = argv[++i];
		}
		else if (paramName == "--threads")
		{
			threads = std::stoull(argv[++i]);
		}
		else if (paramName == "--iterations")
		{
			iterations = std::stoull(argv[++i]);
		}
		else if (paramName == "--resolution")
		{
			width = std::stoull(argv[++i]);
			height = std::stoull(argv[++i]);
		}
		else
		{
			spdlog::error("Unknown parameter: {0}", paramName);
			return -1;
		}
	}

	auto scene = rt::utility::LoadScene(sceneFile);

	spdlog::info("Starting pathtracing");
	spdlog::info(" Scene: {0}", sceneFile);
	spdlog::info(" Threads: {0}", threads);
	spdlog::info(" Viewport: {0} x {1} px", width, height);
		
	rt::Pathtracer pathtracer;
	rt::ViewParameters params;

	params.Width = width;
	params.Height = height;
	params.NumThreads = threads;
	params.FovY = glm::pi<float>() / 4.0f;

	auto result = pathtracer.Run(params, scene, iterations);
	
	result->OnIterationEnd.Subscribe([iterations](const rt::Image& img, size_t iteration) {
		spdlog::info("Iteration completed: {0} / {1}", iteration + 1, iterations);
	});

	result->OnEnd.Subscribe([outFile](const rt::Image& image) {
		// Save image here
		std::vector<uint32_t> pixels(image.GetHeight() * image.GetWidth());

		for (uint32_t x = 0; x < image.GetWidth(); ++x)
		{
			for (uint32_t y = 0; y < image.GetHeight(); ++y)
			{
				auto color = image.GetPixel(x, y);

				// Tone mapping
				color = glm::vec3(1.0f) - glm::exp(-color);

				// Gamma correction
				color = glm::pow(color, glm::vec3(1.0f / 2.2f));

				pixels[y * image.GetWidth() + x] =
					((uint32_t(color.r * 255) << 0)) |
					((uint32_t(color.g * 255) << 8)) |
					((uint32_t(color.b * 255) << 16)) |
					((uint32_t(255) << 24));
			}
		}

		stbi_write_png(outFile.c_str(), image.GetWidth(), image.GetHeight(), 4, pixels.data(), 0);

		spdlog::info("Image saved: {0}", outFile);

	});

	result->Wait();

}