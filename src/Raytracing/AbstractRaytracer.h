#pragma once

#include <cinttypes>
#include <memory>
#include <future>
#include <atomic>

#include <glm\glm.hpp>
#include <glm\ext.hpp>

#include "Sampler.h"

namespace rt {

	class Scene;
	class AbstractRaytracer;

	struct ViewParameters 
	{
		uint32_t NumThreads = 8;
		uint32_t Width = 800;
		uint32_t Height = 600;
		float FovY = glm::pi<float>() / 4.0f;
		struct 
		{
			glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
			glm::vec3 Direction = { 0.0f, 0.0f, -1.0f };
		} Camera;
	};

	class RaytracerResult
	{
	public:
		using Fn = std::function<void(RaytracerResult&)>;
		RaytracerResult(const Fn& fn);
		~RaytracerResult();
		float GetProgressPercent() const { return float(m_Progress) / m_ProgressMax; }

		void IncrementProgress(uint32_t p) { m_Progress += p; }
		void SetProgress(uint32_t p) { m_Progress = p; }
		void SetMaxProgress(uint32_t m) { m_ProgressMax = m; }

		bool IsReady() const { return m_Ready; }
		void Wait();
		const Image& GetResult() const { return m_Result; }
		void SetResult(Image&& result);
	private:
		std::condition_variable m_Cond;
		std::thread m_Thread;
		Image m_Result;
		std::mutex m_Mutex;
		std::atomic_bool m_Ready = false;
		std::atomic_uint32_t m_Progress = 0, m_ProgressMax = 1;
	};


	class AbstractRaytracer 
	{
	public:

		std::shared_ptr<RaytracerResult> Run(const ViewParameters& params, const Scene& scene);
		virtual glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) = 0;

	};

}