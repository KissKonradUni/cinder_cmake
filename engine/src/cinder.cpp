#include "cinder.hpp"
#include "version.hpp"

#include <rfl/json.hpp>
#include <rfl.hpp>

#include <print>

namespace cinder {

void debugVersionInfo() {
    const std::string versionJson = rfl::json::write(version, rfl::json::pretty);
    std::println("Version Info:\n{}", versionJson);
}

}  // namespace cinder