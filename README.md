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

### Current State

OpenPVD is made up of a collection of tools for working with PVD data and for trying to workaround existing problems.

<br>

#### capture
The capture tool offers a way to write a network PVD connection from a game out to a PXD2 file for later filtering or analysis. Useful if you are working with a game that cannot be asked to write out a PXD2 directly itself.

Simply run `opvd-capture` - it will emulate a running PVD server so games should connect directly to it. Add `-o filename.pxd2` to specify a custom filename to write to.

```
Options:
  -h,--help                   Print this help message and exit
  -o,--out TEXT               filename to write captured data to
  -p,--port UINT              port to listen on
  -b,--buf UINT               transmission buffer, in KB`
```

<br>

#### filter

The filter tool can be used to analyse and unpack a PXD2 capture file, producing a readable dump of the internal data structures as well as a summary of the various PhysX classes in use. 

Crucially it can also help strip out data too. Currently it supports limiting the amount of triangle meshes that get stored. When the capture contains a lot of complex meshes it can easily crash the original PVD application.

eg. to only allow 2000 mesh objects,

`opvd-filter.exe -p input.pxd2 --meshlimit 2000 to_file -o filtered.pxd2`

you can also re-stream a PXD2 file out to the official app using `to_net` - just run the PhysX Visual Debugger and try

`opvd-filter.exe -p mm.pxd2 --meshlimit 2000 to_net -o localhost`

```
Options:
  -h,--help                   Print this help message and exit
  -p,--pxd TEXT:FILE          path to a PXD2 capture file to parse
  --meshlimit INT:POSITIVE    limit of trimesh instances to allow

Subcommands:
  to_file
  to_net
```

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
:white_check_mark:|PVD event filtering, emit to file or network
:white_check_mark:|Experimental PVD network stream live decode
:white_large_square:|MemoryEventBuffer stream decompression
:white_check_mark:|Windowing system - GLFW & Imgui
:white_check_mark:|Choose 3D render / scene graph
:white_large_square:|Decypher type metadata system
:white_large_square:|Dynamic class & property instance tracking
:white_large_square:|Property decompose & update using metadata
:white_large_square:|Data sectioning and playback