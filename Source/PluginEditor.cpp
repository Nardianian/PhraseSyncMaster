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

    // Configure and make the Variation slider visible
    addAndMakeVisible(nfVariationSlider);

    // Make the encoder horizontal
    nfVariationSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    nfVariationSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);

    // Create the vital attachment!
    // (No need for setRange, the Attachment automatically takes the 1-16 range from the Processor)
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
    addAndMakeVisible(arpChannelMenu);
    for (int i = 1; i <= 16; ++i)
        arpChannelMenu.addItem("MIDI Channel " + juce::String(i), i);
    arpChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "ARP_CHANNEL", arpChannelMenu);

    addAndMakeVisible(arpChannelLabel);
    arpChannelLabel.setJustificationType(juce::Justification::centred);

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
        audioProcessor.parameters, "CM_SHAPE", cmPhraseMenu);

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

    // Phrase Length Menu (Time Duration)
    addAndMakeVisible(cmLengthMenu);
    cmLengthMenu.addItem("1 Beat", 1);
    cmLengthMenu.addItem("2 Beats", 2);
    cmLengthMenu.addItem("4 Beats", 3);
    cmLengthMenu.addItem("8 Beats", 4);
    cmLengthMenu.addItem("16 Beats", 5);
    cmLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "CM_PHRASE", cmLengthMenu);

    addAndMakeVisible(cmLengthLabel);
    cmLengthLabel.setJustificationType(juce::Justification::centred);

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
                auto modifiers = juce::ModifierKeys::getCurrentModifiers();
                handleLearnButtonClick(i, modifiers);
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

    setupRotaryKnob(ltChannelEncoder, 1.0, 16.0, 1.0); 
    setupRotaryKnob(cfIsolateEncoder, 1.0, 16.0, 1.0);

    // CRITICAL FIX: Connect Encoders directly to APVTS via native Attachments
    ltChanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "LT_CHANNEL", ltChannelEncoder);
    cfChanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "CF_CHANNEL", cfIsolateEncoder);

    // Font sizing of modules 3 and 4 equal to that of the other modules
    ltEncoderLabel.setFont(13.0f);
    ltEncoderLabel.setJustificationType(juce::Justification::centred);
    cfEncoderLabel.setFont(13.0f);
    cfEncoderLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(ltEncoderLabel);
    addAndMakeVisible(cfEncoderLabel); 

    //--------------------------------------------------------------------------
    // CONFIGURATION: MODULE 6 - LIVE MIDI (GROOVE TRANSLATOR)
    //--------------------------------------------------------------------------
    // 1. ComboBox and Text Setup
    routingComboBox.addItem("Pre-FX", 1);
    routingComboBox.addItem("Post-FX", 2);
    routingComboBox.setJustificationType(juce::Justification::centred);
    routingComboBox.setLookAndFeel(&bigMenuLookAndFeel);

    bypassButton.setButtonText("Bypass LiveMidi"); 

    // 2. Makes the main controls visible
    addAndMakeVisible(liveMidiGroup);
    addAndMakeVisible(bypassButton);
    addAndMakeVisible(routingComboBox);
    addAndMakeVisible(routingLabel);
    routingLabel.attachToComponent(&routingComboBox, true);

    // 3.Connect the main controls to the APVTS
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "LIVEMIDI_BYPASS", bypassButton);
    routingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "LIVEMIDI_ROUTING", routingComboBox);    

    // 3b. Initialize and register the internal Transport Controller
    addAndMakeVisible(groovePlayer);
    audioProcessor.getGrooveTransport().addActionListener(&groovePlayer);
    groovePlayer.addActionListener(&audioProcessor.getGrooveTransport());

    // 4. Initialize, make visible, and connect the 16 mute channels
    for (int i = 0; i < 16; ++i)
    {
        juce::String chNumber = juce::String(i + 1);
        channelMuteButtons[i].setButtonText("Mute Ch " + chNumber);

        // Colors in style with the look & feel
        channelMuteButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
        channelMuteButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        addAndMakeVisible(channelMuteButtons[i]);

        juce::String paramId = "LIVEMIDI_MUTE_CH_" + chNumber;
        channelMuteAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.parameters, paramId, channelMuteButtons[i]);
    }  

    // Initialize the label texts
    updateTargetLabels();

    // Initialize the 5 reset buttons for the modules
    for (int i = 0; i < 5; ++i)
    {
        addAndMakeVisible(resetModuleLearnButtons[i]);
        resetModuleLearnButtons[i].setButtonText("Reset MIDI");
        resetModuleLearnButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF552222));
        resetModuleLearnButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }

    // LAMBDA for the reset button clicks linked to the modules
    resetModuleLearnButtons[0].onClick = [this]() { audioProcessor.unlearnMidi(6); audioProcessor.unlearnMidi(4); }; // Module 1
    resetModuleLearnButtons[1].onClick = [this]() { audioProcessor.unlearnMidi(5); audioProcessor.unlearnMidi(7); audioProcessor.unlearnMidi(8); }; // Module 2
    resetModuleLearnButtons[2].onClick = [this]() { audioProcessor.unlearnMidi(9); };  // Module 3
    resetModuleLearnButtons[3].onClick = [this]() { audioProcessor.unlearnMidi(10); }; // Module 4
    resetModuleLearnButtons[4].onClick = [this]() { audioProcessor.unlearnMidi(11); audioProcessor.unlearnMidi(0); audioProcessor.unlearnMidi(1); audioProcessor.unlearnMidi(2); audioProcessor.unlearnMidi(3); }; // Module 5

    // Vertically enlarge the window
    setSize(1000, 530); 

    // Starts the 20Hz timer (updates every 50 milliseconds) for polling MIDI Learn status
    startTimerHz(20);
}

