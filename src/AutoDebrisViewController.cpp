#include "AutoDebrisViewController.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/GameObject.hpp"

#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/IBeatmapLevelPackCollection.hpp"

#include "TMPro/TextMeshProUGUI.hpp"
using namespace TMPro;

using namespace AutoDebris;
DEFINE_CLASS(AutoDebrisViewController);

void onModeChangeButtonClick(AutoDebrisViewController* self)  {
    // Swap the current mode
    Mode newOverrideMode = swapOverrideMode();
    std::string newButtonText = "Currently " + modeToString(newOverrideMode);

    self->modeSelectButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->SetText(il2cpp_utils::createcsstr(newButtonText));
}

void onThresholdToggleChange(AutoDebrisViewController* self, bool newValue) {
    self->thresholdSetting->get_gameObject()->SetActive(newValue);

    if(!newValue)   {
        // Set the setting to -1 if the threshold was disabled
        getLogger().info("NPS threshold disabled: setting to -1.0");
        getConfig().config["notesPerSecondThreshold"].SetDouble(-1.0);
    }   else    {    
        // Set the setting back to a default of 5
        getLogger().info("NPS threshold enabled: setting to default");
        self->thresholdSetting->CurrentValue = 5.0;
        self->thresholdSetting->UpdateValue();
    }
}

// Set the NPS threshold in the config whenever the value changes
void onThresholdSettingChange(float newValue)   {
    getConfig().config["notesPerSecondThreshold"].SetDouble(newValue);
}

void onPlaylistSettingChange(Il2CppString* playlistName, bool newValue) {
    auto& allocator = getConfig().config.GetAllocator();

    // Convert the C# string into a rapidjson string value
    rapidjson::Value playlistNameRapidjson;
    playlistNameRapidjson.SetString(to_utf8(csstrtostr(playlistName)), allocator);

    // Get the array in the config that stores the playlists
    rapidjson::GenericArray playlistsArray = getConfig().config["playlists"].GetArray();
    if(newValue)   {
        playlistsArray.PushBack(playlistNameRapidjson, allocator);
    }   else    {
        // Remove the playlist if it is in the overridden playlists array
        for(auto iter = playlistsArray.Begin(); iter < playlistsArray.End(); iter++)    {
            if(*iter == playlistNameRapidjson)  {
                playlistsArray.Erase(iter);
                break;
            }
        }
    }
    
}

void AutoDebrisViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)  {
    // If this is the first time the settings menu was loaded
    if(firstActivation && addedToHierarchy) {
        UnityEngine::UI::VerticalLayoutGroup* mainLayout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(get_rectTransform());

        // Layout for allowing the user to choose the mode (enable or disable)
        UnityEngine::UI::HorizontalLayoutGroup* modeLayout = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(mainLayout->get_rectTransform());
        modeLayout->set_childAlignment(UnityEngine::TextAnchor::UpperCenter);

        modeLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>()->ApplyBackground(il2cpp_utils::createcsstr("round-rect-panel"));
        modeLayout->set_padding(UnityEngine::RectOffset::New_ctor(2, 2, 2, 2));
        UnityEngine::UI::LayoutElement* modeLayoutElement = modeLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
        modeLayoutElement->set_preferredWidth(100.0);
        modeLayoutElement->set_preferredHeight(15.0);
        QuestUI::BeatSaberUI::AddHoverHint(modeLayout->get_gameObject(), "Chooses whether you want the mod to enable reduce debris or disable reduce debris.\nClick the button to change.");

        // Add some text to explain what the toggle is for
        QuestUI::BeatSaberUI::CreateText(modeLayout->get_rectTransform(), "Choose mode");

        // Create a button to switch modes, assigning it to a function.
        this->modeSelectButton = QuestUI::BeatSaberUI::CreateUIButton(modeLayout->get_rectTransform(), "Currently " + modeToString(getOverrideMode()), [this]{onModeChangeButtonClick(this);});

        // Layout for selecting the NPS threshold
        UnityEngine::UI::HorizontalLayoutGroup* thresholdLayout = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(mainLayout->get_rectTransform());
        UnityEngine::UI::LayoutElement* thresholdLayoutElement = thresholdLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
        thresholdLayoutElement->set_preferredWidth(60.0);
        thresholdLayoutElement->set_preferredHeight(10.0);
        QuestUI::BeatSaberUI::AddHoverHint(thresholdLayout->get_gameObject(), "Notes per second threshold for when the debris setting will be overridden. If the mode is set to enable, the setting will be overriden if the NPS is above this, if set to disable, the setting will be overridden if the NPS is below this.");

        thresholdLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>()->ApplyBackground(il2cpp_utils::createcsstr("round-rect-panel"));
        thresholdLayout->set_padding(UnityEngine::RectOffset::New_ctor(2, 2, 2, 2));
        thresholdLayout->set_childAlignment(UnityEngine::TextAnchor::UpperCenter);


        double currentThresholdValue = getConfig().config["notesPerSecondThreshold"].GetDouble();
        QuestUI::BeatSaberUI::CreateToggle(thresholdLayout->get_rectTransform(), "Enable NPS threshold", true, [this](bool newValue) {onThresholdToggleChange(this, newValue);});
        this->thresholdSetting = QuestUI::BeatSaberUI::CreateIncrementSetting(thresholdLayout->get_rectTransform(), "", 1, 0.5, currentThresholdValue, onThresholdSettingChange);
        // Call the threshold toggle change function to hide the setting if neceessary
        onThresholdToggleChange(this, currentThresholdValue != -1.0);

        // Add a hover hint for the playlist settings
        UnityEngine::UI::VerticalLayoutGroup* playlistsSectionLayout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(mainLayout->get_rectTransform());
        QuestUI::BeatSaberUI::CreateText(playlistsSectionLayout->get_rectTransform(), "Playlist Settings");
        QuestUI::BeatSaberUI::AddHoverHint(playlistsSectionLayout->get_gameObject(), "Select playlists where the debris setting will always be overridden.");
        playlistsSectionLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>()->ApplyBackground(il2cpp_utils::createcsstr("round-rect-panel"));
        playlistsSectionLayout->set_padding(UnityEngine::RectOffset::New_ctor(2, 2, 2, 2));

        // Layout for the buttons to enable/disable the override on specific playlists
        // This is a grid, since one vertical list doesn't allow for enough room
        UnityEngine::UI::GridLayoutGroup* playlistsLayout = QuestUI::BeatSaberUI::CreateGridLayoutGroup(playlistsSectionLayout->get_rectTransform());
        playlistsLayout->set_constraint(UnityEngine::UI::GridLayoutGroup::Constraint::FixedColumnCount);
        playlistsLayout->set_constraintCount(4);
        playlistsLayout->set_cellSize(UnityEngine::Vector2(40.0f, 6.0f));
        playlistsLayout->set_spacing(UnityEngine::Vector2(1.0f, 0.8f));

        // Find all the loaded playlists
        Array<IBeatmapLevelPack*>* loadedPlaylists = getBeatmapLevelsModel()->get_allLoadedBeatmapLevelPackCollection()->get_beatmapLevelPacks();
        for(int i = 0; i < loadedPlaylists->Length(); i++)  {
            IBeatmapLevelPack* playlist = loadedPlaylists->values[i];
            std::string playlistName = to_utf8(csstrtostr(playlist->get_packName()));

            const int MAX_LENGTH = 1000000;
            if(playlistName.length() > MAX_LENGTH) {
                playlistName = playlistName.substr(0, MAX_LENGTH - 3) + "...";
            }

            UnityEngine::UI::HorizontalLayoutGroup* playlistRow = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(playlistsLayout->get_rectTransform());
            // Create the toggle, setting the current value to whether or not the playlist is overridden
            UnityEngine::UI::Toggle* toggle = QuestUI::BeatSaberUI::CreateToggle(playlistRow->get_rectTransform(), playlistName, isPlaylistOverridden(playlistName),
                [playlistName] (bool newValue) {onPlaylistSettingChange(il2cpp_utils::createcsstr(playlistName), newValue);}
            );

            UnityEngine::Transform* toggleParentTransform = toggle->get_transform()->GetParent();
            TextMeshProUGUI* toggleText = toggleParentTransform->get_gameObject()->GetComponentInChildren<TextMeshProUGUI*>();
            toggleText->set_overflowMode(TextOverflowModes::Ellipsis);
            toggleText->set_fontSize(2.5);
        }
    }
}

// Save the config upon closing the menu
void AutoDebrisViewController::DidDeactivate(bool removedFromHierarchy, bool systemScreenDisabling)  {
    getConfig().Write();
}