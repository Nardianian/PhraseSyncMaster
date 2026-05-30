/*
  ==============================================================================

    PluginProcessor.h
    Created: May 2026
    Author: PhraseSync Team

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GrooveTransport.h"
#include "Log.h" // If needed for backend logging macros

//==============================================================================
// LIVE MIDI (GROOVE TRANSLATOR) ENUMERATORS
//==============================================================================
namespace LiveMidiRouting {
    enum Enum { PreFX = 0, PostFX };
}  

//==============================================================================
// LOGICAL ENUMERATORS FOR THE VARIOUS INTERNAL MOTORS
//==============================================================================

namespace ArpWhenNoChord {
    enum Enum { Silence = 0, UsePatternAsNotes, LatchLastChord };
}

namespace ArpWhenSingleNote {
    enum Enum { Silence = 0, UsePatternAsNotes, UseAsIs, Powerchord, TransposeLastChord };
}

namespace ArpMappingMode {
    enum Enum { AlwaysUnmapped = 0, SemitoneToDegree, WhiteKeyToDegree };
}

namespace ArpWraparound {
    enum Enum { NoWraparound = 0, AfterAllChordDegrees };
}

namespace ArpUnmappedBehaviour {
    enum Enum { Silence = 0, UseAsIs, TransposeFromFirstDegree, PlayFullChordUpToNote };
}

namespace ArpRate {
    enum Enum { 
        Rate_1_4 = 0,  // Crotchet (Quarter Note)
        Rate_1_4T,     // Crotchet Triplet (Quarter Note Triplet)
        Rate_1_8,      // Quaver (Eight Note)
        Rate_1_8T,     // Quaver Triplet (Eight Note Triplet)
		Rate_1_16,     // Semiquaver (Sixteenth Note)
        Rate_1_16T,    // Semiquaver Triplet (Sixteenth Note Triplet)
		Rate_1_32      // Demisemiquaver (Thirty-Second Note)
    };
}

//==============================================================================
class PhraseSyncMasterAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    PhraseSyncMasterAudioProcessor();
    ~PhraseSyncMasterAudioProcessor() override;

    GrooveTransport& getGrooveTransport() { return grooveTransport; }

    // --- LIVE MIDI METHODS ---
    void initialize(const juce::File& midiFile);

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // SINGLE PARAMETER CENTRAL (APVTS) ACCESSIBLE FROM THE EDITOR
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;

    //==============================================================================
    // CUSTOM REGION: EXTENDED MIDI LEARN INTERFACE (11 SLOTS)
    //==============================================================================
    void startMidiLearn(int targetSlotIndex) {
        if (targetSlotIndex >= 0 && targetSlotIndex < 17) {
            targetLearnParamIndex = targetSlotIndex;
            isMidiLearnActive = true;
        }
    }

    void stopMidiLearn() {
        isMidiLearnActive = false;
        targetLearnParamIndex = -1;
    }

    bool isLearning() const { return isMidiLearnActive.load(); }
    int getActiveLearnSlot() const { return targetLearnParamIndex.load(); }
     
    void unlearnMidi(int targetSlotIndex) {
        if (targetSlotIndex >= 0 && targetSlotIndex < 17) {
            cmTargetCCs[targetSlotIndex].store(0); // 0 = no CC assigned
        }
    }
      
    int getMappedCCForTarget(int slotIndex) const {
        if (slotIndex >= 0 && slotIndex < 17) return cmTargetCCs[slotIndex].load();
        return 0;
    }  

private:
    // Helper function to initialize the APVTS parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // SHARED INTERNAL STATES AND BASIC CLOCK VARIABLES
    //==============================================================================
    double currentSampleRate = 44100.0;
    
    // Data structure to store the active agreement (Fixed arrays to avoid allocation)
    bool isChordNoteActive[128] = { false };
    int currentChordSize = 0;

    bool isLastValidNoteActive[128] = { false };
    int lastValidChordSize = 0;

    int noteCounters[128] = { 0 };

    // Data structure to store the active arpeggiator mappings (Stores the MIDI channel, 0 = inactive)
    int activeArpMappings[128] = { 0 };

    // Global time variables derived from the DAW Playhead
    double tempoBpm = 120.0;
    double lastPpqPosition = 0.0;
    bool isDawPlaying = false;
    double currentSamplesPerBeat = 0.0;    

    // Internal step counter for the arpeggiator synchronized to the clock
    juce::int64 totalSamplesProcessed = 0;
    double sampleRemainder = 0.0;
    int currentArpStep = 0;

    //--------------------------------------------------------------------------
    // MIDI LEARN STATUS
    //--------------------------------------------------------------------------
    std::atomic<bool> isMidiLearnActive{ false };
    std::atomic<int> targetLearnParamIndex{ -1 }; // Identify which slot (0-3) is learning

    // Store the associated CCs: 0-3 (Targets M5), 4 (Variation M1), 5 (Rate M2),
        // 6 (Height Knob M1), 7 (NoChord M2), 8 (SingleNote M2), 9 (Chan Knob M3), 10 (IsoChan Knob M4)
        // 11 (Phrase Type Menu M5), 12 (LiveMidi Play), 13 (+Meas), 14 (-Meas), 15 (Bypass), 16 (Pre/Post Switch)
    std::atomic<int> cmTargetCCs[17]{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };

    //--------------------------------------------------------------------------
    // LIVE MIDI (GROOVE TRANSLATOR) ENGINE & STATES
    //--------------------------------------------------------------------------
    GrooveTransport grooveTransport;
    std::atomic<bool> grooveChannelMutes[16]{ false }; // true = silenced, false = active 

    // Array to track the last CC values ​​sent (Stage 5)
    // As member of the class to avoid conflicts between multiple instances
    int lastSentValues[4] = { -1, -1, -1, -1 };

    //==============================================================================   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhraseSyncMasterAudioProcessor)
};
