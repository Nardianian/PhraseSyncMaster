/*
  ==============================================================================

    GrooveTransport.h
    Created: 6 Sep 2025 10:29:26am
    Author:  evan

    GrooveTransport is the heart of the MIDI data sampler for MIDI files.

    It creates & manages a TrackPlayHead for each track in the file.
    
    Each call of ::processMidi() (driven by AudioProcessor::processBlock())
    processes each track looking for MIDI events that fall within the current
    AudioPlayHead's temporal window.

    For each in-window event:
        adjust its timestamp to be relative to current AudioPlayHead time
        add it to the outgoing MidiBuffer
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GrooveActionMessage.h"
#include "Log.h"

using namespace juce;

class GrooveTransport : public juce::ActionListener,
                        public juce::ActionBroadcaster
{
public:
    struct GrooveSequence {
        GrooveSequence(const MidiMessageSequence* ms, int idx) : mSequence(ms), mNextIdx(idx) {};

        const MidiMessageSequence* mSequence;
        int mNextIdx;
    };

    typedef std::vector<GrooveSequence> TrackPlayHeads;

    void initialize(File);
    void addTrack(const MidiMessageSequence*);
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    // Runs in the audio thread at end of LiveMidiAudioProcessor::processBlock()
    // Moved from Protected Makes the function accessible to the PluginProcessor
    void processMidi(const juce::Optional<juce::AudioPlayHead::PositionInfo>& p, int numSamples, juce::MidiBuffer& midiMessages);


protected:
    // Near as I can tell, these are driven by the chosen audio interface.
    double mSampleRate{ 44100.0f };
    int mSamplesPerBlock{ 1024 };

    MidiFile mMidiFile;
    TrackPlayHeads mTrackPlayHeads;
    short mTimeFormat{ 0 };
    int mTimeSigNum{ 4 };
    int mTimeSigDen{ 4 };

    bool mIsPlaying{ false };
    bool mUseInternalTransport{ false };
    double mInternalCurrentTime{ 0.0 };
    double mEndTime{ 0.0 };

    // Bitmap of 128 notes for each of 16 midi channels, indexed as [channel][note>>3]  
    const static int mNumMidiChannels{ 16 };
    const static int mNumNoteBlocks{ 16 };
    unsigned char mActiveNotes[mNumMidiChannels][mNumNoteBlocks];

    void actionListenerCallback(const String&);

    // TODO: make these into ActionMessage broadcasters
    void parseMidiFile();
    void parseTracks();
    void parseMetaEvent(const MidiMessage& message);
    void parseEvent(const MidiMessage& message);

    void sendAllNotesOff(MidiBuffer& midiMessages);

    // MIDI all-notes-off event support is not guaranteed, so track active notes
    // and at play stop time we'll turn them off.
    void markNote(MidiMessage& message);
    void markNoteOn(int channel, int note);
    void markNoteOff(int channel, int note);
    void sendNotesOff(int channel, MidiBuffer&);
};
