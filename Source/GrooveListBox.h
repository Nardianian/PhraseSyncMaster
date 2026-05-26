/*
  ==============================================================================

    GrooveListBox.h
    Created: 15 Jul 2025 10:55:58am
    Author:  evan

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
using namespace juce;

#include "GrooveActionMessage.h"

#define GROOVE_LISTBOX_HEIGHT 200
#define GROOVE_LISTBOX_WIDTH 250

class GrooveListBox : public ListBoxModel, public Component, public ActionBroadcaster
{
    const int rowHeight{ 20 };
    int numRows{ 0 };

public:
    GrooveListBox();
    ~GrooveListBox();

    void clear();
    void updateContent();

    void add(String);
    int getSelectedRow() const { return mSelectedRow; }

protected:
    void paint(Graphics& g) override;
    void resized() override;
    int  getNumRows() override;
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int /*lastRowselected*/) override;

    ListBox mListBox;
    StringArray mItems;
    int mSelectedRow{ -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrooveListBox)
};
