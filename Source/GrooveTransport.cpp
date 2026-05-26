/*
  ==============================================================================

    GrooveTransport.cpp
    Created: 6 Sep 2025 10:29:26am
    Author:  evan

  ==============================================================================
*/

#include "GrooveTransport.h"

void GrooveTransport::initialize(File f) {
    mTrackPlayHeads.clear();
    mMidiFile.clear();
    mEndTime = 0.0;

    auto size = sizeof(mActiveNotes);
    memset(mActiveNotes, 0, size);

    FileInputStream fileInStream = FileInputStream(f);
    mMidiFile.readFrom(fileInStream);

    parseMidiFile();
}

void GrooveTransport::addTrack(const MidiMessageSequence* track) {
    mTrackPlayHeads.emplace_back(track, 0);
}

void GrooveTransport::prepareToPlay(double sampleRate, int samplesPerBlock) {
    mSampleRate = sampleRate;
    mSamplesPerBlock = samplesPerBlock;
}  

void GrooveTransport::actionListenerCallback(const String& message) {
    GrooveActionMessage gam = json::parse(message.toStdString());

    // GroovePlayer component ActionMessages we care about...
    if (gam["component"] == "GPLYR") {
        if (gam["action"] == "TPTCTRL") {
            std::string buttonText(gam["value"]);
            MYDBG(__FUNCTION__"(): button: " + buttonText);

            // Calculating the duration of a single beat (Measure) in seconds
            // If the track does not have a detected tempo, we assume a default of 120 BPM.
            double currentBpm = 120.0;
            double secondsPerBeat = 60.0 / currentBpm;
            double secondsPerMeasure = secondsPerBeat * mTimeSigNum; // mTimeSigNum loaded by parseMetaEvent

            if (buttonText == "Play") {
                mUseInternalTransport = true;
                mIsPlaying = true;
            }
            else if (buttonText == "Start") {
                mUseInternalTransport = true;
                mInternalCurrentTime = 0.0;
                mIsPlaying = true;
            }
            else if (buttonText == "Stop") {
                mIsPlaying = false;
                mInternalCurrentTime = 0.0;
                mUseInternalTransport = false;
                // Send all-notes-off to the processor via a temporary buffer,
                // this call clears the internal state of active notes
                auto size = sizeof(mActiveNotes);
                memset(mActiveNotes, 0, size);
                for (auto& track : mTrackPlayHeads)
                    track.mNextIdx = 0;
            }
            else if (buttonText == "Pause") {
                mIsPlaying = false;
            }
            else if (buttonText == "End") {
                mInternalCurrentTime = mEndTime;
                for (auto& track : mTrackPlayHeads)
                    track.mNextIdx = track.mSequence->getNumEvents();
            }
            else if (buttonText == "+Meas") {
                mInternalCurrentTime += secondsPerMeasure;
                if (mInternalCurrentTime > mEndTime) mInternalCurrentTime = mEndTime;

                // Recalculate track play indexes to skip forward
                for (auto& tph : mTrackPlayHeads) {
                    tph.mNextIdx = 0;
                    int numEvents = tph.mSequence->getNumEvents();
                    while (tph.mNextIdx < numEvents &&
                        tph.mSequence->getEventPointer(tph.mNextIdx)->message.getTimeStamp() < mInternalCurrentTime) {
                        tph.mNextIdx++;
                    }
                }
            }
            else if (buttonText == "-Meas") {
                mInternalCurrentTime -= secondsPerMeasure;
                if (mInternalCurrentTime < 0.0) mInternalCurrentTime = 0.0;

                // Recalculate track play indexes to skip backward
                for (auto& tph : mTrackPlayHeads) {
                    tph.mNextIdx = 0;
                    int numEvents = tph.mSequence->getNumEvents();
                    while (tph.mNextIdx < numEvents &&
                        tph.mSequence->getEventPointer(tph.mNextIdx)->message.getTimeStamp() < mInternalCurrentTime) {
                        tph.mNextIdx++;
                    }
                }
            }
        }
    }
}

