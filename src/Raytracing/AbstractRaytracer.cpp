#include "AbstractRaytracer.h"

#include <optional>
#include <random>

#include <spdlog/spdlog.h>

namespace rt {
	std::shared_ptr<RaytracerResult> AbstractRaytracer::Run(const ViewParameters& params, Scene& scene, size_t iterations)
	{
		scene.Compile();
		return std::make_shared<RaytracerResult>([&, iterations, params](RaytracerResult& self) -> void {
			
			std::mt19937 rng;
			std::uniform_real_distribution<float> r01(0.0f, 1.0f);

			std::mutex lineMutex;

			Image image(params.Width, params.Height);

			auto nextIteration = [iterations, current = size_t(0)] () mutable -> std::optional<size_t> {
				if (iterations != 0 && current == iterations)
				{
					return std::nullopt;
				}
				else
				{
					return current++;
				}
			};

			for (auto it = nextIteration(); it.has_value() && !self.IsInterrupted(); it = nextIteration())
			{

				self.Iteration = it.value();

				auto nextScanLine = [&, current = uint32_t(0)]() mutable -> std::optional<uint32_t> {
					std::lock_guard guard(lineMutex);

					if (current == image.GetWidth())
					{
						return std::nullopt;
					}
					else
					{
						self.Progress = current / float(image.GetWidth());
						return current++;
					}
				};

				const auto threadFunc = [&] {

					auto forward = glm::normalize(scene.Camera.GetDirection());
					auto right = glm::normalize(glm::cross(forward, glm::vec3{ 0.0f, 1.0f, 0.0f }));
					auto up = glm::cross(right, forward);

					float h2 = std::atan(params.FovY / 2.0f);
					float w2 = h2 * (float)params.Width / params.Height;

					for (auto scanLine = nextScanLine(); !self.IsInterrupted() && scanLine.has_value(); scanLine = nextScanLine())
					{
						const auto x = scanLine.value();
						const float t = it.value() / float(it.value() + 1);

						for (uint32_t y = 0; y < params.Height && !self.IsInterrupted(); ++y)
						{
							Ray ray;

							float fx = r01(rng) - 0.5f + x;
							float fy = r01(rng) - 0.5f + y;

							float xFactor = fx / params.Width * 2.0f - 1.0f;
							float yFactor = 1.0f - fy / params.Height * 2.0f;

							ray.Origin = scene.Camera.Position;
							ray.Direction = glm::normalize(forward + right * xFactor * w2 + up * yFactor * h2);

							auto color = Trace(params, ray, scene);
							auto prevColor = image.GetPixel(x, y);
							auto nextColor = glm::mix(color, prevColor, t);
							image.SetPixel(x, y, nextColor);
						}

						// ScanLine end
					}
				};

				std::vector<std::thread> threads(params.NumThreads);



				for (size_t i = 0; i < threads.size(); ++i)
					threads[i] = std::thread(threadFunc);

				for (size_t i = 0; i < threads.size(); ++i)
					threads[i].join();

				// Iteration End
				self.OnIterationEnd(image, self.Iteration);


			}

			self.OnEnd(image);


		});

	}
	
	RaytracerResult::RaytracerResult(const Fn& fn)
	{
		m_Thread = std::thread([&, fn] { fn(*this); });
	}

	RaytracerResult::~RaytracerResult()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

}