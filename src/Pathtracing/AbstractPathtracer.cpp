#include "AbstractPathtracer.h"

#include <optional>
#include <random>

#include <spdlog/spdlog.h>

namespace rt {
	std::shared_ptr<PathtracerResult> AbstractPathtracer::Run(const ViewParameters& viewParams, const TraceParameters& traceParams, Scene& scene)
	{
		scene.Compile();
		return std::make_shared<PathtracerResult>([&, traceParams, viewParams](PathtracerResult& self) -> void {
			
			std::mt19937_64 rng;
			std::uniform_real_distribution<float> r01(0.0f, 1.0f);

			std::mutex lineMutex;

			Image image(viewParams.Width, viewParams.Height);
			
			const auto forward = glm::normalize(scene.Camera.GetDirection());
			const auto right = glm::normalize(glm::cross(forward, glm::vec3{ 0.0f, 1.0f, 0.0f }));
			const auto up = glm::cross(right, forward);

			const float h2 = std::atan(viewParams.FovY / 2.0f);
			const float w2 = h2 * (float)viewParams.Width / viewParams.Height;

			auto nextIteration = [traceParams, current = uint64_t(0)] () mutable -> std::optional<uint64_t> {
				if (traceParams.Iterations != 0 && current == traceParams.Iterations)
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
				self.OnIterationStart(it.value());

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

					// Sync here ?

					for (auto scanLine = nextScanLine(); !self.IsInterrupted() && scanLine.has_value(); scanLine = nextScanLine())
					{
						const auto x = scanLine.value();
						const float t = it.value() / float(it.value() + 1);

						for (uint32_t y = 0; y < viewParams.Height && !self.IsInterrupted(); ++y)
						{
							glm::vec3 color(0.0f);

							for (size_t s = 0; s < traceParams.SamplesPerIteration && !self.IsInterrupted(); ++s)
							{
								Ray ray;
								
								float fx = r01(rng) - 0.5f + x;
								float fy = r01(rng) - 0.5f + y;
								
								float xFactor = fx / viewParams.Width * 2.0f - 1.0f;
								float yFactor = 1.0f - fy / viewParams.Height * 2.0f;

								ray.Origin = scene.Camera.Position;
								ray.Direction = glm::normalize(forward + right * xFactor * w2 + up * yFactor * h2);
								
								color += Trace(viewParams, ray, scene);
							}

							auto prevColor = image.GetPixel(x, y);
							auto nextColor = glm::mix(color / float(traceParams.SamplesPerIteration), prevColor, t);
							image.SetPixel(x, y, nextColor);
						}

						// ScanLine end
					}
				};

				std::vector<std::thread> threads(traceParams.NumThreads);

				for (size_t i = 0; i < threads.size(); ++i)
					threads[i] = std::thread(threadFunc);

				for (size_t i = 0; i < threads.size(); ++i)
					threads[i].join();

				self.Iteration = it.value();
				self.SamplesPerPixel += traceParams.SamplesPerIteration;

				// Iteration End
				self.OnIterationEnd(image, it.value());


			}

			self.OnEnd(image);


		});

	}
	
	PathtracerResult::PathtracerResult(const Fn& fn)
	{
		m_Thread = std::thread([&, fn] { fn(*this); });
		m_StartTime = std::chrono::system_clock::now();
	}

	PathtracerResult::~PathtracerResult()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	float PathtracerResult::GetElapsedTime() const
	{
		using seconds = std::chrono::duration<float, std::ratio<1>>;
		std::chrono::duration<float> x;
		return seconds(std::chrono::system_clock::now() - m_StartTime).count();
	}

}