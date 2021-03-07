#include "AbstractRaytracer.h"

#include <optional>

namespace rt {
	std::shared_ptr<RaytracerResult> AbstractRaytracer::Run(const ViewParameters& params, const Scene& scene)
	{
		OnBeforeRun(params, scene);

		return std::make_shared<RaytracerResult>(params, [&, params](RaytracerResult& self) -> void {

			std::mutex lineMutex;
			
			Image& image = self.GetImage();

			auto nextScanLine = [&]()-> std::optional<ScanLine> {
				std::lock_guard guard(lineMutex);
				auto sl = GetNextScanline();
				if (sl.has_value())
				{
					self.Progress = sl.value().Number / float(image.GetWidth());
					self.Iteration = sl.value().Iteration;
				}
				return sl;
			};

			const auto threadFunc = [&] {

				auto forward = glm::normalize(params.Camera.Direction);
				auto right = glm::normalize(glm::cross(forward, glm::vec3{ 0.0f, 1.0f, 0.0f }));
				auto up = glm::cross(right, forward);

				float h2 = std::atan(params.FovY / 2.0f);
				float w2 = h2 * (float)params.Width / params.Height;

				for (auto scanLine = nextScanLine(); !self.IsInterrupted() && scanLine.has_value(); scanLine=nextScanLine())
				{
					const auto s = scanLine.value();
					const auto x = s.Number;
					const auto iteration = s.Iteration;

					const float t = s.Iteration / float(s.Iteration + 1);

					for (uint32_t y = 0; y < params.Height && !self.IsInterrupted(); ++y)
					{
						Ray ray;

						float xFactor = ((float)x / params.Width) * 2.0f - 1.0f;
						float yFactor = 1.0f - ((float)y / params.Height) * 2.0f;

						ray.Origin = params.Camera.Position;
						ray.Direction = glm::normalize(forward + right * xFactor * w2 + up * yFactor * h2);

						auto color = Trace(params, ray, scene);
						auto prevColor = image.GetPixel(x, y);
						auto nextColor = glm::mix(color, prevColor, t);

						image.SetPixel(x, y, nextColor);
					}
				}
			};

			std::vector<std::thread> threads(params.NumThreads);

			for (size_t i = 0; i < threads.size(); ++i)
			{
				threads[i] = std::thread(threadFunc);
			}

			for (size_t i = 0; i < threads.size(); ++i)
			{
				threads[i].join();
			}

		});

	}
	
	RaytracerResult::RaytracerResult(const ViewParameters& viewParams, const Fn& fn)
	{
		m_Result.Resize(viewParams.Width, viewParams.Height);
		m_Thread = std::thread([&, fn] { fn(*this); });
		//m_Thread.detach();
	}

	RaytracerResult::~RaytracerResult()
	{
	}

}