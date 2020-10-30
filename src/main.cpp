#include "main.hpp"
#include "AutoDebrisViewController.hpp"
using namespace AutoDebris;

#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/OverrideEnvironmentSettings.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/LevelParamsPanel.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"

#include "questui/shared/QuestUI.hpp"
#include "custom-types/shared/register.hpp"

#include <optional>
#include <unordered_set>

static ModInfo modInfo;
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

const Logger& getLogger() {
    static const Logger logger(modInfo);
    return logger;
}

static Mode overrideMode; // Whether we are enabling or disabling reduce debris
Mode swapOverrideMode()  {
    // Find the opposite mode and set it back
    Mode newMode = (Mode) !((bool) overrideMode);
    overrideMode = newMode;

    // Save the swapped value back to the config
    getConfig().config["mode"].SetString(modeToString(overrideMode), getConfig().config.GetAllocator());
    return overrideMode;
}

Mode getOverrideMode()  {
    return overrideMode;
}

// Convenience function for converting a mode to a string
std::string modeToString(Mode mode)  {
    return (mode == Mode::ENABLE) ? "enable" : "disable";
}
/*
static std::unordered_set<std::string> overriddenPlaylists;
// Adds or removes from the overridden playlists
void setPlaylistOverridden(std::string playlist, bool newSetting)    {
    if(newSetting)  {
        overriddenPlaylists->insert(playlist);
    }   else    {
        overrideenPlaylists->remove(playlist);
    }
}

// Loads all of the overridden playlists from the config into a map for easy access
void loadOverriddenPlaylists()  {
    // Add each playlist to the map
    for(rapidjson::Value& value : getConfig().config["playlists"].GetArray())    {
        overriddenPlaylists.insert(value.GetString());
    }
}

void saveOverriddenPlaylists*/

static bool willOverride = false;

// Copies all the player settings into a new settings object
// Yes, I know this is a stupid way to do it
PlayerSpecificSettings* cloneSettings(PlayerSpecificSettings* settings) {
    PlayerSpecificSettings* clone = PlayerSpecificSettings::New_ctor();
    clone->adaptiveSfx = settings->adaptiveSfx;
    clone->advancedHud = settings->advancedHud;
    clone->automaticPlayerHeight = settings->automaticPlayerHeight;
    clone->autoRestart = settings->autoRestart;
    clone->hideNoteSpawnEffect = settings->hideNoteSpawnEffect;
    clone->leftHanded = settings->leftHanded;
    clone->noFailEffects = settings->noFailEffects;
    clone->noteJumpStartBeatOffset = settings->noteJumpStartBeatOffset;
    clone->noTextsAndHuds = settings->noTextsAndHuds;
    clone->playerHeight = settings->playerHeight;
    clone->reduceDebris = settings->reduceDebris;
    clone->saberTrailIntensity = settings->saberTrailIntensity;
    clone->sfxVolume = settings->sfxVolume;
    clone->staticLights = settings->staticLights;
    return clone;
}

// Very large hook called when a level is starting
MAKE_HOOK_OFFSETLESS(StandardLevelStart, void, StandardLevelScenesTransitionSetupDataSO* self, Il2CppString* gameMode,
                    IDifficultyBeatmap* difficultyBeatmap, OverrideEnvironmentSettings* overrideEnvironmentSettings,
                    ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers,
                    PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings,
                    Il2CppString* backButtonText, bool useTestNoteCutSoundEffects, 
                    Il2CppObject* beforeSceneSwitchCallback, Il2CppObject* afterSceneSwitchCallback, Il2CppObject* levelFinishedCallback)    {    
    // If we need to override the setting, store the previous setting and then override it
    if(willOverride)    {
        getLogger().info("Overriding setting on level start . . .");
        playerSpecificSettings = cloneSettings(playerSpecificSettings);
        playerSpecificSettings->reduceDebris = overrideMode;
    }

    StandardLevelStart(self, gameMode, difficultyBeatmap, overrideEnvironmentSettings, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, backButtonText, useTestNoteCutSoundEffects, beforeSceneSwitchCallback, afterSceneSwitchCallback, levelFinishedCallback);
}

