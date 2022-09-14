# OpenPVD

NVIDIA ships a [visual debugger](https://developer.nvidia.com/physx-visual-debugger) for their PhysX physics engine, a hugely useful tool for finding quirky bugs or poorly setup physics scenes. 

Sadly, whilst PhysX itself is now open-source, *PhysX Visual Debugger* (PVD) has remained a closed-source component that hasn't seen an update since 2018. Using it with complex physics scenes or with more recent PhysX releases can cause crashes and unsolvable issues.

The last communication from NVIDIA about PVD was [here](https://github.com/NVIDIAGameWorks/PhysX/issues/224), late 2019.

> ... We are just starting work on quite a large rewrite of PVD because there are some fundamental problems with the current incarnation that are hard to fix. So while help is on the way it will still take quite some time to arrive -- I don't have a date so far.


So, although it may be eventually be pointless if NVIDIA *do* suddenly release a new version, here's an attempt to assemble something that can perhaps help out until that rewrite arrives.

<br>
<hr>
<br>

### The Effort

The source release of PhysX contains a large amount of PVD-related code, however it is all concerned with the "sending to PVD" side of affairs .. there is a surprising hole when it comes to unpacking, decoding or decompressing all that data, a hole that presumably is filled by a lot of custom code that lives in the private PVD codebase.

**OpenPVD** uses the latest PhysX code as canonical reference to extract any PVD types and structures available to assist with decoding the debug data stream. 

<br>
<hr>
<br>

### Building

* `git submodule init`
* `git submodule update`
* run `premake.bat` in `/build`
* load and build `vs2019_OpenPVD.sln` in `/build/_generated`

(you will need test PXD2 capture files to load)

<br>
<hr>
<br>

### The Goals & State

This project is extremely WIP and subject to major changes

Done? | Task
:---:| ---
:white_check_mark:|premake project setup
:white_check_mark:|stripped-down static build of PhysX 4.1
:white_check_mark:|`PXD2` file loading
:white_check_mark:|PVD event stream decoder loop
:white_check_mark:|PVD event stream deserialize to log
:white_large_square:|MemoryEventBuffer stream decompression
:white_large_square:|Choose windowing system 
:white_large_square:|Choose 3D render / scene graph
:white_large_square:|Decypher type metadata system
:white_large_square:|Dynamic class & property instance tracking
:white_large_square:|Property decompose & update using metadata
:white_large_square:|Data sectioning and playback