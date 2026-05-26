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
    currentChordNotes.clear();
    lastValidChordNotes.clear();
    noteCounters.clear();
    activeArpMappings.clear();
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

    noteCounters.clear();
    currentChordNotes.clear();
    lastValidChordNotes.clear();
    activeArpMappings.clear();

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

        if (msg.isNoteOn()) {
            int currentCount = noteCounters[noteNum];
            noteCounters.set(noteNum, currentCount + 1);
            currentChordNotes.add(noteNum);
        }
        else if (msg.isNoteOff()) {
            int currentCount = noteCounters[noteNum] - 1;
            if (currentCount <= 0) {
                noteCounters.remove(noteNum);
                currentChordNotes.remove(noteNum);
            }
            else {
                noteCounters.set(noteNum, currentCount);
            }
        }
    }

    if (currentChordNotes.size() > 0) {
        lastValidChordNotes = currentChordNotes;
    }

    //--------------------------------------------------------------------------
    // STAGE 1: NOTE FILTER MODULE
    //--------------------------------------------------------------------------
    if (!nfBypass) {
        int nfVariation = static_cast<int>(*parameters.getRawParameterValue("NF_VARIATION"));
        int nfHeightMode = static_cast<int>(*parameters.getRawParameterValue("NF_HEIGHT"));

        int windowRange = 12;
        if (nfHeightMode == 0) windowRange = 6;
        else if (nfHeightMode == 2) windowRange = 24;
        else if (nfHeightMode == 3) windowRange = 36;

        int lowerBound = 60 + ((nfVariation - 1) * windowRange);
        int upperBound = lowerBound + windowRange;

        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn() || msg.isNoteOff()) {
                int nn = msg.getNoteNumber();
                if (nn >= lowerBound && nn < upperBound) {
                    int normalizedNote = 60 + (nn - lowerBound);
                    msg.setNoteNumber(normalizedNote);
                    processedMidi.addEvent(msg, metadata.samplePosition);
                }
            }
            else {
                processedMidi.addEvent(msg, metadata.samplePosition);
            }
        }
        midiMessages.clear();
        midiMessages.addEvents(processedMidi, 0, -1, 0);
        processedMidi.clear();
    }

    //--------------------------------------------------------------------------
    // STAGE 2: ARPEGGIATOR MODULE
    //--------------------------------------------------------------------------
    if (!arpBypass) {
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

        juce::SortedSet<int> chordToUse;
        bool doProcessArp = true;
        bool silenceArp = false;

        if (currentChordNotes.size() >= 2) {
            chordToUse = currentChordNotes;
        }
        else if (currentChordNotes.size() == 1) {
            if (singleNoteBeh == 4 && lastValidChordNotes.size() > 0) {
                int offset = currentChordNotes[0] - lastValidChordNotes[0];
                for (int n : lastValidChordNotes) chordToUse.add(n + offset);
            }
            else if (singleNoteBeh == 3) {
                chordToUse.add(currentChordNotes[0]);
                chordToUse.add(currentChordNotes[0] + 7);
            }
            else if (singleNoteBeh == 2) {
                chordToUse.add(currentChordNotes[0]);
            }
            else if (singleNoteBeh == 1) {
                doProcessArp = false;
            }
            else {
                silenceArp = true;
            }
        }
        else {
            if (noChordBeh == 2 && lastValidChordNotes.size() > 0) {
                chordToUse = lastValidChordNotes;
            }
            else if (noChordBeh == 1) {
                doProcessArp = false;
            }
            else {
                silenceArp = true;
            }
        }

        if (isDawPlaying && !silenceArp && doProcessArp && chordToUse.size() > 0) {
            for (int sample = 0; sample < numSamples; ++sample) {
                double absoluteSamplePos = static_cast<double>(totalSamplesProcessed) + sample;
                double stepFloat = absoluteSamplePos / samplesPerStep;
                int stepIndex = static_cast<int>(juce::int64(stepFloat) % chordToUse.size());

                if (std::floor((absoluteSamplePos - 1.0) / samplesPerStep) < std::floor(absoluteSamplePos / samplesPerStep)) {
                    int targetNote = chordToUse[stepIndex];

                    // If the note is different from the currently active one, 
                        // turn off the previous one and turn on the new one
                    if (!activeArpMappings.contains(targetNote)) {
                        juce::HashMap<int, int>::Iterator it(activeArpMappings);
                        while (it.next()) {
                            processedMidi.addEvent(juce::MidiMessage::noteOff(1, it.getKey(), 0.0f), sample);
                        }
                        activeArpMappings.clear();

                        processedMidi.addEvent(juce::MidiMessage::noteOn(1, targetNote, 0.8f), sample);
                        activeArpMappings.set(targetNote, targetNote);
                    }
                }
            }
            midiMessages.clear();
            midiMessages.addEvents(processedMidi, 0, -1, 0);
            processedMidi.clear();
        }
        else {
            // ANTI-HANG: If the DAW stops or the arp goes silent, it turns off any remaining open noteOns
            juce::HashMap<int, int>::Iterator it(activeArpMappings);
            while (it.next()) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, it.getKey(), 0.0f), 0);
            }
            activeArpMappings.clear();
        }
    }
    else {
        // ANTI-HANG: If Arpeggiator module is bypassed at runtime, it clears any residues to avoid stuck notes
        if (activeArpMappings.size() > 0) {
            juce::HashMap<int, int>::Iterator it(activeArpMappings);
            while (it.next()) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, it.getKey(), 0.0f), 0);
            }
            activeArpMappings.clear();
        }
    }

    //--------------------------------------------------------------------------
    // STAGE 3: LINE TOGGLER MODULE
    //--------------------------------------------------------------------------
    if (!ltBypass) {
        int ltChannel = static_cast<int>(*parameters.getRawParameterValue("LT_CHANNEL"));

        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn() || msg.isNoteOff()) {
                msg.setChannel(ltChannel);
            }
            processedMidi.addEvent(msg, metadata.samplePosition);
        }
        midiMessages.clear();
        midiMessages.addEvents(processedMidi, 0, -1, 0);
        processedMidi.clear();
    }

    //--------------------------------------------------------------------------
    // STAGE 4: CHANNEL FILTER MODULE
    //--------------------------------------------------------------------------
    if (!cfBypass) {
        int cfChannel = static_cast<int>(*parameters.getRawParameterValue("CF_CHANNEL"));

        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.getChannel() == cfChannel || (!msg.isNoteOn() && !msg.isNoteOff())) {
                processedMidi.addEvent(msg, metadata.samplePosition);
            }
        }
        midiMessages.clear();
        midiMessages.addEvents(processedMidi, 0, -1, 0);
        processedMidi.clear();
    }

    //--------------------------------------------------------------------------
    // STAGE 5: CONTROLLER MOTION MODULE
    //--------------------------------------------------------------------------
    if (!cmBypass) {
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

        double phraseSamplePos = std::fmod(static_cast<double>(totalSamplesProcessed), samplesPerPhrase);
        float progress = static_cast<float>(phraseSamplePos / samplesPerPhrase);

        for (int i = 0; i < 4; ++i) {
            int ccValue = static_cast<int>(progress * targetAmt[i] * 127.0f);
            ccValue = juce::jlimit(0, 127, ccValue);

            int actualCC = cmTargetCCs[i].load();

            if (ccValue != lastSentValues[i]) {
                midiMessages.addEvent(juce::MidiMessage::controllerEvent(cmChannel, actualCC, ccValue), 0);
                lastSentValues[i] = ccValue;
            }
        }
    }

    // --- LIVE MIDI: ROUTING POST-FX ---
    // Adds the MIDI file to the end of the chain to bypass effects
    if (!liveBypass && liveRouting && !liveMasterMute) {
        midiMessages.addEvents(liveMidiBuffer, 0, -1, 0);
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
