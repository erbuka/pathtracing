#include "AbstractRaytracer.h"

#include <optional>

namespace rt {
	std::shared_ptr<RaytracerResult> AbstractRaytracer::Run(const ViewParameters& params, const Scene& scene)
	{
		return std::make_shared<RaytracerResult>([&, params](RaytracerResult& self) -> void {

			Image result(params.Width, params.Height);
			std::mutex lineMutex;

			self.SetMaxProgress(params.Width);

			auto nextScanLine = [&, current = uint32_t(0)]() mutable->std::optional<uint32_t>{
				std::lock_guard guard(lineMutex);
				if (current == params.Width)
				{
					return std::nullopt;
				}
				else
				{
					return current++;
				}
			};

			const auto threadFunc = [&] {


				auto forward = glm::normalize(params.Camera.Direction);
				auto right = glm::normalize(glm::cross(forward, glm::vec3{ 0.0f, 1.0f, 0.0f }));
				auto up = glm::cross(right, forward);

				float h2 = std::atan(params.FovY / 2.0f);
				float w2 = h2 * (float)params.Width / params.Height;


				for (auto scanLine = nextScanLine(); scanLine.has_value(); scanLine = nextScanLine())
				{
					uint32_t x = scanLine.value();
					for (uint32_t y = 0; y < params.Height; ++y)
					{
						Ray ray;

						float xFactor = ((float)x / params.Width) * 2.0f - 1.0f;
						float yFactor = 1.0f - ((float)y / params.Height) * 2.0f;

						ray.Origin = params.Camera.Position;
						ray.Direction = glm::normalize(forward + right * xFactor * w2 + up * yFactor * h2);

						auto color = Trace(params, ray, scene);

						result.SetPixel(x, y, color);
					}
					self.IncrementProgress(1);
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

			self.SetResult(std::move(result));

		});

	}
	
	RaytracerResult::RaytracerResult(const Fn& fn)
	{
		m_Thread = std::thread([&, fn] { fn(*this); });
		m_Thread.detach();
	}

	RaytracerResult::~RaytracerResult()
	{
		//m_Thread.join();
	}

	void RaytracerResult::Wait()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		while (!m_Ready)
			m_Cond.wait(lock);

		lock.unlock();
	}

	void RaytracerResult::SetResult(Image&& result)
	{
		std::lock_guard lock(m_Mutex);
		m_Result = std::move(result);
		m_Ready = true;
		m_Cond.notify_all();
	}
}