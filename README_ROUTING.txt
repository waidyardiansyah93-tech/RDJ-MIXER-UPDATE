RDJ MIXER - Routing Stage 1

Files:
- CMakeLists.txt
- Source/Main.cpp

Implemented in this stage:
- Channel output selector: Master L/R, Group Bus 1-4, physical mono/stereo outputs.
- Four stereo group summing buses with independent level and mute controls.
- Group buses feed the stereo Master output.
- Group bus levels/mutes are saved and restored in scene files.
- Existing channel FX send and shared stereo FX return are retained.

Important:
- This stage currently has one shared FX return chain, not four independent FX returns.
- Build has not been verified in Visual Studio. Back up your project before replacing files.
