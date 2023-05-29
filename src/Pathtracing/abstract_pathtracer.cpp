#include "abstract_pathtracer.h"

#include <optional>
#include <limits>
#include <spdlog/spdlog.h>

#include "rng.h"

namespace rt {

	std::shared_ptr<pathtracer_result> abstract_pathtracer::run(const view_parameters& view_params, const trace_parameters& trace_params, scene& scene)
	{
		scene.compile();
		return std::make_shared<pathtracer_result>([&, trace_params, view_params](pathtracer_result& self) -> void {
				
			std::mutex line_mutex;

			image image(view_params.width, view_params.height);
			
			const auto forward = glm::normalize(scene.camera.get_direction());
			const auto right = glm::normalize(glm::cross(forward, glm::vec3{ 0.0f, 1.0f, 0.0f }));
			const auto up = glm::cross(right, forward);

			const float h2 = std::atan(view_params.fov_y / 2.0f);
			const float w2 = h2 * (float)view_params.width / view_params.height;

			auto next_iteration = [trace_params, current = uint64_t(0)] () mutable -> std::optional<uint64_t> {
				if (trace_params.iterations != 0 && current == trace_params.iterations)
				{
					return std::nullopt;
				}
				else
				{
					return current++;
				}
			};

			for (auto it = next_iteration(); it.has_value() && !self.is_interrupted(); it = next_iteration())
			{
				self.on_iteration_start(it.value());

				auto next_scan_line = [&, current = uint32_t(0)]() mutable -> std::optional<uint32_t> {
					std::lock_guard guard(line_mutex);

					if (current == image.get_width())
					{
						return std::nullopt;
					}
					else
					{
						self.progress = current / float(image.get_width());
						return current++;
					}
				};


				const auto thread_func = [&] (const std::uint32_t seed) {

					// Sync here ?

					rng::seed(seed);

					for (auto scan_line = next_scan_line(); !self.is_interrupted() && scan_line.has_value(); scan_line = next_scan_line())
					{
						const auto x = scan_line.value();
						const float t = it.value() / float(it.value() + 1);

						for (uint32_t y = 0; y < view_params.height && !self.is_interrupted(); ++y)
						{
							glm::vec3 color(0.0f);

							for (size_t s = 0; s < trace_params.samples_per_iteration && !self.is_interrupted(); ++s)
							{
								ray r;
								
								float fx = rng::next() - 0.5f + x;
								float fy = rng::next() - 0.5f + y;
								
								float x_factor = fx / view_params.width * 2.0f - 1.0f;
								float y_factor = 1.0f - fy / view_params.height * 2.0f;

								r.origin = scene.camera.position;
								r.direction = glm::normalize(forward + right * x_factor * w2 + up * y_factor * h2);
								
								color += trace(view_params, r, scene);
							}

							auto prev_color = image.get_pixel(x, y);
							auto next_color = glm::mix(color / float(trace_params.samples_per_iteration), prev_color, t);
							image.set_pixel(x, y, next_color);
						}

						// ScanLine end
					}
				};

				std::vector<std::thread> threads(trace_params.num_threads);

				for (size_t i = 0; i < threads.size(); ++i)
					threads[i] = std::thread(thread_func, rng::next<std::uint32_t>(0u, std::numeric_limits<std::uint32_t>::max()));

				for (size_t i = 0; i < threads.size(); ++i)
					threads[i].join();

				self.iteration = it.value();
				self.samples_per_pixel += trace_params.samples_per_iteration;

				// iteration End
				self.on_iteration_end(image, it.value());


			}

			self.on_end(image);


		});

	}
	
	pathtracer_result::pathtracer_result(const fn& fn)
	{
		m_thread = std::thread([&, fn] { fn(*this); });
		m_start_time = std::chrono::system_clock::now();
	}

	pathtracer_result::~pathtracer_result()
	{
		if (m_thread.joinable())
			m_thread.join();
	}

	float pathtracer_result::get_elapsed_time() const
	{
		using seconds = std::chrono::duration<float, std::ratio<1>>;
		return seconds(std::chrono::system_clock::now() - m_start_time).count();
	}

}