static BeatmapLevelsModel* beatmapLevelsModel = nullptr;
// Finds the BeatmapLevelsModel, if it hasn't been found already
BeatmapLevelsModel* getBeatmapLevelsModel() {
    // If the model hasn't already been found, find it using Resources
    if(!beatmapLevelsModel) {
        beatmapLevelsModel = (*UnityEngine::Resources::FindObjectsOfTypeAll<BeatmapLevelsModel*>())[0];
    }
    return beatmapLevelsModel;
}

// Called whenever the user selects a difficulty of a level. Used to decide whether or not to override the debris setting
MAKE_HOOK_OFFSETLESS(RefreshContent, void, StandardLevelDetailView* self)    {
    RefreshContent(self);
    
    // Find the selected difficulty
    IDifficultyBeatmap* difficulty = self->selectedDifficultyBeatmap;
    IBeatmapLevel* level = difficulty->get_level();

    // If the NPS threshold is enabled
    float npsThreshold = getConfig().config["notesPerSecondThreshold"].GetFloat();
    if(npsThreshold > 0)    {
        getLogger().info("Checking NPS threshold . . .");

        // Find the length of the audio for the song
        IBeatmapLevelData* levelData = level->get_beatmapLevelData();
        UnityEngine::AudioClip* audioClip = levelData->get_audioClip();
        float songLength = audioClip->get_length();

        // Find the number of notes
        BeatmapData* beatmapData = difficulty->get_beatmapData();
        int notesCount = beatmapData->get_cuttableNotesType();

        // Find the notes per second
        float nps = notesCount / songLength;

        // We either check if the NPS is greater or lesser depending on the mode
        if((overrideMode == Mode::ENABLE && nps > npsThreshold) || (overrideMode == Mode::DISABLE && nps < npsThreshold))   {
            getLogger().info("NPS below or above threshold, overriding . . .");
            willOverride = true;
            return;
        }
    }

    // Check the song's playlist to see if we need to override
    getLogger().info("Checking song playlist . . .");

    // Reinterpret this level as an IPreviewBeatmapLevel, then find the level pack it is in
    IPreviewBeatmapLevel* previewLevel = reinterpret_cast<IPreviewBeatmapLevel*>(level);
    IBeatmapLevelPack* levelPack = getBeatmapLevelsModel()->GetLevelPackForLevelId(previewLevel->get_levelID());
    // Find the pack name
    std::string playlistName = to_utf8(csstrtostr(levelPack->get_packName()));

    // Check if this playlist is in the overridden playlists list.
    bool isOverriddenPlaylist = false;
    for(rapidjson::Value& value : getConfig().config["playlists"].GetArray())   {
        if(value.GetString() == playlistName)   {
            isOverriddenPlaylist = true; break;
        }
    }

    // If it is, override the setting
    if(isOverriddenPlaylist) {
        getLogger().info("Playlist is set as overridden, overriding . . .");
        willOverride = true;
        return;
    }

    willOverride = false;
}

void createDefaultConfig()  {
    ConfigDocument& config = getConfig().config;
    // Find if the config has been created
    if(config.HasMember("mode")) {return;}

    // Add all the default options
    auto& alloc = config.GetAllocator();
    config.AddMember("mode", "disable", alloc);
    config.AddMember("notesPerSecondThreshold", 5.0, alloc);
    config.AddMember("playlists", rapidjson::Value(rapidjson::kArrayType), alloc);

    getConfig().Write(); // Write the config back to disk
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    createDefaultConfig(); // Create the default config file if it doesn't already exist

    // Find the mode that we're using
    std::string modeString =  getConfig().config["mode"].GetString();
    overrideMode = modeString == "enable" ? ENABLE : DISABLE;

    getLogger().info("Completed setup!");
}

extern "C" void load() {
    getLogger().info("Installing hooks...");
    il2cpp_functions::Init();

    // Register our custom ViewController type
    custom_types::Register::RegisterType<AutoDebrisViewController>();

    // Register our mod settings menu
    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController<AutoDebrisViewController*>(modInfo);

    // Install our hooks
    INSTALL_HOOK_OFFSETLESS(RefreshContent, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailView", "RefreshContent", 0));
    INSTALL_HOOK_OFFSETLESS(StandardLevelStart, il2cpp_utils::FindMethodUnsafe("", "MenuTransitionsHelper", "StartStandardLevel", 12));

    getLogger().info("Installed all hooks!");
}