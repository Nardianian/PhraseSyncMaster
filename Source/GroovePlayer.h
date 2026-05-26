/*
  ==============================================================================

    GroovePlayer.h
    Created: 3 Sep 2025 10:44:30am
    Author:  evan

  ==============================================================================
*/

#pragma once
#include <sstream>
#include <iomanip>
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GrooveActionMessage.h"
#include "GrooveTransport.h"

using namespace juce;

class PhraseSyncMasterAudioProcessor; // Forward declaration to make new class known

class GroovePlayer : public Component, public ActionListener, public ActionBroadcaster, public Button::Listener {
public:
    GroovePlayer(PhraseSyncMasterAudioProcessor&); 
    ~GroovePlayer();

    void setGrooveMidiFile(File);

private:
    PhraseSyncMasterAudioProcessor& mAudioProcessor;

    Array<String> mButtonsText{ "Start", "-Meas", "Stop", "Pause", "Play", "+Meas" , "End"};

    const int mButtonWidth{ 60 };
    const int mButtonHeight{ 32 };
    const int mButtonMargin{ 2 };

    Array<std::shared_ptr<TextButton>> mButtons;

    void paint(Graphics& g) override;
    void resized() override;

    void actionListenerCallback(const String&);
    void buttonClicked(Button* button) override;
};
