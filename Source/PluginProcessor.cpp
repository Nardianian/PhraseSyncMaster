/*
  ==============================================================================

    PluginProcessor.cpp
    Created: May 2026
    Author: PhraseSync Team

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PhraseSyncMasterAudioProcessor::PhraseSyncMasterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    parameters(*this, nullptr, "PhraseSyncParameters", createParameterLayout())
#endif
{
    // Initializing static arrays
    for (int i = 0; i < 128; ++i) {
        isChordNoteActive[i] = false;
        isLastValidNoteActive[i] = false;
        noteCounters[i] = 0;
        activeArpMappings[i] = 0;
    }
    currentChordSize = 0;
    lastValidChordSize = 0;
}

PhraseSyncMasterAudioProcessor::~PhraseSyncMasterAudioProcessor()
{
}

//==============================================================================
// BUILDING THE LAYOUT OF ALL THE SUITE PARAMETERS (APVTS)
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PhraseSyncMasterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Phrase Length Strings for Quantized Modules
    juce::StringArray phraseLengthOptions;
    phraseLengthOptions.add("1 Beat");
    phraseLengthOptions.add("2 Beats");
    phraseLengthOptions.add("4 Beats (1 Bar)");
    phraseLengthOptions.add("8 Beats (2 Bars)");
    phraseLengthOptions.add("16 Beats (4 Bars)");
    phraseLengthOptions.add("32 Beats (8 Bars)");
    phraseLengthOptions.add("64 Beats (16 Bars)");

    // MIDI note reference strings
    juce::StringArray midiNoteOptions;
    for (int i = 0; i <= 127; ++i) {
        midiNoteOptions.add(juce::String(i) + " (" + juce::MidiMessage::getMidiNoteName(i, true, true, 3) + ")");
    }

    //--------------------------------------------------------------------------
    // 0. PARAMETERS: ROUTING INPUT
    //--------------------------------------------------------------------------
    juce::StringArray inputChannelOptions{ "None", "Any", "Ch 1", "Ch 2", "Ch 3", "Ch 4", "Ch 5", "Ch 6", "Ch 7", "Ch 8", "Ch 9", "Ch 10", "Ch 11", "Ch 12", "Ch 13", "Ch 14", "Ch 15", "Ch 16" };

    params.push_back(std::make_unique<juce::AudioParameterChoice>("NF_INPUT", "NF Input Channel", inputChannelOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_INPUT", "Arp Input Channel", inputChannelOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("LT_INPUT", "LT Input Channel", inputChannelOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CF_INPUT", "CF Input Channel", inputChannelOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CM_INPUT", "CM Input Channel", inputChannelOptions, 0));

    //--------------------------------------------------------------------------
    // 1. PARAMETERS: NOTE FILTER MODULE
    //--------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("NF_BYPASS", "Note Filter Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("NF_VARIATION", "NF Variation", 1, 16, 1));

    juce::StringArray nfHeightOptions{ "6 Semitones", "1 Octave", "2 Octaves", "3 Octaves" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("NF_HEIGHT", "NF Var Height", nfHeightOptions, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("NF_PHRASE", "NF Phrase Length", phraseLengthOptions, 2));

    //--------------------------------------------------------------------------
    // 2. PARAMETERS: ARPEGGIATOR MODULE (Geometry + Clock)
    //--------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("ARP_BYPASS", "Arpeggiator Bypass", true));
    params.push_back(std::make_unique<juce::AudioParameterInt>("ARP_CHANNEL", "Arp MIDI Channel", 1, 16, 1));
    juce::StringArray arpRateOptions{ "1/4", "1/4 Triplet", "1/8", "1/8 Triplet", "1/16", "1/16 Triplet", "1/32" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_RATE", "Arp Rate (Clock)", arpRateOptions, 4));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_REF_NOTE", "Arp Reference Note", midiNoteOptions, 60));

    juce::StringArray arpNoChordOptions{ "Silence", "Use Pattern As Notes", "Latch Last Chord" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_NO_CHORD", "Arp When No Chord", arpNoChordOptions, 2));

    juce::StringArray arpSingleNoteOptions{ "Silence", "Use Pattern As Notes", "Use As Is", "Powerchord", "Transpose Last Chord" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_SINGLE_NOTE", "Arp When Single Note", arpSingleNoteOptions, 4));

    juce::StringArray arpMappingOptions{ "Always Leave Unmapped", "Semitone To Degree", "White Key To Degree" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_MAPPING", "Arp Pattern Mapping", arpMappingOptions, 1));

    juce::StringArray arpWrapOptions{ "No Wraparound", "After All Chord Degrees" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_WRAP", "Arp Octave Wraparound", arpWrapOptions, 1));

    juce::StringArray arpUnmappedOptions{ "Silence", "Use As Is", "Transpose From 1st Degree", "Play Full Chord Up To Note" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ARP_UNMAPPED", "Arp Unmapped Behaviour", arpUnmappedOptions, 0));

    //--------------------------------------------------------------------------
    // 3. PARAMETERS: LINE TOGGLER MODULE
    //--------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("LT_BYPASS", "Line Toggler Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("LT_CHANNEL", "LT Target Channel", 1, 16, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("LT_PHRASE", "LT Phrase Length", phraseLengthOptions, 2));

    //--------------------------------------------------------------------------
    // 4. PARAMETERS: CHANNEL FILTER MODULE
    //--------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("CF_BYPASS", "Channel Filter Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("CF_CHANNEL", "CF Target Channel", 1, 16, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CF_PHRASE", "CF Phrase Length", phraseLengthOptions, 2));

    //--------------------------------------------------------------------------
    // 5. PARAMETERS: CONTROLLER MOTION MODULE
    //--------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("CM_BYPASS", "Controller Motion Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CM_PHRASE", "CM Phrase Length", phraseLengthOptions, 2));
    juce::StringArray cmShapeOptions{ "Ramp Up", "Ramp Down", "Triangle", "Sine Wave", "Random Sync" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CM_SHAPE", "CM Motion Shape", cmShapeOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("CM_BASE_CC", "CM First CC Number", 1, 124, 10));
    params.push_back(std::make_unique<juce::AudioParameterInt>("CM_CHANNEL", "CM MIDI Channel", 1, 16, 1));

    // The four continuous Targets
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CM_TARGET_1", "CM Target CC 1", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CM_TARGET_2", "CM Target CC 2", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CM_TARGET_3", "CM Target CC 3", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CM_TARGET_4", "CM Target CC 4", 0.0f, 1.0f, 0.0f));

    // --- LIVE MIDI / GROOVE TRANSLATOR PARAMS ---
    params.push_back(std::make_unique<juce::AudioParameterBool>("LIVEMIDI_BYPASS", "LiveMidi Bypass", false));

    juce::StringArray routingOptions{ "Pre-FX", "Post-FX" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("LIVEMIDI_ROUTING", "LiveMidi Routing", routingOptions, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("LIVEMIDI_MASTER_MUTE", "LiveMidi Master Mute", false));

    // Mute channels (1-16)
    for (int i = 1; i <= 16; ++i)
    {
        juce::String paramId = "LIVEMIDI_MUTE_CH_" + juce::String(i);
        juce::String paramName = "Mute Ch " + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterBool>(paramId, paramName, false));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String PhraseSyncMasterAudioProcessor::getName() const { return JucePlugin_Name; }
bool PhraseSyncMasterAudioProcessor::acceptsMidi() const { return true; }
bool PhraseSyncMasterAudioProcessor::producesMidi() const { return true; }
bool PhraseSyncMasterAudioProcessor::isMidiEffect() const { return false; }
double PhraseSyncMasterAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int PhraseSyncMasterAudioProcessor::getNumPrograms() { return 1; }
int PhraseSyncMasterAudioProcessor::getCurrentProgram() { return 0; }
void PhraseSyncMasterAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String PhraseSyncMasterAudioProcessor::getProgramName(int index) { juce::ignoreUnused(index); return {}; }
void PhraseSyncMasterAudioProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void PhraseSyncMasterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    grooveTransport.prepareToPlay(sampleRate, samplesPerBlock);

    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    totalSamplesProcessed = 0;
    sampleRemainder = 0.0;
    currentArpStep = 0;

    for (int i = 0; i < 128; ++i) {
        isChordNoteActive[i] = false;
        isLastValidNoteActive[i] = false;
        noteCounters[i] = 0;
        activeArpMappings[i] = 0;
    }
    currentChordSize = 0;
    lastValidChordSize = 0;

    for (int i = 0; i < 4; ++i)
    {
        lastSentValues[i] = -1;
    }
}

void PhraseSyncMasterAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PhraseSyncMasterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

//==============================================================================
bool PhraseSyncMasterAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PhraseSyncMasterAudioProcessor::createEditor()
{
    return new PhraseSyncMasterAudioProcessorEditor(*this);
}

//==============================================================================
// SAVING AND LOADING STATE
//==============================================================================
void PhraseSyncMasterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    if (xml != nullptr)
    {
        auto* ccXml = xml->createNewChildElement("MidiLearnCCs");
        for (int i = 0; i < 17; ++i)
        {
            ccXml->setAttribute("slot_" + juce::String(i), cmTargetCCs[i].load());
        }
    }

    copyXmlToBinary(*xml, destData);
}

void PhraseSyncMasterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

            if (auto* ccXml = xmlState->getChildByName("MidiLearnCCs"))
            {
                for (int i = 0; i < 17; ++i)
                {
                    int defaultCc = (i < 4) ? (10 + i) : 0;
                    cmTargetCCs[i].store(ccXml->getIntAttribute("slot_" + juce::String(i), defaultCc));
                }
            }
        }
    }
}

//==============================================================================
// CORE OF THE MULTI-ENGINE
//==============================================================================
void PhraseSyncMasterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();

    const juce::MidiBuffer originalIncomingMidi = midiMessages;

    //--------------------------------------------------------------------------
    // STAGE 0: SYSTEM MIDI LEARN & INCOMING CC AUTOMATION MAPPING
    //--------------------------------------------------------------------------
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isController())
        {
            int ccNum = msg.getControllerNumber();
            int ccValue = msg.getControllerValue();

            // A: MIDI LEARN IS ACTIVE (Immediate capture on any of the 17 slots)
            if (isMidiLearnActive.load())
            {
                int slot = targetLearnParamIndex.load();
                if (slot >= 0 && slot < 17)
                {
                    // Anti-overlap protection system: cleans if already used
                    for (int i = 0; i < 17; ++i)
                    {
                        if (i != slot && cmTargetCCs[i].load() == ccNum) {
                            cmTargetCCs[i].store(0);
                        }
                    }

                    cmTargetCCs[slot].store(ccNum);
                    stopMidiLearn(); // Disables global state and makes the button gray again
                    break;           // Exit to process the stability of the state in the next block
                }
            }
            // B: NORMAL STATE (Parameter automation via MIDI CC)
            else
            {
                for (int slot = 0; slot <= 11; ++slot)
                {
                    if (cmTargetCCs[slot].load() == ccNum && ccNum != 0)
                    {
                        juce::RangedAudioParameter* param = nullptr;
                        if (slot == 0)      param = parameters.getParameter("CM_TARGET_1"); 
                        else if (slot == 1) param = parameters.getParameter("CM_TARGET_2"); 
                        else if (slot == 2) param = parameters.getParameter("CM_TARGET_3"); 
                        else if (slot == 3) param = parameters.getParameter("CM_TARGET_4"); 
                        // --------------------------------------
                        else if (slot == 4) param = parameters.getParameter("NF_VARIATION");
                        else if (slot == 5) param = parameters.getParameter("ARP_RATE");
                        else if (slot == 6) param = parameters.getParameter("NF_HEIGHT");
                        else if (slot == 7) param = parameters.getParameter("ARP_NO_CHORD");
                        else if (slot == 8) param = parameters.getParameter("ARP_SINGLE_NOTE");
                        else if (slot == 9) param = parameters.getParameter("LT_CHANNEL");
                        else if (slot == 10) param = parameters.getParameter("CF_CHANNEL");
                        else if (slot == 11) param = parameters.getParameter("CM_PHRASE");

                        if (param != nullptr)
                        {
                            auto& normalisableRange = param->getNormalisableRange();
                            float realValue = normalisableRange.convertFrom0to1(static_cast<float>(ccValue) / 127.0f);
                            float juceNormalizedValue = normalisableRange.convertTo0to1(realValue);

                            // ANTI-FLOOD: Update host and UI ONLY if the value has actually changed
                            if (param->getValue() != juceNormalizedValue)
                            {
                                param->setValueNotifyingHost(juceNormalizedValue);
                            }
                        }
                    }
                }
            }   
        }
    }

    //--------------------------------------------------------------------------
    // EXTRACTING RIGID TEMPORAL DATA FROM THE PLAYHEAD
    //--------------------------------------------------------------------------
    double bpm = 120.0;
    double ppqPos = 0.0;
    bool isPlaying = false;

    juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo;
    if (auto* ph = getPlayHead())
    {
        posInfo = ph->getPosition();

        if (posInfo.hasValue())
        {
            if (auto b = posInfo->getBpm())           bpm = *b;
            if (auto p = posInfo->getPpqPosition())   ppqPos = *p;
            isPlaying = posInfo->getIsPlaying();
        }
    }

    tempoBpm = bpm;
    lastPpqPosition = ppqPos;
    isDawPlaying = isPlaying;

    // Reading the bypass states of the modules
    bool nfBypass = *parameters.getRawParameterValue("NF_BYPASS") > 0.5f;
    bool arpBypass = *parameters.getRawParameterValue("ARP_BYPASS") > 0.5f;
    int arpChannel = static_cast<int>(*parameters.getRawParameterValue("ARP_CHANNEL"));
    bool ltBypass = *parameters.getRawParameterValue("LT_BYPASS") > 0.5f;
    bool cfBypass = *parameters.getRawParameterValue("CF_BYPASS") > 0.5f;
    bool cmBypass = *parameters.getRawParameterValue("CM_BYPASS") > 0.5f;

    // --- LIVE MIDI PARAMS ---
    bool liveBypass = *parameters.getRawParameterValue("LIVEMIDI_BYPASS") > 0.5f;
    bool liveRouting = *parameters.getRawParameterValue("LIVEMIDI_ROUTING") > 0.5f;
    bool liveMasterMute = *parameters.getRawParameterValue("LIVEMIDI_MASTER_MUTE") > 0.5f;

    juce::MidiBuffer liveMidiBuffer;

    if (!liveBypass && !liveMasterMute) {
        juce::MidiBuffer rawLiveMidi;
        grooveTransport.processMidi(posInfo, numSamples, rawLiveMidi);

        for (const auto metadata : rawLiveMidi) {
            auto msg = metadata.getMessage();
            int channel = msg.getChannel();

            if (channel >= 1 && channel <= 16) {
                juce::String paramId = "LIVEMIDI_MUTE_CH_" + juce::String(channel);
                bool isChannelMuted = *parameters.getRawParameterValue(paramId) > 0.5f;

                if (!isChannelMuted) {
                    liveMidiBuffer.addEvent(msg, metadata.samplePosition);
                }
            }
            else {
                liveMidiBuffer.addEvent(msg, metadata.samplePosition);
            }
        }
    }

    // Save the value in a class member variable to make it available anywhere in the process
    currentSamplesPerBeat = (currentSampleRate * 60.0) / tempoBpm;

    juce::MidiBuffer processedMidi;

    // --- LIVE MIDI: ROUTING PRE-FX ---
    // Merges the muted MIDI with the incoming MIDI
    if (!liveBypass && !liveRouting && !liveMasterMute) {
        midiMessages.addEvents(liveMidiBuffer, 0, -1, 0);
    }

    //--------------------------------------------------------------------------
    // INTERNAL MANAGEMENT OF CHORDS AND NOTE COUNTERS
    //--------------------------------------------------------------------------
    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        int noteNum = msg.getNoteNumber();

        if (noteNum >= 0 && noteNum < 128) {
            if (msg.isNoteOn()) {
                if (noteCounters[noteNum] == 0) {
                    isChordNoteActive[noteNum] = true;
                    currentChordSize++;
                }
                noteCounters[noteNum]++;
            }
            else if (msg.isNoteOff()) {
                noteCounters[noteNum]--;
                if (noteCounters[noteNum] <= 0) {
                    noteCounters[noteNum] = 0;
                    if (isChordNoteActive[noteNum]) {
                        isChordNoteActive[noteNum] = false;
                        currentChordSize--;
                    }
                }
            }
        }
    }

    if (currentChordSize > 0) {
        for (int i = 0; i < 128; ++i) {
            isLastValidNoteActive[i] = isChordNoteActive[i];
        }
        lastValidChordSize = currentChordSize;
    }

    //--------------------------------------------------------------------------
    // STAGE 1: NOTE FILTER MODULE
    //--------------------------------------------------------------------------
    int nfInputChoice = static_cast<int>(*parameters.getRawParameterValue("NF_INPUT"));
    if (!nfBypass || nfInputChoice > 0) {
        juce::MidiBuffer bufferToProcess_NF;

        // Stream 1: Main flow (if not in bypass)
        if (!nfBypass) {
            bufferToProcess_NF.addEvents(midiMessages, 0, -1, 0);
            midiMessages.clear();
        }

        // Stream 2: Parallel flow extracted from the channel input
        if (nfInputChoice > 0) {
            for (const auto metadata : originalIncomingMidi) {
                auto msg = metadata.getMessage();
                if (nfInputChoice == 1 || msg.getChannel() == (nfInputChoice - 1)) {
                    bufferToProcess_NF.addEvent(msg, metadata.samplePosition);
                }
            }
        }

        int nfVariation = static_cast<int>(*parameters.getRawParameterValue("NF_VARIATION"));
        int nfHeightMode = static_cast<int>(*parameters.getRawParameterValue("NF_HEIGHT"));

        int windowRange = 12;
        if (nfHeightMode == 0) windowRange = 6;
        else if (nfHeightMode == 2) windowRange = 24;
        else if (nfHeightMode == 3) windowRange = 36;

        int lowerBound = 60 + ((nfVariation - 1) * windowRange);
        int upperBound = lowerBound + windowRange;

        for (const auto metadata : bufferToProcess_NF) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn() || msg.isNoteOff()) {
                int nn = msg.getNoteNumber();
                if (nn >= lowerBound && nn < upperBound) {
                    int normalizedNote = 60 + (nn - lowerBound);
                    msg.setNoteNumber(normalizedNote);
                }
            }
            processedMidi.addEvent(msg, metadata.samplePosition);
        }
        midiMessages.addEvents(processedMidi, 0, -1, 0);
        processedMidi.clear();
    }

    //--------------------------------------------------------------------------
    // STAGE 2: ARPEGGIATOR MODULE
    //--------------------------------------------------------------------------
    int arpInputChoice = static_cast<int>(*parameters.getRawParameterValue("ARP_INPUT"));
    if (!arpBypass || arpInputChoice > 0) {
        int arpRateMode = static_cast<int>(*parameters.getRawParameterValue("ARP_RATE"));
        int noChordBeh = static_cast<int>(*parameters.getRawParameterValue("ARP_NO_CHORD"));
        int singleNoteBeh = static_cast<int>(*parameters.getRawParameterValue("ARP_SINGLE_NOTE"));

        double durationInBeats = 0.25;
        if (arpRateMode == 0) durationInBeats = 1.0;
        else if (arpRateMode == 1) durationInBeats = 1.0 / 3.0;
        else if (arpRateMode == 2) durationInBeats = 0.5;
        else if (arpRateMode == 3) durationInBeats = 0.5 / 3.0;
        else if (arpRateMode == 4) durationInBeats = 0.25;
        else if (arpRateMode == 5) durationInBeats = 0.25 / 3.0;
        else if (arpRateMode == 6) durationInBeats = 0.125;

        // Retrieves already calculated time variables
        double samplesPerStep = currentSamplesPerBeat * durationInBeats;

        int chordToUse[128] = { 0 };
        int chordToUseSize = 0;
        bool doProcessArp = true;
        bool silenceArp = false;

        if (currentChordSize >= 2) {
            for (int i = 0; i < 128; ++i) {
                if (isChordNoteActive[i]) chordToUse[chordToUseSize++] = i;
            }
        }
        else if (currentChordSize == 1) {
            int singleNote = -1;
            for (int i = 0; i < 128; ++i) {
                if (isChordNoteActive[i]) { singleNote = i; break; }
            }

            if (singleNoteBeh == 4 && lastValidChordSize > 0) {
                int firstLastValid = -1;
                for (int i = 0; i < 128; ++i) {
                    if (isLastValidNoteActive[i]) { firstLastValid = i; break; }
                }
                int offset = singleNote - firstLastValid;
                for (int i = 0; i < 128; ++i) {
                    if (isLastValidNoteActive[i]) {
                        int transposed = i + offset;
                        if (transposed >= 0 && transposed < 128) {
                            chordToUse[chordToUseSize++] = transposed;
                        }
                    }
                }
            }
            else if (singleNoteBeh == 3) {
                chordToUse[chordToUseSize++] = singleNote;
                if (singleNote + 7 < 128) chordToUse[chordToUseSize++] = singleNote + 7;
            }
            else if (singleNoteBeh == 2) {
                chordToUse[chordToUseSize++] = singleNote;
            }
            else if (singleNoteBeh == 1) {
                doProcessArp = false;
            }
            else {
                silenceArp = true;
            }
        }
        if (isDawPlaying && !silenceArp && doProcessArp && chordToUseSize > 0) {
            // SAVE EVENTS (Only if the main stream is not in bypass)
            if (!arpBypass) {
                for (const auto metadata : midiMessages) {
                    auto msg = metadata.getMessage();
                    if (!msg.isNoteOn() && !msg.isNoteOff()) {
                        processedMidi.addEvent(msg, metadata.samplePosition);
                    }
                }
            }
            for (int sample = 0; sample < numSamples; ++sample) {
                double absoluteSamplePos = static_cast<double>(totalSamplesProcessed) + sample;
                double stepFloat = absoluteSamplePos / samplesPerStep;
                int stepIndex = static_cast<int>(juce::int64(stepFloat) % chordToUseSize);

                if (std::floor((absoluteSamplePos - 1.0) / samplesPerStep) < std::floor(absoluteSamplePos / samplesPerStep)) {
                    int targetNote = chordToUse[stepIndex];

                    if (activeArpMappings[targetNote] == 0) { // 0 = inactive
                        for (int i = 0; i < 128; ++i) {
                            if (activeArpMappings[i] > 0) { // Use the stored channel to turn off the note
                                processedMidi.addEvent(juce::MidiMessage::noteOff(activeArpMappings[i], i, 0.0f), sample);
                                activeArpMappings[i] = 0;
                            }
                        }
                        processedMidi.addEvent(juce::MidiMessage::noteOn(arpChannel, targetNote, 0.8f), sample);
                        activeArpMappings[targetNote] = arpChannel; // Remember which channel it's on
                    }
                }
            }
            // Deletes the original main stream only if the module is active in the main chain
            if (!arpBypass) {
                midiMessages.clear();
            }
            midiMessages.addEvents(processedMidi, 0, -1, 0);
            processedMidi.clear();
        }
        else {
            // ANTI-HANG: If your DAW stops or the arp goes silent, mute the notes using the correct channel
            for (int i = 0; i < 128; ++i) {
                if (activeArpMappings[i] > 0) {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(activeArpMappings[i], i, 0.0f), 0);
                    activeArpMappings[i] = 0;
                }
            }
        }
    }
    else {
        // ANTI-HANG: If the module is bypassed at runtime
        for (int i = 0; i < 128; ++i) {
            if (activeArpMappings[i] > 0) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(activeArpMappings[i], i, 0.0f), 0);
                activeArpMappings[i] = 0;
            }
        }
    }

    // --- LIVE MIDI: ROUTING POST-FX (Injection after the Arpeggiator) ---
    if (!liveBypass && liveRouting && !liveMasterMute) {
        midiMessages.addEvents(liveMidiBuffer, 0, -1, 0);
    }

    //--------------------------------------------------------------------------
    // STAGE 3: LINE TOGGLER MODULE
    //--------------------------------------------------------------------------
    int ltInputChoice = static_cast<int>(*parameters.getRawParameterValue("LT_INPUT"));
    if (!ltBypass || ltInputChoice > 0) {
        juce::MidiBuffer bufferToProcess_LT;

        if (!ltBypass) {
            bufferToProcess_LT.addEvents(midiMessages, 0, -1, 0);
            midiMessages.clear();
        }

        if (ltInputChoice > 0) {
            for (const auto metadata : originalIncomingMidi) {
                auto msg = metadata.getMessage();
                if (ltInputChoice == 1 || msg.getChannel() == (ltInputChoice - 1)) {
                    bufferToProcess_LT.addEvent(msg, metadata.samplePosition);
                }
            }
        }

        int ltChannel = static_cast<int>(*parameters.getRawParameterValue("LT_CHANNEL"));

        for (const auto metadata : bufferToProcess_LT) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn() || msg.isNoteOff()) {
                msg.setChannel(ltChannel);
            }
            processedMidi.addEvent(msg, metadata.samplePosition);
        }
        midiMessages.addEvents(processedMidi, 0, -1, 0);
        processedMidi.clear();
    }

    //--------------------------------------------------------------------------
    // STAGE 4: CHANNEL FILTER MODULE
    //--------------------------------------------------------------------------
    int cfInputChoice = static_cast<int>(*parameters.getRawParameterValue("CF_INPUT"));
    if (!cfBypass || cfInputChoice > 0) {
        juce::MidiBuffer bufferToProcess_CF;

        if (!cfBypass) {
            bufferToProcess_CF.addEvents(midiMessages, 0, -1, 0);
            midiMessages.clear();
        }

        if (cfInputChoice > 0) {
            for (const auto metadata : originalIncomingMidi) {
                auto msg = metadata.getMessage();
                if (cfInputChoice == 1 || msg.getChannel() == (cfInputChoice - 1)) {
                    bufferToProcess_CF.addEvent(msg, metadata.samplePosition);
                }
            }
        }

        int cfChannel = static_cast<int>(*parameters.getRawParameterValue("CF_CHANNEL"));

        for (const auto metadata : bufferToProcess_CF) {
            auto msg = metadata.getMessage();
            if (msg.getChannel() == cfChannel || (!msg.isNoteOn() && !msg.isNoteOff())) {
                processedMidi.addEvent(msg, metadata.samplePosition);
            }
        }
        midiMessages.addEvents(processedMidi, 0, -1, 0);
        processedMidi.clear();
    }

    //--------------------------------------------------------------------------
    // STAGE 5: CONTROLLER MOTION MODULE
    //--------------------------------------------------------------------------
    int cmInputChoice = static_cast<int>(*parameters.getRawParameterValue("CM_INPUT"));
    if (!cmBypass || cmInputChoice > 0) {
        int cmPhraseMode = static_cast<int>(*parameters.getRawParameterValue("CM_PHRASE"));
        int cmChannel = static_cast<int>(*parameters.getRawParameterValue("CM_CHANNEL"));

        double cmPhraseBeats = 1.0;
        if (cmPhraseMode == 0) cmPhraseBeats = 1.0;
        else if (cmPhraseMode == 1) cmPhraseBeats = 2.0;
        else if (cmPhraseMode == 2) cmPhraseBeats = 4.0;
        else if (cmPhraseMode == 3) cmPhraseBeats = 8.0;
        else if (cmPhraseMode == 4) cmPhraseBeats = 16.0;
        else if (cmPhraseMode == 5) cmPhraseBeats = 32.0;
        else if (cmPhraseMode == 6) cmPhraseBeats = 64.0;

        double samplesPerPhrase = currentSamplesPerBeat * cmPhraseBeats;

        float targetAmt[4];
        targetAmt[0] = *parameters.getRawParameterValue("CM_TARGET_1");
        targetAmt[1] = *parameters.getRawParameterValue("CM_TARGET_2");
        targetAmt[2] = *parameters.getRawParameterValue("CM_TARGET_3");
        targetAmt[3] = *parameters.getRawParameterValue("CM_TARGET_4");

        int cmShape = static_cast<int>(*parameters.getRawParameterValue("CM_SHAPE"));
        for (int sample = 0; sample < numSamples; ++sample)
        {
            double absoluteSamplePos = static_cast<double>(totalSamplesProcessed) + sample;
            double phraseSamplePos = std::fmod(absoluteSamplePos, samplesPerPhrase);
            float progress = static_cast<float>(phraseSamplePos / samplesPerPhrase);

            float shapeValue = progress; // Default: Ramp Up (0)

            if (cmShape == 1)      // Ramp Down
            {
                shapeValue = 1.0f - progress;
            }
            else if (cmShape == 2) // Triangle
            {
                shapeValue = (progress < 0.5f) ? (progress * 2.0f) : (2.0f - (progress * 2.0f));
            }
            else if (cmShape == 3) // Sine Wave
            {
                shapeValue = 0.5f - 0.5f * std::cos(progress * juce::MathConstants<float>::twoPi);
            }
            else if (cmShape == 4) // Random Sync
            {
                int phraseIndex = static_cast<int>(absoluteSamplePos / samplesPerPhrase);
                unsigned int seed = static_cast<unsigned int>(phraseIndex) * 1103515245 + 12345;
                shapeValue = static_cast<float>(seed % 10001) / 10000.0f;
            }

            for (int i = 0; i < 4; ++i) {
                int ccValue = static_cast<int>(shapeValue * targetAmt[i] * 127.0f);
                ccValue = juce::jlimit(0, 127, ccValue);

                int actualCC = cmTargetCCs[i].load();

                if (ccValue != lastSentValues[i]) {
                    midiMessages.addEvent(juce::MidiMessage::controllerEvent(cmChannel, actualCC, ccValue), sample);
                    lastSentValues[i] = ccValue;
                }
            }
        }
    }

    // Incremental advancement of the global counter
    totalSamplesProcessed += numSamples;
}

//==============================================================================
// LIVE MIDI / GROOVE TRANSLATOR ENGINE
//==============================================================================
void PhraseSyncMasterAudioProcessor::initialize(const juce::File& midiFile)
{
    grooveTransport.initialize(midiFile);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhraseSyncMasterAudioProcessor();
}

