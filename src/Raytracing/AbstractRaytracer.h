#pragma once

#include <cinttypes>
#include <memory>
#include <future>
#include <atomic>
#include <optional>

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
		std::atomic<float> Progress = 0.0f;
		std::atomic_uint64_t Iteration = 0;

		RaytracerResult(const ViewParameters& viewParams, const Fn& fn);
		~RaytracerResult();


		void Wait() { m_Thread.join(); }
		void Interrupt() { m_Interrupted = true; }
		bool IsInterrupted() const { return m_Interrupted; }
		Image& GetImage() { return m_Result; }

	private:
		std::thread m_Thread;
		Image m_Result;
		std::atomic_bool m_Interrupted = false;
	};

	struct ScanLine
	{
		uint32_t Number = 0;
		size_t Iteration = 0;
	};


	class AbstractRaytracer 
	{
	public:
		std::shared_ptr<RaytracerResult> Run(const ViewParameters& params, const Scene& scene);
		virtual glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) = 0;
		virtual std::optional<ScanLine> GetNextScanline() = 0;
		virtual void OnBeforeRun(const ViewParameters& params, const Scene& scene) = 0;
	};

}