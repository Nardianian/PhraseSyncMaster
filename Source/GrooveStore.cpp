/*
  ==============================================================================

    GrooveStore.cpp
    Created: 19 Aug 2025 2:43:05pm
    Author:  evan

  ==============================================================================
*/

#include <assert.h>
#include "GrooveStore.h"
#include "Log.h"

GrooveStore::GrooveStore() {
}

GrooveStore::~GrooveStore() {
}

void GrooveStore::Initialize(File f) {
    mRoot.Initialize(f, nullptr);
    mRoot.Scan();
}

void GrooveStore::Enumerate(GrooveFolder::EnumerationCb cb) {
    mRoot.Enumerate(cb);
}

GrooveFolder* GrooveStore::GetGrooveFolder(GrooveSelector& selector) {
    assert(selector.size());

    // Start at the root...
    GrooveFolder* node = &mRoot;

    // move down the hiearchy of "selector" choices
    for (int i = 0; i < selector.size(); i++)
        node = node->GetChildren()[selector[i]].get();
    
    // *might* be nullptr, upper layers handle that.
    return node;
}

File GrooveStore::GetGrooveFile(GrooveSelector& selector) {
    GrooveFolder* grooveFolder;
    int rowIdx = selector.getLast();

    // selector from somewhere lower in the hierarchy that points to a file
    if (selector.size() > 1) {
        selector.removeLast();

        // Truncate "selector" so we select the GrooveFolder one level up
        grooveFolder = GetGrooveFolder(selector);
    }
    else {
        // "selector" is referring to "root" GrooveFolder, but not a child folder
        grooveFolder = GetRoot();
    }

    // rowIdx is known to be greater than the number of GrooveFolder children,
    // so adjust it accordingly.
    rowIdx -= grooveFolder->GetChildren().size();

    return File(grooveFolder->GetSelfFile().getFullPathName() + File::getSeparatorString() + grooveFolder->GetFileNames()[rowIdx]);
}