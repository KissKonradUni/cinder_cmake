#include "layers/devtools.hpp"
#include "host.hpp"

#include "imgui.h"

#ifdef __linux__
#include <fstream>
#endif

namespace echo {

// TODO: refactor whole thing
PhaseState DevToolsLayer::onPrepareFrame() {
	if (!m_showDevToolsWindow)
		return PhaseState::Continue;

	ImGui::Begin("DevTools", &m_showDevToolsWindow);

	ImGui::BeginTable("DevToolsTable", 2, ImGuiTableFlags_Borders);
	ImGui::TableNextColumn();

	ImGui::Text("Timings: ");
	ImGui::Text("  Total Time: %.3f s", m_host->getTime().totalTime);
	ImGui::Text("  Update Delta: %.3f ms", m_host->getTime().updateDelta);
	ImGui::Text("  Render Delta: %.3f ms", m_host->getTime().renderDelta);
	
    static float frameTimeHistory[128] = {0};
	static int frameTimeHistoryIndex = 0;
    static float maxFrameTime = 0.01f;

	frameTimeHistory[frameTimeHistoryIndex] = m_host->getTime().renderDelta;
    if (frameTimeHistory[frameTimeHistoryIndex] > maxFrameTime) {
        maxFrameTime = frameTimeHistory[frameTimeHistoryIndex];
    }
	frameTimeHistoryIndex = (frameTimeHistoryIndex + 1) % 128;
	ImGui::PlotLines("##FrameTimeHistory", frameTimeHistory,
	                 IM_ARRAYSIZE(frameTimeHistory), frameTimeHistoryIndex,
	                 nullptr, 0.0f, maxFrameTime, ImVec2(-0.1f, 80.0f));

	ImGui::TableNextColumn();

	ImGui::Text("Memory: ");

#ifdef __linux__
	static double lastMeasurementTime = 0.0;
	static size_t size, resident, shared, pageSize;

	if (m_host->getTime().totalTime - lastMeasurementTime > 0.1) {
		lastMeasurementTime = m_host->getTime().totalTime;

		std::ifstream statm("/proc/self/statm");
		if (statm.is_open()) {
			statm >> size >> resident >> shared;
			pageSize = sysconf(_SC_PAGESIZE);
		}
		statm.close();
	}

	ImGui::Text("  Resident: %.2f MB",
	            (static_cast<double>(resident) * pageSize) / (1024.0 * 1024.0));
	ImGui::Text("  Shared: %.2f MB",
	            (static_cast<double>(shared) * pageSize) / (1024.0 * 1024.0));
    ImGui::NewLine();

    static float residentMemoryHistory[128] = {0};
    static int residentMemoryHistoryIndex = 0;
    static float maxResidentMemory = 0.0f;
    residentMemoryHistory[residentMemoryHistoryIndex] = resident * pageSize / (1024.0f * 1024.0f);
    if (residentMemoryHistory[residentMemoryHistoryIndex] > maxResidentMemory) {
        maxResidentMemory = residentMemoryHistory[residentMemoryHistoryIndex];
    }
    residentMemoryHistoryIndex = (residentMemoryHistoryIndex + 1) % 128;
    ImGui::PlotHistogram("##ResidentMemoryHistory", residentMemoryHistory,
                     IM_ARRAYSIZE(residentMemoryHistory), residentMemoryHistoryIndex,
                     nullptr, 0.0f, maxResidentMemory, ImVec2(-0.1f, 80.0f));
#else
	ImGui::Text("  Memory info not available on this platform.");
#endif

	ImGui::EndTable();

	ImGui::End();

	return PhaseState::Continue;
}

EventState DevToolsLayer::onEvent(SDL_Event* event) {
	if (event->type == SDL_EVENT_KEY_DOWN) {
		if (event->key.scancode == SDL_SCANCODE_F12) {
			m_showDevToolsWindow = !m_showDevToolsWindow;
			return EventState::Consume;
		}
	}

	return EventState::Propagate;
}

} // namespace echo