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

#include "scene.h"
#include "sampler.h"

namespace rt {

	class scene;
	class abstract_pathtracer;


	/// <summary>
	/// An event emitter
	/// </summary>
	/// <typeparam name="...Args">The event arguments</typeparam>
	template<typename... Args>
	class event_emitter
	{
	public:
		
		using handler_fn = std::function<void(Args...)>;
		template<typename T>
		using member_handler_fn = void(T::*)(Args...);

	private:
		std::list<handler_fn> m_handlers;
	public:
		/// <summary>
		/// subscribe to this event emitter
		/// </summary>
		/// <param name="fn">The handler function</param>
		void subscribe(const handler_fn& fn)
		{
			m_handlers.push_back(fn);
		}

		/// <summary>
		/// subscribe to this event emitter with a member function
		/// </summary>
		/// <typeparam name="T">The type of the instance</typeparam>
		/// <param name="instance">The instance of the member function</param>
		/// <param name="fn">The member function</param>
		template<typename T>
		void subscribe(T* instance, member_handler_fn<T> fn)
		{
			subscribe([instance, fn](Args&&... args) {
				(instance->*fn)(std::forward<Args>(args)...);
			});
		}

		/// <summary>
		/// emit an event
		/// </summary>
		/// <param name="...evt">The event's parameters</param>

		template<typename... EvtArgs>
		void emit(EvtArgs&&... evt)
		{
			for (auto& h : m_handlers)
				h(std::forward<EvtArgs>(evt)...);
		}

		/// <summary>
		/// Functional operator to emit an event
		/// </summary>
		/// <param name="...evt">The event's parameters</param>

		template<typename... EvtArgs>
		void operator()(EvtArgs&&... evt)
		{
			emit(std::forward<EvtArgs>(evt)...);
		}

	};

	/// <summary>
	/// View paramters for the pathtracer
	/// </summary>
	struct view_parameters 
	{
		uint32_t width = 800;
		uint32_t height = 600;
		float fov_y = glm::pi<float>() / 4.0f;
	};

	/// <summary>
	/// Tech parameters for the pathtracer
	/// </summary>
	struct trace_parameters
	{
		uint32_t num_threads = 4;
		uint64_t iterations = 1;
		uint64_t samples_per_iteration = 1;
	};

	/// <summary>
	/// Class to query information about the pathtracing process and
	/// getting the result back
	/// </summary>
	class pathtracer_result
	{
	public:
		using fn = std::function<void(pathtracer_result&)>;
		/// <summary>
		/// Progress of the current iteration
		/// </summary>
		std::atomic<float> progress = 0.0f; 

		/// <summary>
		/// Current iteration
		/// </summary>
		std::atomic_uint64_t iteration = 0;

		/// <summary>
		/// Current samples per pixel
		/// </summary>
		std::atomic_uint64_t samples_per_pixel = 0;

		pathtracer_result(const fn& fn);
		~pathtracer_result();
		
		/// <summary>
		/// Wait until the end of the process
		/// </summary>
		void wait() { m_thread.join(); }

		/// <summary>
		/// Interrupts the process
		/// </summary>
		void interrupt() { m_interrupted = true; }

		/// <summary>
		/// Check if the process has been interrupted
		/// </summary>
		bool is_interrupted() const { return m_interrupted; }
		
		/// <summary>
		/// Get the time since start
		/// </summary>
		/// <returns>Time since start in seconds</returns>
		float get_elapsed_time() const;

		/// <summary>
		/// Event: fires when a new iteration starts
		/// </summary>
		event_emitter<const uint64_t&> on_iteration_start;
		
		/// <summary>
		/// Event: fires when the current iteration ends
		/// </summary>
		event_emitter<const image&, const uint64_t&> on_iteration_end;
		

		/// <summary>
		/// Event: fires when the process is complete
		/// </summary>
		event_emitter<const image&> on_end;

	private:
		std::thread m_thread;
		std::atomic_bool m_interrupted = false;
		std::chrono::system_clock::time_point m_start_time;
	};

	/// <summary>
	/// An abstact pathtracer. Subclasses must override the "Trace" method
	/// </summary>
	class abstract_pathtracer 
	{
	public:
		/// <summary>
		/// Runs the pathtracer with the given paramters and scene. The Trace function is invoked on each
		/// pixel. The ray is cast randomly within the pixel bounds.
		/// </summary>
		/// <param name="view_params">The view paramters</param>
		/// <param name="trace_params">The technical parameters</param>
		/// <param name="scene">The scene to render</param>
		/// <returns>A pointer to the result</returns>
		std::shared_ptr<pathtracer_result> run(const view_parameters& view_params, const trace_parameters& trace_params, scene& scene);
		
		/// <summary>
		/// Trace a screen ray (ai, a ray cast from the camera through a pixel) and returns its radiance
		/// </summary>
		/// <param name="params">The view parameters</param>
		/// <param name="ray">The ray</param>
		/// <param name="scene">The scene</param>
		/// <returns>A color representing the radiance</returns>
		virtual glm::vec3 trace(const view_parameters& params, const ray& ray, const scene& scene) = 0;
	};

}