PhraseSyncMasterAudioProcessorEditor::~PhraseSyncMasterAudioProcessorEditor()
{
    routingComboBox.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}  

//==============================================================================

void PhraseSyncMasterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // General plugin background
    g.fillAll(juce::Colour(0xFF1A1A1A));

    auto totalWidth = getWidth();
    auto totalHeight = getHeight();

    auto padding = 8;
    auto moduleWidth = (totalWidth - (padding * 6)) / 5;

    // LiveMidi panel height and dynamic calculation of the background of the upper modules
    int liveMidiPanelHeight = 175;
    auto moduleHeight = totalHeight - liveMidiPanelHeight - padding;

    // Base color array for the 5 modules + LiveMidi
    juce::Colour baseColors[6] = {
        juce::Colour(0xFF3A3F44), // Module 1: Note Filter
        juce::Colour(0xFF1C2D42), // Module 2: Arpeggiator
        juce::Colour(0xFF24382A), // Module 3: Line Toggler
        juce::Colour(0xFF4A321A), // Module 4: Channel Filter
        juce::Colour(0xFF3D1C2A), // Module 5: Controller Motion
        juce::Colour(0xFF2B2B2B)  // Module 6: Live Midi
    };

    // Lambda for drawing the panels
    auto drawPanel = [&](juce::Graphics& gr, juce::Rectangle<int> area, juce::Colour baseCol) {
        auto fArea = area.toFloat();
        juce::ColourGradient gradient(
            baseCol.brighter(0.15f), fArea.getX(), fArea.getY(),
            baseCol.darker(0.2f), fArea.getX(), fArea.getBottom(),
            false
        );
        gradient.addColour(0.3, baseCol.brighter(0.25f));
        gradient.addColour(0.7, baseCol);
        gr.setGradientFill(gradient);
        gr.fillRoundedRectangle(fArea, 6.0f);
        gr.setColour(baseCol.brighter(0.4f).withAlpha(0.3f));
        gr.drawRoundedRectangle(fArea, 6.0f, 1.0f);
        };

    // Draw the 5 vertical columns
    for (int i = 0; i < 5; ++i)
    {
        int colX = padding + (moduleWidth + padding) * i;
        juce::Rectangle<int> moduleArea(colX, padding, moduleWidth, moduleHeight);
        drawPanel(g, moduleArea, baseColors[i]);
    }

    // Draw the horizontal Live Midi panel anchored at the new absolute bottom
    juce::Rectangle<int> liveMidiArea(padding, totalHeight - liveMidiPanelHeight + padding, totalWidth - (padding * 2), liveMidiPanelHeight - (padding * 2));
    drawPanel(g, liveMidiArea, baseColors[5]);

    //--------------------------------------------------------------------------
    // Automatic Version Number Printing (Bottom right)
    //--------------------------------------------------------------------------
    g.setFont(11.0f);
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    juce::String versionText = "v" + juce::String(ProjectInfo::versionString);
    g.drawText(versionText, totalWidth - 100 - padding - 12, totalHeight - 15 - padding - 12, 100, 15, juce::Justification::bottomRight);
}  
//==============================================================================

