/*
  ==============================================================================

    GrooveFolder.cpp
    Created: 20 Aug 2025 3:27:42pm
    Author:  evan

  ==============================================================================
*/

#include "GrooveFolder.h"
#include "Log.h"

GrooveFolder::GrooveFolder() 
{
}

GrooveFolder::~GrooveFolder()
{
}

// Fills a GrooveFolder's mSubdirNames Array and mFileNames Array
void GrooveFolder::Initialize(File f, GrooveFolder* parent) {
    mSubdirNames.clear();
    mFileNames.clear();
    mChildren.clear();

    mFileSelf = f;
    mParent = parent;

    auto subdirs = mFileSelf.findChildFiles(File::findDirectories, false);
    for (const auto& subdir : subdirs)
        mSubdirNames.add(subdir.getFileName());

    auto files = mFileSelf.findChildFiles(File::findFiles, false, "*.mid");
    for (const auto& file : files)
        mFileNames.add(file.getFileName());
}

// Recursively scan mSubdirNames, creating new GrooveFolder mChildren along the way
void GrooveFolder::Scan() {
    if (mSubdirNames.size()) {
        for (const auto& subdirName : mSubdirNames) {
            // Create a File object for the child...
            // Figure out the absolute path for File class
            String subdirAbsolutePathName = mFileSelf.getFullPathName() + File::getSeparatorString() + subdirName;

            auto file = File(subdirAbsolutePathName);

            auto child = std::make_shared<GrooveFolder>();
            child->Initialize(file, this);
            child->Scan();

            mChildren.add(child);
        }
    }
}

void GrooveFolder::Enumerate(EnumerationCb cb) {
    cb(*this);
}
