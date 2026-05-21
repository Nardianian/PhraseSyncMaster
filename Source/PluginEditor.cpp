/*
  ==============================================================================

    PluginEditor.cpp
    Created: May 2026
    Author: PhraseSync Team

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PhraseSyncMasterAudioProcessorEditor::PhraseSyncMasterAudioProcessorEditor(PhraseSyncMasterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Custom graphic style
    setLookAndFeel(&customLookAndFeel);

    //--------------------------------------------------------------------------
    // CONFIGURATION: MODULE 1 - NOTE FILTER
    //--------------------------------------------------------------------------
    addAndMakeVisible(noteFilterGroup);

    addAndMakeVisible(nfBypassButton);
    nfBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "NF_BYPASS", nfBypassButton);

    addAndMakeVisible(nfVariationSlider);
    nfVariationSlider.setRange(1.0, 4.0, 1.0);
    nfVariationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "NF_VARIATION", nfVariationSlider);

    addAndMakeVisible(nfHeightMenu);
    nfHeightMenu.addItem("1/2 Octave (6st)", 1);
    nfHeightMenu.addItem("1 Octave (12st)", 2);
    nfHeightMenu.addItem("2 Octaves (24st)", 3);
    nfHeightMenu.addItem("3 Octaves (36st)", 4);
    nfHeightAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "NF_HEIGHT", nfHeightMenu);

    addAndMakeVisible(nfVariationLabel);
    nfVariationLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nfHeightLabel);
    nfHeightLabel.setJustificationType(juce::Justification::centred);

    //--------------------------------------------------------------------------
    // CONFIGURATION: MODULE 2 - ARPEGGIATOR
    //--------------------------------------------------------------------------
    addAndMakeVisible(arpeggiatorGroup);

    addAndMakeVisible(arpBypassButton);
    arpBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "ARP_BYPASS", arpBypassButton);

    addAndMakeVisible(arpRateMenu);
    arpRateMenu.addItem("1/4 (Quarter)", 1);
    arpRateMenu.addItem("1/4 T", 2);
    arpRateMenu.addItem("1/8 (Octave)", 3);
    arpRateMenu.addItem("1/8 T", 4);
    arpRateMenu.addItem("1/16 (Semi)", 5);
    arpRateMenu.addItem("1/16 T", 6);
    arpRateMenu.addItem("1/32", 7);
    arpRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "ARP_RATE", arpRateMenu);

    addAndMakeVisible(arpNoChordMenu);
    arpNoChordMenu.addItem("Silence", 1);
    arpNoChordMenu.addItem("Play Notes As Is", 2);
    arpNoChordMenu.addItem("Latch Last Chord", 3);
    arpNoChordAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "ARP_NO_CHORD", arpNoChordMenu);

    addAndMakeVisible(arpSingleNoteMenu);
    arpSingleNoteMenu.addItem("Silence", 1);
    arpSingleNoteMenu.addItem("Don't Arpeggiate", 2);
    arpSingleNoteMenu.addItem("Play Root Only", 3);
    arpSingleNoteMenu.addItem("Powerchord (Root+5th)", 4);
    arpSingleNoteMenu.addItem("Transpose Last Chord", 5);
    arpSingleNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "ARP_SINGLE_NOTE", arpSingleNoteMenu);

    addAndMakeVisible(arpRateLabel);
    arpRateLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(arpNoChordLabel);
    arpNoChordLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(arpSingleNoteLabel);
    arpSingleNoteLabel.setJustificationType(juce::Justification::centred);

    //--------------------------------------------------------------------------
    // CONFIGURATION: MODULE 3 - LINE TOGGLER
    //--------------------------------------------------------------------------
    addAndMakeVisible(lineTogglerGroup);

    addAndMakeVisible(ltBypassButton);
    ltBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "LT_BYPASS", ltBypassButton);

    addAndMakeVisible(ltChannelMenu);
    for (int i = 1; i <= 16; ++i)
        ltChannelMenu.addItem("MIDI Channel " + juce::String(i), i);
    ltChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "LT_CHANNEL", ltChannelMenu);

    addAndMakeVisible(ltChannelLabel);
    ltChannelLabel.setJustificationType(juce::Justification::centred);

    //--------------------------------------------------------------------------
    // CONFIGURATION: MODULE 4 - CHANNEL FILTER
    //--------------------------------------------------------------------------
    addAndMakeVisible(channelFilterGroup);

    addAndMakeVisible(cfBypassButton);
    cfBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "CF_BYPASS", cfBypassButton);

    addAndMakeVisible(cfChannelMenu);
    for (int i = 1; i <= 16; ++i)
        cfChannelMenu.addItem("Isolate Channel " + juce::String(i), i);
    cfChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "CF_CHANNEL", cfChannelMenu);

    addAndMakeVisible(cfChannelLabel);
    cfChannelLabel.setJustificationType(juce::Justification::centred);

    //--------------------------------------------------------------------------
    // CONFIGURATION: MODULE 5 - CONTROLLER MOTION
    //--------------------------------------------------------------------------
    addAndMakeVisible(controllerMotionGroup);

    addAndMakeVisible(cmBypassButton);
    cmBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "CM_BYPASS", cmBypassButton);

    // Phrase Type Menu (Automation Geometry)
    addAndMakeVisible(cmPhraseMenu);
    cmPhraseMenu.addItem("Ramp Up", 1);
    cmPhraseMenu.addItem("Ramp Down", 2);
    cmPhraseMenu.addItem("Triangle", 3);
    cmPhraseMenu.addItem("Sine Wave", 4);
    cmPhraseMenu.addItem("Random", 5);
    cmPhraseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "CM_PHRASE", cmPhraseMenu);

    addAndMakeVisible(cmPhraseLabel);
    cmPhraseLabel.setJustificationType(juce::Justification::centred);

    // LAMBDA: Helper function to configure the 4 Targets
    auto setupTarget = [this](juce::ComboBox& menu, juce::Label& label, const juce::String& paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& attach)
        {
            addAndMakeVisible(menu);
            menu.addItem("None (Off)", 1);
            for (int i = 1; i <= 119; ++i) // Standard MIDI CC 1 to 119
                menu.addItem("CC " + juce::String(i), i + 1);

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, paramId, menu);

            addAndMakeVisible(label);
            label.setJustificationType(juce::Justification::centred);
        };

    // Initialize the 4 CC slots using the lambda
    setupTarget(cmTarget1Menu, cmTarget1Label, "CM_TARGET_1", cmTarget1Attachment);
    setupTarget(cmTarget2Menu, cmTarget2Label, "CM_TARGET_2", cmTarget2Attachment);
    setupTarget(cmTarget3Menu, cmTarget3Label, "CM_TARGET_3", cmTarget3Attachment);
    setupTarget(cmTarget4Menu, cmTarget4Label, "CM_TARGET_4", cmTarget4Attachment);

    //--------------------------------------------------------------------------
    // ENABLE PROPORTIONAL RESIZE
    //--------------------------------------------------------------------------
    setResizable(true, true);
    setResizeLimits(750, 400, 1920, 1080); 

    //--------------------------------------------------------------------------
    // MIDI LEARN BUTTON INITIALIZATION (MODULE 5)
    //--------------------------------------------------------------------------
    for (int i = 0; i < 12; ++i)
    {
        addAndMakeVisible(cmLearnButtons[i]);
        cmLearnButtons[i].setButtonText("LN");
        cmLearnButtons[i].setTooltip("Click and move an external MIDI control to map this parameter");
        cmLearnButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
        cmLearnButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        cmLearnButtons[i].onClick = [this, i]()
            {
                if (audioProcessor.isLearning() && audioProcessor.getActiveLearnSlot() == i) {
                    audioProcessor.stopMidiLearn();
                }
                else {
                    audioProcessor.startMidiLearn(i);
                }
            };
    }

    // Configuring the style of graphics and text boxes under the potentiometers
    auto setupRotaryKnob = [this](juce::Slider& knob, double min, double max, double def) {
        addAndMakeVisible(knob);
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        knob.setRange(min, max, 1.0);
        knob.setValue(def);
        };

    setupRotaryKnob(nfHeightEncoder, 1.0, 4.0, 3.0);
    // Transform the Variation into a 123px Rotary
    setupRotaryKnob(nfVariationSliderEncoder, 0.0, 127.0, 64.0);
    setupRotaryKnob(ltChannelEncoder, 1.0, 16.0, 1.0);
    setupRotaryKnob(cfIsolateEncoder, 1.0, 16.0, 1.0);

    // CRITICAL FIX: Connect Encoders directly to APVTS via native Attachments
    heightAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "NF_HEIGHT", nfHeightEncoder);
    ltChanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "LT_CHANNEL", ltChannelEncoder);
    cfChanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "CF_CHANNEL", cfIsolateEncoder);
    phraseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "CM_PHRASE", cmPhraseMenu);

    // Font sizing of modules 3 and 4 equal to that of the other modules
    ltEncoderLabel.setFont(13.0f);
    ltEncoderLabel.setJustificationType(juce::Justification::centred);
    cfEncoderLabel.setFont(13.0f);
    cfEncoderLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(ltEncoderLabel);
    addAndMakeVisible(cfEncoderLabel); 

    // Starts the GUI timer at 10Hz (checks MIDI Learn status every 100ms)
    startTimer(100);

    // Initialize the label texts
    updateTargetLabels();

    setSize(1000, 500); // Ideal starting size
}

PhraseSyncMasterAudioProcessorEditor::~PhraseSyncMasterAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
//==============================================================================
void PhraseSyncMasterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // General plugin background (Empty space between modules and borders)
    g.fillAll(juce::Colour(0xFF1A1A1A));

    auto totalWidth = getWidth();
    auto totalHeight = getHeight();

    auto padding = 8;
    auto moduleWidth = (totalWidth - (padding * 6)) / 5;
    auto moduleHeight = totalHeight - (padding * 2);

    // Base color array for the 5 modules
    juce::Colour baseColors[5] = {
        juce::Colour(0xFF3A3F44), // Module 1: Note Filter (Steel Grey)
        juce::Colour(0xFF1C2D42), // Module 2: Arpeggiator (Midnight Blue)
        juce::Colour(0xFF24382A), // Module 3: Line Toggler (Dark Olive Green)
        juce::Colour(0xFF4A321A), // Module 4: Channel Filter (Bronze Copper)
        juce::Colour(0xFF3D1C2A)  // Module 5: Controller Motion (Amaranth Magma)
    };

    // Design your custom metallic background column by column
    for (int i = 0; i < 5; ++i)
    {
        // X calculation identical to the resized() method for a millimetric alignment
        int colX = padding + (moduleWidth + padding) * i;

        juce::Rectangle<int> moduleArea(colX, padding, moduleWidth, moduleHeight);
        auto fArea = moduleArea.toFloat();

        // Creating the Brushed Metal Effect Using a Vertical Gradient
        // Zenithal light: lightest part at the top, soft reflection in the center, dark at the base
        juce::ColourGradient gradient(
            baseColors[i].brighter(0.15f), fArea.getX(), fArea.getY(),                // Start (High)
            baseColors[i].darker(0.2f), fArea.getX(), fArea.getBottom(),            // End (Low)
            false
        );

        // Adds a central highlight to simulate the reflection of curved sheet metal
        gradient.addColour(0.3, baseColors[i].brighter(0.25f));
        gradient.addColour(0.7, baseColors[i]);

        // Apply gradient to form area
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(fArea, 6.0f); // Chamfered corners for panels

        // BORDER DESIGN
        g.setColour(baseColors[i].brighter(0.4f).withAlpha(0.3f));
        g.drawRoundedRectangle(fArea, 6.0f, 1.0f);
    }
    //--------------------------------------------------------------------------
    // Automatic Version Number Printing (Bottom right)
    //--------------------------------------------------------------------------
    g.setFont(11.0f); // Small font, real hardware style
    g.setColour(juce::Colours::white.withAlpha(0.4f)); // Semi-transparent white

    juce::String versionText = "v" + juce::String(ProjectInfo::versionString);

    // Draw text in the lower right corner with a padding of 8 pixels from the edge
    g.drawText(versionText,
        totalWidth - 100 - padding,
        totalHeight - 15 - padding,
        100, 15,
        juce::Justification::bottomRight);
} 

void PhraseSyncMasterAudioProcessorEditor::resized()
{
    auto totalWidth = getWidth();
    auto totalHeight = getHeight();

    auto padding = 8;
    // Divides the horizontal space into 5 columns separated by padding
    auto moduleWidth = (totalWidth - (padding * 6)) / 5;
    auto moduleHeight = totalHeight - (padding * 2);

    auto elementGap = juce::jlimit(4, 12, totalHeight / 40);

    // Rotary Diameter Calculation
    const int targetDiameter = 123;
    const int knobXOffset = (moduleWidth - targetDiameter) / 2; // Automatic centering in the column

    //--------------------------------------------------------------------------
    // COLUMN 1: NOTE FILTER (Structural geometric shifts)
    //--------------------------------------------------------------------------
    {
        const int colX = padding;
        noteFilterGroup.setBounds(colX, padding, moduleWidth, moduleHeight);
        auto bounds = noteFilterGroup.getBounds().reduced(10);
        bounds.removeFromTop(20);

        nfBypassButton.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(elementGap);

        // Menu Octaves Higher + LN (Slot 6)
        auto heightRow = bounds.removeFromTop(24);
        cmLearnButtons[6].setBounds(heightRow.removeFromRight(30));
        heightRow.removeFromRight(4);
        nfHeightMenu.setBounds(heightRow);

        bounds.removeFromTop(elementGap);
        nfHeightLabel.setBounds(bounds.removeFromTop(16));

        // Positioning the axis of the first "Octaves" Rotary
        bounds.removeFromTop(40);
        nfHeightEncoder.setBounds(colX + knobXOffset, bounds.getY(), targetDiameter, targetDiameter + 18);

        // Make room for the Variation section
        bounds.removeFromTop(targetDiameter + 35);
        nfVariationLabel.setBounds(bounds.removeFromTop(16));

        // Positioning the "Variation" Rotary and its LN button
        int currentY = bounds.getY();
        nfVariationSliderEncoder.setBounds(colX + knobXOffset, currentY, targetDiameter, targetDiameter + 18);
        cmLearnButtons[4].setBounds(colX + moduleWidth - 40, currentY + 40, 30, 22);
    }

    //--------------------------------------------------------------------------
    // COLUMN 2: ARPEGGIATOR (the three menus with LN button alignment)
    //--------------------------------------------------------------------------
    {
        const int colX = padding + moduleWidth + padding;
        arpeggiatorGroup.setBounds(colX, padding, moduleWidth, moduleHeight);
        auto bounds = arpeggiatorGroup.getBounds().reduced(10);
        bounds.removeFromTop(20);

        arpBypassButton.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(elementGap);

        arpRateLabel.setBounds(bounds.removeFromTop(16));
        auto rateRow = bounds.removeFromTop(24);
        cmLearnButtons[5].setBounds(rateRow.removeFromRight(30));
        rateRow.removeFromRight(4);
        arpRateMenu.setBounds(rateRow);

        bounds.removeFromTop(elementGap);
        arpNoChordLabel.setBounds(bounds.removeFromTop(16));
        auto noChordRow = bounds.removeFromTop(24);
        cmLearnButtons[7].setBounds(noChordRow.removeFromRight(30));
        noChordRow.removeFromRight(4);
        arpNoChordMenu.setBounds(noChordRow);

        bounds.removeFromTop(elementGap);
        arpSingleNoteLabel.setBounds(bounds.removeFromTop(16));
        auto singleNoteRow = bounds.removeFromTop(24);
        cmLearnButtons[8].setBounds(singleNoteRow.removeFromRight(30));
        singleNoteRow.removeFromRight(4);
        arpSingleNoteMenu.setBounds(singleNoteRow);
    }

    //--------------------------------------------------------------------------
    // COLUMN 3: LINE TOGGLER (Rotary repositioned)
    //--------------------------------------------------------------------------
    {
        const int colX = padding + (moduleWidth + padding) * 2;
        lineTogglerGroup.setBounds(colX, padding, moduleWidth, moduleHeight);
        auto bounds = lineTogglerGroup.getBounds().reduced(10);
        bounds.removeFromTop(20);

        ltBypassButton.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(elementGap);

        ltChannelLabel.setBounds(bounds.removeFromTop(16));
        ltChannelMenu.setBounds(bounds.removeFromTop(24));

        // Moving the label and allocating space for the encoder 
        bounds.removeFromTop(25);
        ltEncoderLabel.setBounds(bounds.removeFromTop(16));
        bounds.removeFromTop(5);

        int currentY = bounds.getY();
        ltChannelEncoder.setBounds(colX + knobXOffset, currentY, targetDiameter, targetDiameter + 18);
        cmLearnButtons[9].setBounds(colX + moduleWidth - 40, currentY + 40, 30, 22);
    }

    //--------------------------------------------------------------------------
    // COLUMN 4: CHANNEL FILTER (Rotary and components)
    //--------------------------------------------------------------------------
    {
        const int colX = padding + (moduleWidth + padding) * 3;
        channelFilterGroup.setBounds(colX, padding, moduleWidth, moduleHeight);
        auto bounds = channelFilterGroup.getBounds().reduced(10);
        bounds.removeFromTop(20);

        cfBypassButton.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(elementGap);

        cfChannelLabel.setBounds(bounds.removeFromTop(18));
        cfChannelMenu.setBounds(bounds.removeFromTop(24));

        // Make room for the encoder
        bounds.removeFromTop(25);
        cfEncoderLabel.setBounds(bounds.removeFromTop(16));
        bounds.removeFromTop(5);

        int currentY = bounds.getY();
        cfIsolateEncoder.setBounds(colX + knobXOffset, currentY, targetDiameter, targetDiameter + 18);
        cmLearnButtons[10].setBounds(colX + moduleWidth - 40, currentY + 40, 30, 22);
    }

    //--------------------------------------------------------------------------
    // COLUMN 5: CONTROLLER MOTION (Phrase Menu with LN button insertion)
    //--------------------------------------------------------------------------
    {
        const int colX = padding + (moduleWidth + padding) * 4;
        controllerMotionGroup.setBounds(colX, padding, moduleWidth, moduleHeight);
        auto bounds = controllerMotionGroup.getBounds().reduced(10);
        bounds.removeFromTop(20);

        cmBypassButton.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(elementGap);

        cmPhraseLabel.setBounds(bounds.removeFromTop(16));

        // Phrase Menu with LN button (Slot 11)
        auto phraseRow = bounds.removeFromTop(24);
        cmLearnButtons[11].setBounds(phraseRow.removeFromRight(30));
        phraseRow.removeFromRight(4);
        cmPhraseMenu.setBounds(phraseRow);

        bounds.removeFromTop(elementGap);

        // Align and create space for the four Targets
        juce::Label* labels[] = { &cmTarget1Label, &cmTarget2Label, &cmTarget3Label, &cmTarget4Label };
        juce::ComboBox* menus[] = { &cmTarget1Menu, &cmTarget2Menu, &cmTarget3Menu, &cmTarget4Menu };

        for (int i = 0; i < 4; ++i)
        {
            labels[i]->setBounds(bounds.removeFromTop(14));
            auto row = bounds.removeFromTop(24);
            cmLearnButtons[i].setBounds(row.removeFromRight(30));
            row.removeFromRight(4);
            menus[i]->setBounds(row);
            bounds.removeFromTop(elementGap / 2);
        }
    }
}

//==============================================================================
// TIMER: UPDATE GRAPHICS BASED ON AUDIO ENGINE STATUS
//==============================================================================
void PhraseSyncMasterAudioProcessorEditor::timerCallback()
{
    bool isProcessorLearning = audioProcessor.isLearning();
    int activeSlot = audioProcessor.getActiveLearnSlot();

    for (int i = 0; i < 12; ++i)
    {
        if (isProcessorLearning && activeSlot == i)
        {
            cmLearnButtons[i].setButtonText("???");
            cmLearnButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
        }
        else
        {
            cmLearnButtons[i].setButtonText("LN");
            cmLearnButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
        }
    }

    // Update the labels constantly to show the current CC values
    updateTargetLabels();
}

//==============================================================================
// HELPER: UPDATE THE LABEL TEXTS WITH THE ACTUALLY ASSIGNED CC VALUES
//==============================================================================
void PhraseSyncMasterAudioProcessorEditor::updateTargetLabels()
{
    nfHeightLabel.setText("Octaves (CC " + juce::String(audioProcessor.getMappedCCForTarget(6)) + ")", juce::dontSendNotification);
    nfVariationLabel.setText("Variation (CC " + juce::String(audioProcessor.getMappedCCForTarget(4)) + ")", juce::dontSendNotification);

    arpRateLabel.setText("Rate (CC " + juce::String(audioProcessor.getMappedCCForTarget(5)) + ")", juce::dontSendNotification);
    arpNoChordLabel.setText("No Chord (CC " + juce::String(audioProcessor.getMappedCCForTarget(7)) + ")", juce::dontSendNotification);
    arpSingleNoteLabel.setText("Single Note (CC " + juce::String(audioProcessor.getMappedCCForTarget(8)) + ")", juce::dontSendNotification);
    
    cmPhraseLabel.setText("Phrase Type (CC " + juce::String(audioProcessor.getMappedCCForTarget(11)) + ")", juce::dontSendNotification);

    ltEncoderLabel.setText("Quick CC " + juce::String(audioProcessor.getMappedCCForTarget(9)), juce::dontSendNotification);
    cfEncoderLabel.setText("Quick CC " + juce::String(audioProcessor.getMappedCCForTarget(10)), juce::dontSendNotification);

    cmTarget1Label.setText("Target 1 (CC " + juce::String(audioProcessor.getMappedCCForTarget(0)) + ")", juce::dontSendNotification);
    cmTarget2Label.setText("Target 2 (CC " + juce::String(audioProcessor.getMappedCCForTarget(1)) + ")", juce::dontSendNotification);
    cmTarget3Label.setText("Target 3 (CC " + juce::String(audioProcessor.getMappedCCForTarget(2)) + ")", juce::dontSendNotification);
    cmTarget4Label.setText("Target 4 (CC " + juce::String(audioProcessor.getMappedCCForTarget(3)) + ")", juce::dontSendNotification);
} 