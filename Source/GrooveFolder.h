/*
  ==============================================================================

    GrooveFolder.h
    Created: 20 Aug 2025 3:27:42pm
    Author:  evan

    Helper class for storing a set of groove MIDI file names in a single folder,
    the set of subdirs found (possibly containing additional MIDI files),    
    along with the relative "level" down the heirarchy from the "root" folder.
  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

using namespace juce;

class GrooveFolder {
public:
    typedef std::function<void(GrooveFolder&)> EnumerationCb;
    typedef std::shared_ptr<GrooveFolder> GrooveFolderPtr;

    GrooveFolder();
    ~GrooveFolder();

    // Load up all "*.mid" file and subfolder *names* non-recursively
    void Initialize(File, GrooveFolder*);

    // Build the "mChildren" / "mParent" tree
    void Scan();

    void Enumerate(EnumerationCb cb);

    GrooveFolder* GetParent() { return mParent; }
    Array<GrooveFolderPtr>& GetChildren() { return mChildren; }
    File& GetSelfFile() { return mFileSelf; }
    StringArray GetFileNames() { return mFileNames; }
    StringArray GetSubdirNames() { return mSubdirNames; }

private:
    File mFileSelf;
    StringArray mFileNames;
    StringArray mSubdirNames;

    GrooveFolder* mParent{ nullptr };
    Array<GrooveFolderPtr> mChildren;
};
