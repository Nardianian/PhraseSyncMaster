# PhraseSyncMaster
PhraseSync Master is a (WIP) real-time MIDI plugin for synchronizing, filtering, arpeggiating, and automating complex MIDI data streams while preserving the expressiveness and cleanliness of the signal.

Thanks to Rua Haszard (aka Haszari) for his "SyncPhrase Plugins" here at https://github.com/haszari/PhraseSyncPlugins , to Yves Parès (aka Ywen) for his "Arpligner" plugin here at https://github.com/YPares/arpligner and to Evan Mortimore (aka freeflyclone) for his "LiveMidi" plugin available here on GitHub at this link:   https://github.com/freeflyclone/LiveMidi 

This work started with a refactoring of R. Haszard's plugins and - after a partial rewrite and deep revision - by chaining them in a given sequence and into a single interface, integrating them with Y. Parès's arpeggiator, this last one deeply revised, and "Live Midi" by E. Mortimore, which have had some short sections of code revised, particularly those relating to sending midi data and some lines in "GrooveTransport.h\.cpp" and "GroovePlayer.h\.cpp".
<img width="1016" height="486" alt="PhraseSyncMaster-v1 2 1" src="https://github.com/user-attachments/assets/a224cb75-df70-424d-9cf4-2419f6273a3b" />

This is a work in progress that still needs to be properly tested; if you want to compile it yourself, remember to check that the SDK directories into the .jucer file match those on your system.