void GrooveTransport::parseMidiFile() {
    mMidiFile.convertTimestampTicksToSeconds();

    auto numTracks = mMidiFile.getNumTracks();
    MYDBG(__FUNCTION__"(): found " + std::to_string(numTracks) + " channels");

    mTimeFormat = mMidiFile.getTimeFormat();
    if (mTimeFormat > 0) {
        MYDBG(" time format: " + std::to_string(mTimeFormat) + " PPQ");
    }
    else {
        MYDBG(" SMPTE time format: " + std::to_string((-mTimeFormat) >> 8 & 0xFF) + ":" + std::to_string(mTimeFormat & 0xFF));
    }

    // Add all tracks to "mTracks"
    for (int idx = 0; idx < numTracks; idx++)
        addTrack(mMidiFile.getTrack(idx));

    parseTracks();
}

void GrooveTransport::parseTracks() {
    int idx{ 0 };

    for (const auto& track : mTrackPlayHeads) {
        double trackEndTime = track.mSequence->getEndTime();
        int numEvents = track.mSequence->getNumEvents();

        MYDBG("    track: " + std::to_string(idx++) + ", " + std::to_string(numEvents) + " events, end time: " + std::to_string(trackEndTime));

        if (trackEndTime > mEndTime)
            mEndTime = trackEndTime;

        for (int i = 0; i < numEvents; i++) {
            auto event = track.mSequence->getEventPointer(i);
            auto message = event->message;

            if (message.isMetaEvent())
                parseMetaEvent(message);
            else
                parseEvent(message);
        }
    }
}

void GrooveTransport::parseMetaEvent(const MidiMessage& message) {
    if (message.isTrackNameEvent()) {
        MYDBG("       track name: " + message.getTextFromTextMetaEvent().toStdString() + " @ " + std::to_string(message.getTimeStamp()));
    }
    else if (message.isEndOfTrackMetaEvent()) {
        MYDBG("       End-of-track @ timestamp: " + std::to_string(message.getTimeStamp()));
    }
    else if (message.isTempoMetaEvent()) {
        double tempo = 60.0 / message.getTempoSecondsPerQuarterNote();
        MYDBG("       Tempo Event:  Tempo: " + std::to_string(tempo));
    }
    else if (message.isTimeSignatureMetaEvent()) {
        message.getTimeSignatureInfo(mTimeSigNum, mTimeSigDen);
        MYDBG("       Time Signature: " + std::to_string(mTimeSigNum) + " / " + std::to_string(mTimeSigDen));
    }
    else {
        std::stringstream ss;
        ss << std::hex << message.getMetaEventType();
        std::string hexStr = ss.str();
        MYDBG("       meta, type: " + hexStr + ", ts: " + String::formatted("%0.4f", message.getTimeStamp()).toStdString());
    }
}

void GrooveTransport::parseEvent(const MidiMessage& message) {
    if (!message.isNoteOnOrOff())
        MYDBG("      event: " + message.getDescription().toStdString() + ", ts: " + String::formatted("%0.4f", message.getTimeStamp()).toStdString());
}

void GrooveTransport::markNoteOn(int channel, int note) {
    jassert(channel >= 1 && channel <= 16);
    jassert(note >= 0 && note <= 127);

    mActiveNotes[channel-1][note>>3] |= (1 << (note & 0x7));
}

void GrooveTransport::markNoteOff(int channel, int note) {
    jassert(channel >= 1 && channel <= 16);
    jassert(note >= 0 && note <= 127);

    mActiveNotes[channel-1][note >> 3] &= ~(1 << (note & 0x7));
}

void GrooveTransport::markNote(MidiMessage& message) {
    if (message.isNoteOn())
        markNoteOn(message.getChannel(), message.getNoteNumber());
    else
        markNoteOff(message.getChannel(), message.getNoteNumber());
}

