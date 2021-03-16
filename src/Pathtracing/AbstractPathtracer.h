#pragma once

#include <cinttypes>
#include <memory>
#include <future>
#include <atomic>
#include <optional>
#include <list>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Scene.h"
#include "Sampler.h"

namespace rt {

	class Scene;
	class AbstractPathtracer;

	template<typename... Args>
	class EventEmitter
	{
	public:
		
		using HandlerFn = std::function<void(Args...)>;
		template<typename T>
		using MemberHandlerFn = void(T::*)(Args...);

	private:
		std::list<HandlerFn> m_Handlers;
	public:
		void Subscribe(const HandlerFn& fn)
		{
			m_Handlers.push_back(fn);
		}

		template<typename T>
		void Subscribe(T* instance, MemberHandlerFn<T> fn)
		{
			Subscribe([instance, fn](Args&&... args) {
				(instance->*fn)(std::forward<Args>(args)...);
			});
		}

		void Emit(Args&&... evt) 
		{
			for (auto& h : m_Handlers)
				h(std::forward<Args>(evt)...);
		}

		void operator()(Args&&... evt)
		{
			Emit(std::forward<Args>(evt)...);
		}

	};

	struct ViewParameters 
	{
		uint32_t Width = 800;
		uint32_t Height = 600;
		float FovY = glm::pi<float>() / 4.0f;
	};

	struct TraceParameters
	{
		uint32_t NumThreads = 4;
		uint64_t Iterations = 1;
		uint64_t SamplesPerIteration = 1;
	};

	class PathtracerResult
	{
	public:
		using Fn = std::function<void(PathtracerResult&)>;
		std::atomic<float> Progress = 0.0f;
		std::atomic_uint64_t Iteration = 0;
		std::atomic_uint64_t SamplesPerPixel = 0;

		PathtracerResult(const Fn& fn);
		~PathtracerResult();
		
		void Wait() { m_Thread.join(); }
		void Interrupt() { m_Interrupted = true; }
		bool IsInterrupted() const { return m_Interrupted; }
		float GetElapsedTime() const;

		EventEmitter<const uint64_t&> OnIterationStart;
		EventEmitter<const Image&, const uint64_t&> OnIterationEnd;
		EventEmitter<const Image&> OnEnd;

	private:
		std::thread m_Thread;
		std::atomic_bool m_Interrupted = false;
		std::chrono::system_clock::time_point m_StartTime;
	};


	class AbstractPathtracer 
	{
	public:

		std::shared_ptr<PathtracerResult> Run(const ViewParameters& viewParams, const TraceParameters& traceParams, Scene& scene);
		virtual glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) = 0;
	};

}