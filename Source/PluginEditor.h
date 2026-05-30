/*
  ==============================================================================

    PluginEditor.h
    Created: May 2026
    Author: PhraseSync Team

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GroovePlayer.h"

//==============================================================================
// CUSTOM LOOK AND FEEL CLASS
//==============================================================================
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF1A1A1A));
        setColour(juce::Label::textColourId, juce::Colours::white);

        setColour(juce::Slider::thumbColourId, juce::Colour(0xFF00FF99));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF00AA66));
        setColour(juce::Slider::trackColourId, juce::Colour(0xFF333333));

        setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFF00FF99));
        setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);

        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2D2D2D));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF444444));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xFF00FF99));
    }
};

//==============================================================================
// BIG MENU LOOK AND FEEL CLASS
//==============================================================================
class BigMenuLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BigMenuLookAndFeel() {}

    // Force font size inside popup window
    juce::Font getPopupMenuFont() override
    {
        return juce::Font(15.0f, juce::Font::plain);
    }

    // Force the menu row to expand to accommodate the large text
    void getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
        int targetHeight, int& width, int& height) override
    {
        juce::LookAndFeel_V4::getIdealPopupMenuItemSize(text, isSeparator, targetHeight, width, height);
        height = 32; // Increased row height to 32 pixels
        width += 20; // Extra width margin
    }
};
//==============================================================================     

class PhraseSyncMasterAudioProcessorEditor : public juce::AudioProcessorEditor, 
                                             public juce::Timer
{
public:
    PhraseSyncMasterAudioProcessorEditor(PhraseSyncMasterAudioProcessor&);
    ~PhraseSyncMasterAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CustomLookAndFeel customLookAndFeel;
    BigMenuLookAndFeel bigMenuLookAndFeel;
    PhraseSyncMasterAudioProcessor& audioProcessor;

    //--------------------------------------------------------------------------
    // GRAPHIC COMPONENTS: MODULE 1 - NOTE FILTER
    //--------------------------------------------------------------------------
    juce::GroupComponent noteFilterGroup{ "group_nf", "1. NOTE FILTER" };
    juce::ToggleButton   nfBypassButton{ "Bypass" };
    juce::Slider         nfVariationSlider{ juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::ComboBox       nfHeightMenu;
    juce::Label          nfVariationLabel{ "lbl_nf_var", "Variation" };
    juce::Label          nfHeightLabel{ "lbl_nf_hgt", "Window Height" };

    //--------------------------------------------------------------------------
    // GRAPHIC COMPONENTS: MODULE 2 - ARPEGGIATOR
    //--------------------------------------------------------------------------
    juce::GroupComponent arpeggiatorGroup{ "group_arp", "2. ARPEGGIATOR" };
    juce::ToggleButton   arpBypassButton{ "Bypass" };
    juce::ComboBox       arpRateMenu;
    juce::ComboBox       arpNoChordMenu;
    juce::ComboBox       arpSingleNoteMenu;
    juce::Label          arpRateLabel{ "lbl_arp_rt",  "Arp Rate" };
    juce::Label          arpNoChordLabel{ "lbl_arp_nc",  "When No Chord" };
    juce::Label          arpSingleNoteLabel{ "lbl_arp_sn",  "When Single Note" };
    juce::ComboBox       arpChannelMenu;
    juce::Label          arpChannelLabel{ "lbl_arp_ch", "Output Channel" };

    //--------------------------------------------------------------------------
    // GRAPHIC COMPONENTS: MODULE 3 - LINE TOGGLER
    //--------------------------------------------------------------------------
    juce::GroupComponent lineTogglerGroup{ "group_lt", "3. LINE TOGGLER" };
    juce::ToggleButton   ltBypassButton{ "Bypass" };
    juce::ComboBox       ltChannelMenu;
    juce::Label          ltChannelLabel{ "lbl_lt_ch", "Target Channel" };

    //--------------------------------------------------------------------------
    // GRAPHIC COMPONENTS: MODULE 4 - CHANNEL FILTER
    //--------------------------------------------------------------------------
    juce::GroupComponent channelFilterGroup{ "group_cf", "4. CHAN FILTER" };
    juce::ToggleButton   cfBypassButton{ "Bypass" };
    juce::ComboBox       cfChannelMenu;
    juce::Label          cfChannelLabel{ "lbl_cf_ch", "Filter Channel" };

    //--------------------------------------------------------------------------
    // GRAPHIC COMPONENTS: MODULE 5 - CONTROLLER MOTION
    //--------------------------------------------------------------------------
    juce::GroupComponent controllerMotionGroup{ "group_cm", "5. CTRL MOTION" };
    juce::ToggleButton   cmBypassButton{ "Bypass" };

    juce::ComboBox       cmPhraseMenu;
    juce::ComboBox       cmTarget1Menu;
    juce::ComboBox       cmTarget2Menu;
    juce::ComboBox       cmTarget3Menu;
    juce::ComboBox       cmTarget4Menu;

    juce::Label          cmPhraseLabel{ "lbl_cm_ph", "Phrase Type" };
    juce::Label          cmTarget1Label{ "lbl_cm_t1", "Target 1 (CC)" };
    juce::Label          cmTarget2Label{ "lbl_cm_t2", "Target 2 (CC)" };
    juce::Label          cmTarget3Label{ "lbl_cm_t3", "Target 3 (CC)" };
    juce::Label          cmTarget4Label{ "lbl_cm_t4", "Target 4 (CC)" };

    juce::ComboBox       cmLengthMenu;
    juce::Label          cmLengthLabel{ "lbl_cm_ln", "Phrase Length" };

    //--------------------------------------------------------------------------
    // GRAPHIC COMPONENTS: INPUT ROUTING 
    //--------------------------------------------------------------------------
    juce::ComboBox nfInputBox;
    juce::ComboBox arpInputBox;
    juce::ComboBox ltInputBox;
    juce::ComboBox cfInputBox;
    juce::ComboBox cmInputBox;

    // "Input" labels for the midi input channels of the first 5 modules
    juce::Label nfInLabel;
    juce::Label arpInLabel;
    juce::Label ltInLabel;
    juce::Label cfInLabel;
    juce::Label cmInLabel;

    //--------------------------------------------------------------------------
    // ATTACHMENTS FOR CONNECTION TO THE APVTS
    //--------------------------------------------------------------------------
    // Module 1
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   nfBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   nfVariationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> nfHeightAttachment;

    // Module 2
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   arpBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpNoChordAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpSingleNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpChannelAttachment;

    // Module 3
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   ltBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ltChannelAttachment;

    // Module 4
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   cfBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cfChannelAttachment;

    // Module 5
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   cmBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cmPhraseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cmTarget1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cmTarget2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cmTarget3Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cmTarget4Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cmLengthAttachment;

    // Input Routing Attachments 
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> nfInputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpInputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ltInputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cfInputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cmInputAttachment;

    //--------------------------------------------------------------------------
    // MODULE 5 MIDI LEARN 
    //--------------------------------------------------------------------------

    // Rotary Encoders
    juce::Slider ltChannelEncoder;    // Module 3
    juce::Slider cfIsolateEncoder;    // Module 4

    // Twelve LN Buttons Array
    juce::TextButton cmLearnButtons[12];

    // Buttons to reset the MIDI Learn of each module
    juce::TextButton resetModuleLearnButtons[5];

    // Attachments to bind Encoders to the APVTS in a bidirectional way
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ltChanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cfChanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phraseAttachment;

    // New support labels
    juce::Label ltEncoderLabel;
    juce::Label cfEncoderLabel;

    private:
        // --- LIVE MIDI COMPONENTS ---
        juce::GroupComponent liveMidiGroup{ "liveMidiGroup", "LIVE MIDI GROOVE TRANSLATOR" };
        juce::ToggleButton bypassButton{ "Bypass LiveMidi" };
        juce::TextButton loadMidiButton{ "Load MIDI File..." };
        std::unique_ptr<juce::FileChooser> midiFileChooser;
        juce::ComboBox routingComboBox;
        juce::Label routingLabel{ "RoutingLabel", "Routing:" };

        // Array for the 16 mute buttons
        juce::ToggleButton channelMuteButtons[16];

        // --- LIVE MIDI TRANSPORT CONTROL COMPONENT ---
        GroovePlayer groovePlayer{ audioProcessor };

        // --- ATTACHMENTS APVTS ---
        using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
        using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

        std::unique_ptr<ButtonAttachment> bypassAttachment;
        std::unique_ptr<ComboBoxAttachment> routingAttachment;

        // Array for the attachments of the 16 channels
        std::unique_ptr<ButtonAttachment> channelMuteAttachments[16];  

    // Helper function to update label text with the actual assigned CC
    void updateTargetLabels();

    // Helper to handle left-clicking on MIDI Learn buttons
    void handleLearnButtonClick(int buttonIndex, const juce::ModifierKeys& modifiers);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraseSyncMasterAudioProcessorEditor)
};
