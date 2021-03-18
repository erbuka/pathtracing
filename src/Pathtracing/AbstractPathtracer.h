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


	/// <summary>
	/// An event emitter
	/// </summary>
	/// <typeparam name="...Args">The event arguments</typeparam>
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
		/// <summary>
		/// Subscribe to this event emitter
		/// </summary>
		/// <param name="fn">The handler function</param>
		void Subscribe(const HandlerFn& fn)
		{
			m_Handlers.push_back(fn);
		}

		/// <summary>
		/// Subscribe to this event emitter with a member function
		/// </summary>
		/// <typeparam name="T">The type of the instance</typeparam>
		/// <param name="instance">The instance of the member function</param>
		/// <param name="fn">The member function</param>
		template<typename T>
		void Subscribe(T* instance, MemberHandlerFn<T> fn)
		{
			Subscribe([instance, fn](Args&&... args) {
				(instance->*fn)(std::forward<Args>(args)...);
			});
		}

		/// <summary>
		/// Emit an event
		/// </summary>
		/// <param name="...evt">The event's parameters</param>

		template<typename... EvtArgs>
		void Emit(EvtArgs&&... evt)
		{
			for (auto& h : m_Handlers)
				h(std::forward<EvtArgs>(evt)...);
		}

		/// <summary>
		/// Functional operator to emit an event
		/// </summary>
		/// <param name="...evt">The event's parameters</param>

		template<typename... EvtArgs>
		void operator()(EvtArgs&&... evt)
		{
			Emit(std::forward<EvtArgs>(evt)...);
		}

	};

	/// <summary>
	/// View paramters for the pathtracer
	/// </summary>
	struct ViewParameters 
	{
		uint32_t Width = 800;
		uint32_t Height = 600;
		float FovY = glm::pi<float>() / 4.0f;
	};

	/// <summary>
	/// Tech parameters for the pathtracer
	/// </summary>
	struct TraceParameters
	{
		uint32_t NumThreads = 4;
		uint64_t Iterations = 1;
		uint64_t SamplesPerIteration = 1;
	};

	/// <summary>
	/// Class to query information about the pathtracing process and
	/// getting the result back
	/// </summary>
	class PathtracerResult
	{
	public:
		using Fn = std::function<void(PathtracerResult&)>;
		/// <summary>
		/// Progress of the current iteration
		/// </summary>
		std::atomic<float> Progress = 0.0f; 

		/// <summary>
		/// Current iteration
		/// </summary>
		std::atomic_uint64_t Iteration = 0;

		/// <summary>
		/// Current samples per pixel
		/// </summary>
		std::atomic_uint64_t SamplesPerPixel = 0;

		PathtracerResult(const Fn& fn);
		~PathtracerResult();
		
		/// <summary>
		/// Wait until the end of the process
		/// </summary>
		void Wait() { m_Thread.join(); }

		/// <summary>
		/// Interrupts the process
		/// </summary>
		void Interrupt() { m_Interrupted = true; }

		/// <summary>
		/// Check if the process has been interrupted
		/// </summary>
		bool IsInterrupted() const { return m_Interrupted; }
		
		/// <summary>
		/// Get the time since start
		/// </summary>
		/// <returns>Time since start in seconds</returns>
		float GetElapsedTime() const;

		/// <summary>
		/// Event: fires when a new iteration starts
		/// </summary>
		EventEmitter<const uint64_t&> OnIterationStart;
		
		/// <summary>
		/// Event: fires when the current iteration ends
		/// </summary>
		EventEmitter<const Image&, const uint64_t&> OnIterationEnd;
		

		/// <summary>
		/// Event: fires when the process is complete
		/// </summary>
		EventEmitter<const Image&> OnEnd;

	private:
		std::thread m_Thread;
		std::atomic_bool m_Interrupted = false;
		std::chrono::system_clock::time_point m_StartTime;
	};

	/// <summary>
	/// An abstact pathtracer. Subclasses must override the "Trace" method
	/// </summary>
	class AbstractPathtracer 
	{
	public:
		/// <summary>
		/// Runs the pathtracer with the given paramters and scene. The Trace function is invoked on each
		/// pixel. The ray is cast randomly within the pixel bounds.
		/// </summary>
		/// <param name="viewParams">The view paramters</param>
		/// <param name="traceParams">The technical parameters</param>
		/// <param name="scene">The scene to render</param>
		/// <returns>A pointer to the result</returns>
		std::shared_ptr<PathtracerResult> Run(const ViewParameters& viewParams, const TraceParameters& traceParams, Scene& scene);
		
		/// <summary>
		/// Trace a screen ray (ai, a ray cast from the camera through a pixel) and returns its radiance
		/// </summary>
		/// <param name="params">The view parameters</param>
		/// <param name="ray">The ray</param>
		/// <param name="scene">The scene</param>
		/// <returns>A color representing the radiance</returns>
		virtual glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) = 0;
	};

}