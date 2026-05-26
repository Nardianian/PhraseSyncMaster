/*
  ==============================================================================

    GrooveBrowser.h
    Created: 22 Aug 2025 10:37:17am
    Author:  evan

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "GrooveStore.h"
#include "GrooveListBox.h"
#include "GrooveActionMessage.h"

using namespace juce;

//#define TEMP_STORE_FOLDER_NAME "E:/music/Native Instruments Content/Abbey Road 60s Drummer Library/MIDI Files"
//#define TEMP_STORE_FOLDER_NAME "E:/music/GigPerformer/Recordings/Studio/2025-08-31 at 1219"
#define TEMP_STORE_FOLDER_NAME "E:/music/Free MIDI"
#define MAX_GROOVE_LISTBOXES 6

class GrooveBrowser : public Component, public ActionListener, public ActionBroadcaster {
public:
    GrooveBrowser();
    ~GrooveBrowser();

    void Initialize(File);
    void HandleSelectionChangeAction(GrooveActionMessage&);

protected:
    void paint(Graphics& g) override;
    void resized() override;
    void actionListenerCallback(const String&) override;

    GrooveStore mStore;

    GrooveListBox mListBoxes[MAX_GROOVE_LISTBOXES];
};