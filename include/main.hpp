#pragma once

#include "modloader/shared/modloader.hpp"

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"

#include "GlobalNamespace/BeatmapLevelsModel.hpp"
using namespace GlobalNamespace;

Configuration& getConfig();
const Logger& getLogger();

// Convenience method for finding the BeatmapLevelsModel if it hasn't been found already.
BeatmapLevelsModel* getBeatmapLevelsModel();

enum Mode   {
    DISABLE = false,
    ENABLE = true
};
// Converts a mode to a string
std::string modeToString(Mode mode);

Mode swapOverrideMode(); // Swaps the current override mode to the other value. Returns the new mode
Mode getOverrideMode(); // Gets the current override mode

// Adds this playlist to the map (for quick access) and to the config file
//void setPlaylistOverridden(std::string name);

