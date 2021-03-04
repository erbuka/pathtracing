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
		void Wait();
		const Image& GetResult() const;
		void SetResult(Image&& result);
	private:
		std::thread m_Thread;
		Image m_Result;
		std::mutex m_Mutex;
	};


	class AbstractRaytracer 
	{
	public:

		std::future<Image> Run(const ViewParameters& params, const Scene& scene);
		virtual glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) = 0;

	};

}