void GrooveTransport::sendNotesOff(int channel, MidiBuffer& midiMessages) {
    // go through each byte of the bitmap that tracks note on/off state...
    for (int noteBlock = 0; noteBlock < mNumNoteBlocks; noteBlock++) {
        unsigned char activeNotesThisBlock = mActiveNotes[channel - 1][noteBlock];

        // for each bit in this activeNotesThisBlock...
        for (int bitIdx = 0; bitIdx < sizeof(unsigned char) * 8; bitIdx++) {
            unsigned char mask = 1 << bitIdx;

            // if this bit (note) is on...
            if (activeNotesThisBlock & mask) {
                // calculate the note number
                int noteNumber = noteBlock * 8 + bitIdx;

                // and send a Note On with 0 velocity.  (I believe its the most universal note-off) 
                midiMessages.addEvent(MidiMessage().noteOn(channel, noteNumber, 0.0f), 0);
            }
        }
    }
}

void GrooveTransport::sendAllNotesOff(MidiBuffer& midiMessages)
{
    // Go through all 16 MIDI channels...
    for (auto channel = 1; channel <= mNumMidiChannels; channel++) {
        midiMessages.addEvent(MidiMessage::allNotesOff(channel), 0);
        midiMessages.addEvent(MidiMessage::allSoundOff(channel), 0);
        midiMessages.addEvent(MidiMessage::allControllersOff(channel), 0);

        // silence hanging notes, 'cuz allNotesOff() not guaranteed.
        sendNotesOff(channel, midiMessages);
    }

    // rewind "mNextIdx" for every track in the file 
    for (auto& track : mTrackPlayHeads)
        track.mNextIdx = 0;

    mIsPlaying = false;
}

void GrooveTransport::processMidi(
    const Optional<AudioPlayHead::PositionInfo>& posInfo,
    int numSamples,
    MidiBuffer& midiMessages)
{
    if (!posInfo.hasValue()) {
        MYDBG(__FUNCTION__"(): missing PositionInfo");
        return;
    }

    // If/when we're synced to host's tempo...
    static double hostBpm{ 0.0 };
    auto bpm = posInfo->getBpm();
    if (bpm.hasValue()) {
        if (*bpm != hostBpm) {
            hostBpm = *bpm;
            MYDBG("  Host BPM: " + std::to_string(hostBpm));
        }
    }

    // If not playing there's nothing to do
    if (!mUseInternalTransport && !posInfo->getIsPlaying()) {
        if (mIsPlaying)
            sendAllNotesOff(midiMessages);
        return;
    }

    if (mUseInternalTransport && !mIsPlaying) {
        return;
    }

    // Use time-in-seconds to specify the temporal window bounds 
    double startTime = 0.0;
    if (mUseInternalTransport) {
        startTime = mInternalCurrentTime;
        mInternalCurrentTime += (numSamples / mSampleRate);
    }
    else {
        startTime = posInfo->getTimeInSeconds().orFallback(0.0);
    }
    double endTime = startTime + (numSamples / mSampleRate);

    // If we're playing and we're past the end of the longest track...
    if (mIsPlaying && startTime > mEndTime) {
        sendAllNotesOff(midiMessages);
        return;
    }

    // For all the tracks we have (that are in the MIDI file)...
    for (auto& tph : mTrackPlayHeads) {
        int numEventsThisTrack = tph.mSequence->getNumEvents();

        // Expect tracks to vary in number of events
        if (tph.mNextIdx == numEventsThisTrack)
            continue;

        // For each remaining event in this track: starting at 1st event AFTER current "startTime"...
        for (int idx = tph.mNextIdx; idx < numEventsThisTrack; idx++) {
            MidiMessage message = tph.mSequence->getEventPointer(idx)->message;
            double eventTime = message.getTimeStamp();

            // If next event occurs AFTER current "endTime", we're done with this track for now.
            if (eventTime >= endTime)
                break;

            // Adjust time stamp (as sample position), make it relative to current play head.
            auto samplePosition = roundToInt((message.getTimeStamp() - startTime) * mSampleRate);
            midiMessages.addEvent(message, samplePosition);

            // Track all note events for more efficient "all notes off" behavior 
            if (message.isNoteOnOrOff())
                markNote(message);

            tph.mNextIdx++;
            mIsPlaying = true;
        }
    }
}
