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

    //--------------------------------------------------------------------------
    // MODULE 5 MIDI LEARN 
    //--------------------------------------------------------------------------

    // Rotary Encoders
    juce::Slider nfHeightEncoder;     // Module 1
    juce::Slider ltChannelEncoder;    // Module 3
    juce::Slider cfIsolateEncoder;    // Module 4
    juce::Slider nfVariationSliderEncoder; // Module 1 Rotary Variation

    // Twelve LN Buttons Array
    juce::TextButton cmLearnButtons[12];

    // Attachments to bind Encoders to the APVTS in a bidirectional way
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> heightAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ltChanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cfChanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phraseAttachment;

    // New support labels
    juce::Label ltEncoderLabel;
    juce::Label cfEncoderLabel;

    // Helper function to update label text with the actual assigned CC
    void updateTargetLabels();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraseSyncMasterAudioProcessorEditor)
};