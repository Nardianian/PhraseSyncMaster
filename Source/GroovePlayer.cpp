/*
  ==============================================================================

    GroovePlayer.cpp
    Created: 3 Sep 2025 10:44:30am
    Author:  evan

  ==============================================================================
*/

#include "GroovePlayer.h"
#include "Log.h"
#include "PluginProcessor.h" 

GroovePlayer::GroovePlayer(PhraseSyncMasterAudioProcessor& p) : mAudioProcessor(p) 
{
    setName("GPLYR");
    setComponentID("0");

    // Add transport control buttons whose names are held in "::mButtonsText"
    for (auto& buttonText : mButtonsText) {
        auto newButton = std::make_shared<TextButton>(buttonText);

        mButtons.add(newButton);

        newButton->setSize(mButtonWidth, mButtonHeight);
        newButton->addListener(this);
        addAndMakeVisible(*newButton);
    }
  
    setSize( (mButtonWidth + mButtonMargin) * mButtons.size() + mButtonMargin, mButtonHeight + 2 * mButtonMargin);
}

GroovePlayer::~GroovePlayer() {
}

void GroovePlayer::paint(Graphics& g) {
    g.fillAll(Colour(0xffcf00cf));
}

void GroovePlayer::resized() {
    int xOffset{ mButtonMargin };

    for (int idx = 0; idx < mButtons.size(); idx++) {
        mButtons[idx]->setTopLeftPosition(xOffset, mButtonMargin);
        xOffset += mButtonWidth + mButtonMargin;
    }
}

void GroovePlayer::setGrooveMidiFile(File f) {
    mAudioProcessor.initialize(f);
}

void GroovePlayer::actionListenerCallback(const String& message) {
    GrooveActionMessage gam = json::parse(message.toStdString());

    // Ignore message if it isn't from GrooveBrowser or isn't a "NewFile" action
    if (gam["component"] != "GVBR" || gam["action"] != "NEWF")
        return;

    setGrooveMidiFile(File((std::string)gam["value"]));
}

void GroovePlayer::buttonClicked(Button* button) {
    GrooveActionMessage gam;

    gam["component"] = getName().toStdString();
    gam["index"] = getComponentID().getIntValue();
    gam["action"] = "TPTCTRL";
    gam["value"] = button->getButtonText().toStdString();

    // broadcast a Transport Control message based on the just clicked transport control button
    // Presently GrooveTransport (part of LiveMidiAudioProcessor) is the intended client.
    sendActionMessage(gam.dump());
}