void PhraseSyncMasterAudioProcessorEditor::resized()
{
    auto padding = 8;

    // 1. HEIGHTS CALCULATION: Upper modules elongated by exactly 80px (253 -> 333)
    int topModulesHeight = 333;
    int liveMidiHeight = 175;

    int moduleWidth = (getWidth() - (padding * 6)) / 5;

    // 2. POSITIONING OF THE 5 UPPER MODULES (GROW DOWNWARD)
    {
        // Column 1: Note Filter
        noteFilterGroup.setBounds(padding, padding, moduleWidth, topModulesHeight);
        auto b = noteFilterGroup.getBounds().reduced(10); b.removeFromTop(20);
        nfBypassButton.setBounds(b.removeFromTop(24));
        b.removeFromTop(4);
        auto hRow = b.removeFromTop(24);
        cmLearnButtons[6].setBounds(hRow.removeFromRight(30)); hRow.removeFromRight(4);
        nfHeightMenu.setBounds(hRow);
        b.removeFromTop(4);
        nfVariationLabel.setBounds(b.removeFromTop(16));
        auto vRow = b.removeFromTop(24);
        cmLearnButtons[4].setBounds(vRow.removeFromRight(30)); vRow.removeFromRight(4);
        nfVariationSlider.setBounds(vRow);

        // Reset Button layout dynamic anchor on the new bottom
        resetModuleLearnButtons[0].setBounds(padding + 10, topModulesHeight - 22, moduleWidth - 20, 18);
    }

    {
        // Column 2: Arpeggiator
        int colX = padding + moduleWidth + padding;
        arpeggiatorGroup.setBounds(colX, padding, moduleWidth, topModulesHeight);
        auto b = arpeggiatorGroup.getBounds().reduced(10); b.removeFromTop(20);
        arpBypassButton.setBounds(b.removeFromTop(24));
        b.removeFromTop(2);
        arpRateLabel.setBounds(b.removeFromTop(14));
        auto rRow = b.removeFromTop(22);
        cmLearnButtons[5].setBounds(rRow.removeFromRight(30)); rRow.removeFromRight(4);
        arpRateMenu.setBounds(rRow);
        arpNoChordLabel.setBounds(b.removeFromTop(14));
        auto ncRow = b.removeFromTop(22);
        cmLearnButtons[7].setBounds(ncRow.removeFromRight(30)); ncRow.removeFromRight(4);
        arpNoChordMenu.setBounds(ncRow);
        arpSingleNoteLabel.setBounds(b.removeFromTop(14));
        auto snRow = b.removeFromTop(22);
        cmLearnButtons[8].setBounds(snRow.removeFromRight(30)); snRow.removeFromRight(4);
        arpSingleNoteMenu.setBounds(snRow);
        arpChannelLabel.setBounds(b.removeFromTop(14));
        arpChannelMenu.setBounds(b.removeFromTop(22));

        resetModuleLearnButtons[1].setBounds(colX + 10, topModulesHeight - 22, moduleWidth - 20, 18);
    }

    const int targetDiameter = 95;
    const int knobXOffset = (moduleWidth - targetDiameter) / 2;

    {
        // Column 3: Line Toggler
        int colX = padding + (moduleWidth + padding) * 2;
        lineTogglerGroup.setBounds(colX, padding, moduleWidth, topModulesHeight);
        auto b = lineTogglerGroup.getBounds().reduced(10); b.removeFromTop(20);
        ltBypassButton.setBounds(b.removeFromTop(24));
        b.removeFromTop(4);
        ltChannelLabel.setBounds(b.removeFromTop(14));
        ltChannelMenu.setBounds(b.removeFromTop(22));
        b.removeFromTop(4);
        ltEncoderLabel.setBounds(b.removeFromTop(14));
        int currentY = b.getY() + 2;
        ltChannelEncoder.setBounds(colX + knobXOffset, currentY, targetDiameter, targetDiameter + 14);
        cmLearnButtons[9].setBounds(colX + moduleWidth - 36, currentY + 20, 30, 20);

        resetModuleLearnButtons[2].setBounds(colX + 10, topModulesHeight - 22, moduleWidth - 20, 18);
    }

    {
        // Column 4: Channel Filter
        int colX = padding + (moduleWidth + padding) * 3;
        channelFilterGroup.setBounds(colX, padding, moduleWidth, topModulesHeight);
        auto b = channelFilterGroup.getBounds().reduced(10); b.removeFromTop(20);
        cfBypassButton.setBounds(b.removeFromTop(24));
        b.removeFromTop(4);
        cfChannelLabel.setBounds(b.removeFromTop(14));
        cfChannelMenu.setBounds(b.removeFromTop(22));
        b.removeFromTop(4);
        cfEncoderLabel.setBounds(b.removeFromTop(14));
        int currentY = b.getY() + 2;
        cfIsolateEncoder.setBounds(colX + knobXOffset, currentY, targetDiameter, targetDiameter + 14);
        cmLearnButtons[10].setBounds(colX + moduleWidth - 36, currentY + 20, 30, 20);

        resetModuleLearnButtons[3].setBounds(colX + 10, topModulesHeight - 22, moduleWidth - 20, 18);
    }

    {
        // Column 5: Controller Motion
        int colX = padding + (moduleWidth + padding) * 4;
        controllerMotionGroup.setBounds(colX, padding, moduleWidth, topModulesHeight);
        auto b = controllerMotionGroup.getBounds().reduced(10); b.removeFromTop(20);
        cmBypassButton.setBounds(b.removeFromTop(24));
        b.removeFromTop(2);
        cmPhraseLabel.setBounds(b.removeFromTop(12));
        auto pRow = b.removeFromTop(20);
        cmLearnButtons[11].setBounds(pRow.removeFromRight(30)); pRow.removeFromRight(4);
        cmPhraseMenu.setBounds(pRow);

        juce::Label* labels[] = { &cmTarget1Label, &cmTarget2Label, &cmTarget3Label, &cmTarget4Label };
        juce::ComboBox* menus[] = { &cmTarget1Menu, &cmTarget2Menu, &cmTarget3Menu, &cmTarget4Menu };
        for (int i = 0; i < 4; ++i)
        {
            labels[i]->setBounds(b.removeFromTop(12));
            auto row = b.removeFromTop(20);
            cmLearnButtons[i].setBounds(row.removeFromRight(30)); row.removeFromRight(4);
            menus[i]->setBounds(row);
        }

        // Space allocation check
        b.removeFromTop(12);
        cmLengthLabel.setBounds(b.removeFromTop(14));
        auto lRow = b.removeFromTop(20);
        cmLengthMenu.setBounds(lRow);

        resetModuleLearnButtons[4].setBounds(colX + 10, topModulesHeight - 22, moduleWidth - 20, 18);
    }

    // 3. MODULE 6 POSITIONING
    int liveMidiY = getHeight() - liveMidiHeight + padding;
    liveMidiGroup.setBounds(padding, liveMidiY, getWidth() - (padding * 2), liveMidiHeight - (padding * 2));

    auto liveBounds = liveMidiGroup.getBounds().reduced(12);
    liveBounds.removeFromTop(12);

    auto transportRowArea = liveBounds.removeFromBottom(36);

    auto controlsArea = liveBounds.removeFromLeft(200);
    bypassButton.setBounds(controlsArea.removeFromTop(24));
    controlsArea.removeFromTop(14);
    routingComboBox.setBounds(controlsArea.removeFromTop(28).withTrimmedLeft(65));

    liveBounds.removeFromLeft(15);

    int numCols = 8;
    int numRows = 2;
    int btnWidth = liveBounds.getWidth() / numCols;
    int btnHeight = liveBounds.getHeight() / numRows;

    for (int i = 0; i < 16; ++i)
    {
        int row = i / numCols;
        int col = i % numCols;
        int x = liveBounds.getX() + col * btnWidth;
        int y = liveBounds.getY() + row * btnHeight;
        channelMuteButtons[i].setBounds(juce::Rectangle<int>(x, y, btnWidth, btnHeight).reduced(2));
    }

    int grooveX = (getWidth() - 500) / 2;
    int grooveWidth = 500 - 65;
    int grooveHeight = 32 + 10;

    groovePlayer.setBounds(grooveX, transportRowArea.getY(), grooveWidth, grooveHeight);
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
            if (cmLearnButtons[i].getButtonText() != "???")
            {
                cmLearnButtons[i].setButtonText("???");
                cmLearnButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
            }
        }
        else
        {
            if (cmLearnButtons[i].getButtonText() != "LN")
            {
                cmLearnButtons[i].setButtonText("LN");
                cmLearnButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
                cmLearnButtons[i].setToggleState(false, juce::dontSendNotification);
            }
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

//==============================================================================
void PhraseSyncMasterAudioProcessorEditor::handleLearnButtonClick(int buttonIndex, const juce::ModifierKeys& modifiers)
{
    if (modifiers.isPopupMenu())
        return;

    const bool processorIsLearning = audioProcessor.isLearning();
    const int currentActiveSlot = audioProcessor.getActiveLearnSlot();

    if (processorIsLearning)
    {
        // If the clicked slot is the same, turn off the learn
        if (currentActiveSlot == buttonIndex)
        {
            audioProcessor.stopMidiLearn();
        }
        // If the user clicks another LN button while one is already active,
        // turn off the old one and immediately activate the new one without getting stuck
        else
        {
            audioProcessor.stopMidiLearn();
            audioProcessor.startMidiLearn(buttonIndex);
        }
    }
    else
    {
        audioProcessor.startMidiLearn(buttonIndex);
    }
}

