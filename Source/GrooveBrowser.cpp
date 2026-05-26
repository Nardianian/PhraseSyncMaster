/*
  ==============================================================================

    GrooveBrowser.cpp
    Created: 22 Aug 2025 10:37:17am
    Author:  evan

  ==============================================================================
*/

#include "GrooveBrowser.h"
#include "Log.h"

GrooveBrowser::GrooveBrowser()
{
    int xOffset{ 0 };
    int idx{ 0 };

    setName("GVBR");
    setComponentID("0");

    for (auto& listBox : mListBoxes) {
        addChildComponent(listBox);
        listBox.setTopLeftPosition(xOffset, 0);
        listBox.addActionListener(this);
        listBox.setName("GLBX");
        listBox.setComponentID(std::to_string(idx++));
        xOffset += GROOVE_LISTBOX_WIDTH - 1;
    }
}

GrooveBrowser::~GrooveBrowser() {
}

void GrooveBrowser::Initialize(File f) {
    mStore.Initialize(f);

    for (auto& listBox : mListBoxes) {
        listBox.clear();
        listBox.setVisible(false);
    }

    mStore.Enumerate([&](GrooveFolder& gf) {
        auto& listBox = mListBoxes[0];

        for (const auto child : gf.GetChildren()) {
            listBox.setVisible(true);
            listBox.add(child->GetSelfFile().getFileName());
        }

        for (const auto fileName : gf.GetFileNames()) {
            listBox.setVisible(true);
            listBox.add(fileName);
        }
    });

    for (auto& listBox : mListBoxes)
        listBox.updateContent();

    repaint();
}

void GrooveBrowser::paint(Graphics& g) {
    g.fillAll(Colour(0xff000000));
}

void GrooveBrowser::resized() {
    // Here's where we'll manage set of GrooveList children according to
    // GrooveStore::MaxDepth()
    for (int idx = 0; idx < MAX_GROOVE_LISTBOXES; idx++)
        mListBoxes[idx].setSize(GROOVE_LISTBOX_WIDTH, getHeight());
}

void GrooveBrowser::actionListenerCallback(const String& message) {
    GrooveActionMessage gam = json::parse(message.toStdString());

    if (gam["component"] != "GLBX" || gam["action"] != "SRC")
        return;

    HandleSelectionChangeAction(gam);
}

// Fourth pass at using the GrooveStore data for navigation.
// Using new GrooveStore::GetGrooveFolder() works great!
// Added use of GrooveStore::GetGrooveFile() when user chooses a "*.mid" File
//
// NOTE: it is assumed that JUCE calls this only with valid input. 
void GrooveBrowser::HandleSelectionChangeAction(GrooveActionMessage& inGam) {
    int boxIdx = inGam["index"];
    int rowIdx = inGam["value"];

    // Handle cases where selection occurs in higher level box
    // when lower level box(es) is(are) present.
    if (rowIdx < 0) {
        auto& box = mListBoxes[boxIdx + 1];

        box.clear();
        box.updateContent();
        box.setVisible(false);

        return;
    }

    // Build selector from chain of mListBoxes' current selected row(s)
    GrooveStore::GrooveSelector selector;
    for (int idx = 0; idx <= boxIdx; idx++)
        selector.add(mListBoxes[idx].getSelectedRow());

    // This WILL be null if user clicked on a MIDI file somewhere in the tree.
    GrooveFolder* grooveFolder = mStore.GetGrooveFolder(selector);
    if (!grooveFolder) {
        File grooveFile = mStore.GetGrooveFile(selector);
        GrooveActionMessage outGam;

        outGam["component"] = getName().toStdString();
        outGam["index"] = getComponentID().getIntValue();
        outGam["action"] = "NEWF";
        outGam["value"] = grooveFile.getFullPathName().toStdString();

        sendActionMessage(outGam.dump());

        return;
    }

    // If we're still here, user clicked on a folder instead
    grooveFolder->Enumerate([&](GrooveFolder& gf) {
        // find the next lower GrooveListBox 
        auto& box = mListBoxes[boxIdx + 1];

        // Clear it, and update its contents.  (update IS needed here)
        box.clear();
        box.updateContent();

        // start with sub folders (children)
        for (const auto& name : gf.GetSubdirNames())
            box.add(name);

        // add *.mid files
        for (const auto& name : gf.GetFileNames())
            box.add(name);

        // refresh the GrooveBoxList and make it visible (possibly for 1st time)
        box.updateContent();
        box.setVisible(true);
    });
}
