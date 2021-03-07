#pragma once

#include "AbstractRaytracer.h"

namespace rt
{
	class DebugRaytracer : public AbstractRaytracer
	{
	public:

		enum class Mode
		{
			Color,
			Normal
		};

		Mode CurrentMode = Mode::Color;

		glm::vec3 Trace(const ViewParameters& params, const Ray& ray, const Scene& scene) override;
		std::optional<ScanLine> GetNextScanline() override;
		void OnBeforeRun(const ViewParameters& params, const Scene& scene) override;
	private:
		uint32_t m_NextScanline, m_ScanlineCount;
	};
}