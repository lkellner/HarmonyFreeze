# HarmonyFreeze

HarmonyFreeze is a Toon Boom Harmony plugin that lets you transform rigs or parts of a rig without adding new transformation keyframes or nodes, keeping all elements in place. It is designed to facilitate rescaling and repositioning rigs, as well as reusing parts across different rigs.





## Table of Contents
- [Disclaimer](#disclaimer)
- [Installation](#installation)
  - [Windows](#windows)
  - [Mac](#mac)
- [Optional: Building HarmonyFreeze from Scratch](#optional-building-harmonyfreeze-from-scratch)
  - [Windows](#windows-1)
  - [Mac](#mac-1)
- [Usage](#usage)
  - [Drawing Pivots](#drawing-pivots)
  - [Animated Rigs and Stop-Motion Keyframes](#animated-rigs-and-stop-motion-keyframes)
  - [Animated Rigs with Motion Keyframes](#animated-rigs-with-motion-keyframes)
  - [Cloned Drawings and Shared Functions](#cloned-drawings-and-shared-functions)
- [Best Practices](#best-practices)
- [Settings](#settings)
  - [ExperimentalMode](#experimentalmode)
  - [2dMode](#2dmode)
  - [PassOnOglControllerTransformation](#passonoglcontrollertransformation)
  - [UseMultithreading](#usemultithreading)
  - [MoveUnusedPivots](#moveunusedpivots)
  - [DebugMode](#debugmode)
  - [SetInbetweenKeyframesMode](#setinbetweenkeyframesmode)
- [Limitations](#limitations)
  - [Shared Functions](#shared-functions)
  - [Pegs](#pegs)
  - [Elements](#elements)
  - [Curve and Envelope Deformers](#curve-and-envelope-deformers)
  - [Static Transformations](#static-transformations)
  - [OGL Controllers](#ogl-controllers)
  - [Additional limitations (3D Mode off)](#additional-limitations-arising-from-switching-off-2d-mode-in-harmonyfreeze)
- [Roadmap](#roadmap)
- [Acknowledgements](#acknowledgements)
- [License](#license)





# Disclaimer

This plugin has been tested across different versions of Harmony 24 and 25 on both Windows and Mac. Pre-compiled binaries are available for Harmony 24.0.2 and 25.2 on Windows (64-bit) and Mac (ARM64).

This is a beta version. While the plugin is still in development, please save your project before using it. 

Despite having been tested in a broad range of contexts, some corner cases may have slipped through, as the plugin affects a large variety of nodes and needs to work with an equally wide range of Toon Boom Harmony settings.

Additionally, certain combinations of transformations or settings may run into restrictions within Harmony itself. Please read through the [Limitations](#limitations) section for details.

If you encounter any bugs, please report them on the [issues page](https://github.com/lkellner/HarmonyFreeze/issues). Contributions of any kind to this open source project are very much appreciated!

<details>
<summary>Legal</summary>

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</details>





# Installation

 

 ## Windows

Installers are available for Harmony 24.0.2 and 25.2. Download the one matching your version of Harmony.

The installer will attempt to automatically detect your Toon Boom Harmony directory, e.g. `”C:\Program Files (x86)\Toon Boom Animation\Toon Boom Harmony 24 Premium”`.

Confirm that this is the correct directory. If not, there is an option to select it manually.

Follow the installer’s directions to complete the installation.

 

## Mac

Download the Mac binaries for your version of Harmony.

Extract the files and copy the complete "HarmonyFreeze" folder. Open the package contents of your Toon Boom Harmony installation, go to `Contents -> tba -> Plugins`, and paste the folder there.




<img width="1703" height="542" alt="Screenshot 2026-06-12 at 10 03 49 am" src="https://github.com/user-attachments/assets/761cedfe-9bae-40c4-a165-4b8a5039af77" />





# Optional: Building HarmonyFreeze from Scratch

This plugin can only be built using a Toon Boom Harmony C++ SDK that matches your version of Harmony and your operating system. To obtain a copy, please contact Toon Boom directly.

Depending on your SDK version, it may already include a copy of Qt. For SDK versions that don’t, consult the SDK’s README for the required Qt version. Use the [Qt Online Installer](https://download.qt.io/official_releases/online_installers/) and select the version specified there.

 ## Windows

Make sure you have the Visual Studio version recommended by the SDK installed (see the SDK’s README for details).

Open x64 Native Tools Command Prompt for VS <version>. (If you have difficulty finding it, it should appear in the Visual Studio <version> folder when browsing all apps.)

Navigate to the folder where your HarmonyFreeze source code is located and run the following commands:

```
mkdir build
cd build
set HARMONY_SDK_ROOT=<path-to-SDK-folder>
set QTDIR=<path-to-qmake.exe>
<path-to-qmake.exe>\qmake.exe> ..
nmake
```

 

Navigate to the plugin folder of your Toon Boom Harmony installation and, if it doesn’t exist already, create a new folder called “HarmonyFreeze” and inside it a subfolder called “win64”.

In the HarmonyFreeze source directory, you should now find a folder called “Release”. Copy the .dll file from there into the newly created “win64” folder.
Then copy the PluginInfo.xml file and the resources folder into the “HarmonyFreeze” folder.

 

 

## Mac

Open the Terminal app, navigate to the folder where your HarmonyFreeze source code is located, and run the following commands:
```
mkdir build
cd build
export HARMONY_SDK_ROOT=<path-to-SDK-folder>
export QTDIR=<path-to-QT>
export TB_ROOT=<path-to-Toon-Boom-Harmony>
<path-to-qmake>/qmake ..
make
```
 

Escape any spaces in folder names to ensure the variables are set correctly.

Example:
```
mkdir build
cd build
export HARMONY_SDK_ROOT=/Users/laurakellner/Dev/harmonySDK/25.0/harmony-full-25.0.0.23967-macosx_sdk
export QTDIR=/Users/laurakellner/Dev/harmonySDK/25.0/harmony-full-25.0.0.23967-macosx_sdk/qt
export TB_ROOT='/Applications/Toon\ Boom\ Harmony\ 25\ Premium/Harmony\ 25\ Premium.app'
/Users/laurakellner/Dev/harmonySDK/25.0/harmony-full-25.0.0.23967-macosx_sdk/qt/bin/qmake ..
make
```
 
Open the package contents of your Toon Boom Harmony installation and go to `Contents -> tab -> Plugins`. If it doesn’t exist already, create a new folder called “HarmonyFreeze” and within it a subfolder called “macosx”.

In your HarmonyFreeze build folder, you should find four files:

libHarmonyFreeze.dylib

libHarmonyFreeze.1.dylib

libHarmonyFreeze.1.0.dylib

libHarmonyFreeze.1.0.0.dylib

 

Copy these into the newly created “macosx” folder, then copy the PluginInfo.xml file and the resources folder into the “HarmonyFreeze” folder.

## Usage

Once installed, the plugin will be loaded automatically the next time you start Harmony and can be found under 
```Animation->HarmonyFreeze->Freeze Transformation``` : 
 
 
<img width="1342" height="572" alt="ScreenshotAnimation" src="https://github.com/user-attachments/assets/c7a335cc-7090-415d-b0a0-bfde3a6ea10a" />


Additionally, you can assign a shortcut in the keyboard shortcuts menu under 
```Edit->Keyboard Shortcuts->HarmonyFreeze->Freeze Transformation``` :
 

<img width="1736" height="1272" alt="ScreenshotPreferences" src="https://github.com/user-attachments/assets/9ec867ea-f2bf-412f-b93a-436d647872de" />


Select a peg and run HarmonyFreeze via the menu or shortcut. The peg’s transformation in the selected frame will be reset while its transformations are applied to the nodes below, preserving the original appearance of the scene.


Depending on the size of the rig and your computer, HarmonyFreeze may take some time to run. Smaller rigs should process instantly, while very large rigs can take several minutes on an older computer. Open Harmony’s message log before running HarmonyFreeze to see a message when all transformations are complete.

Alternatively, if you start Harmony via the terminal, you can see which node is currently being processed.



>[!IMPORTANT]
>To undo a transformation, it is necessary to press ctrl-z/click undo twice, once for the drawings and once for the attributes.




If you're not sure where to start, try the "Rabbit_Demo_Rig" included with this plugin.




>[!NOTE]
>The following nodes are currently supported:
>Pegs, Elements (Drawings)[^1], Curve Deformers, Envelope Deformers, Static Transformations and OGL Controllers.

[^1]: Vector drawings are fully supported, as are bitmap drawings wrapped inside a vector container (e.g. brush strokes, bitmap drawings copied into a vector layer). True bitmap drawings are only supported in Experimental Mode.


You can find a few more tips for smoothest results in the following section: [Best Practices](#best-practices)



There are a few settings available that can be configured: [Settings](#settings)





# Best Practices

HarmonyFreeze works best in 2D Mode (default) with translations and uniform scales on rigs without 3D rotations.
Rotations, non-uniform scales and skews are generally handled without issue in 2D space, but may result in undesired skew values and additional keyframes.

The plugin has been implemented for 3D space as well. However, there are a few points to keep in mind when working with 3D transformations:

+ Not all nodes contain enough attributes to represent every possible matrix transformation. Depending on the combination of rotations around different axes, scales and skews, applying HarmonyFreeze may result in some differences compared to the original rig.
+ Similarly, changes to drawings and Deformers will be orthographically projected into 2D space.
+ Applying any translation in x, y or z, or a uniform scale (x, y **AND** z), will always work even for rigs with complex 3D transformations when 2D Mode is turned **off**.



Have a look at [Limitations](#limitations) for a better idea of any restrictions you might encounter for each node.

 


>[!Note]
>Keep in mind that only one Peg's transformation will be frozen at a time. If you have a chain of Pegs that all need to be frozen, select the topmost Peg and run HarmonyFreeze, then work your way down the chain one Peg at a time.

## Drawing Pivots

HarmonyFreeze can work with permanent pivots, drawing pivots, or a mixture of the two. You can even connect more than one drawing containing drawing pivots to the same Peg. For best results, make sure there is always a drawing exposed (even if it's empty) for every keyframe that needs to be converted.

 
 


## Animated Rigs and Stop-Motion Keyframes

When using HarmonyFreeze on animated rigs, extra keyframes may be added for frames where only some attributes previously had keyframes. This is done to ensure the correct values are applied, since the attributes of a node or deformer chain are not always fully independent of each other.

 
 


## Animated Rigs with Motion Keyframes

The best way to use HarmonyFreeze on rigs that already include Motion Keyframes is to stick to uniform scaling and translations.

Depending on the setup of the rig, using non-uniform scale, shear or rotation may result in interpolations between keyframes that differ slightly from the original.

To make sure every frame matches the original interpolated value, there is an option to turn on `setInbetweenKeyframesMode` in the [Settings](#settings).

 


## Cloned Drawings and Shared Functions

Ideally, all instances of a drawing or nodes sharing a function should be connected to the Freeze Peg. HarmonyFreeze then ensures that transformations are applied only once.

When this is not the case, transformation changes may appear in clones outside the Freeze Peg’s scope.

To avoid this, if you have, for example, arm drawings shared between both arms and want to use HarmonyFreeze on the Lower Arm, start by adding a temporary Freeze Peg to the scene and connecting it to the two lower arms’ imports.
Transfer the transformation you would like to freeze onto the newly created Freeze Peg. (Make sure the transformation doesn’t exist "twice" — for example, if the Lower Arm was already scaled up by a factor of 2 and this value has been set on the Freeze Peg, reset the original scale on the Lower Arm manually.)
Select the temporary Freeze Peg, run HarmonyFreeze, and once done, reconnect the nodes to their original setup.




# Settings



After running the plugin at least once, settings are usually stored under

`Users/<username>/AppData/Roaming/HarmonyFreeze` on Windows and `/Users/<username>/.config/HarmonyFreeze` on Mac. (Make sure hidden folders are visible to find it.)

For more details, see [Qt | QSettings](https://doc.qt.io/qt-6/qsettings.html).

You can open the “HarmonyFreeze.ini” file in any text editor. It should look like this:

 
```
[General]
ExperimentalMode=false
2dMode=true
PassOnOglControllerTransformation=true
UseMultithreading=false
MoveUnusedPivots=false
DebugMode=false
SetInbetweenKeyframesMode=false
```

 

To make changes, toggle the setting you'd like to modify from `true` to `false` or vice versa, then save the file.



## ExperimentalMode
>[!CAUTION]
>Use HarmonyFreeze with extreme caution when set to true, as undoing/redoing transformations is not possible. Make sure to save your work before calling HarmonyFreeze.

Since Experimental Mode bypasses JavaScript, it is considerably faster. It also enables true bitmap drawings to be modified.


## 2dMode

2D Mode projects a 3D transformation into 2D, e.g. a rotation around the x axis becomes a scale in x. Keep it on for 2D rigs, since most nodes lack the full range of parameters needed to store a 3D transformation.

Nodes that work fairly well with 2D Mode turned off include: Pegs with 3D turned on, Static Transformations and OGL Controllers without drawings attached.


## PassOnOglControllerTransformation

In line with the general concept of the plugin, only nodes connected downstream of the selected peg should be transformed.

When it comes to OGL Controllers with plugged-in drawings, it is more intuitive to transform the drawings rather than the controller's position. Enabling this setting does exactly that.


## UseMultithreading

Multithreading is only available in Experimental Mode. It allows Elements to be modified at the same time as attributes are being set, resulting in faster processing.


## MoveUnusedPivots

When set to false, drawing pivots on drawings set to “Do not use drawing pivots”, as well as permanent pivots on Element nodes without any transformation, won’t be recalculated.


## DebugMode

When Experimental Mode is turned off, all attributes are applied via a single JavaScript script.

With Debug Mode enabled, the script is saved to the scene’s script folder instead of a temporary file, and can be accessed for reference.
Additionally, Debug Mode will display the time spent running HarmonyFreeze when Harmony is launched via the terminal.


## SetInbetweenKeyframesMode

When using Motion Keyframes, the interpolations between keyframes after applying HarmonyFreeze may not 100% match the originals. `SetInbetweenKeyframesMode` is an option to preemptively set a new keyframe for every frame as soon as a value changes compared to the previous keyframe.



# Limitations

When using HarmonyFreeze in 2D Mode, you should rarely encounter any limitations. This may change when 2D Mode is turned off or with certain node settings.

Most nodes have a limited range of attributes, some with options to restrict them further. As a result, there are cases where a node cannot store and display the full transformation it was supposed to.

To keep transformations linear, HarmonyFreeze does not hypercorrect missing information (with a few exceptions), to avoid side effects for cloned functions and Elements.

The following is an overview of the limitations you may encounter:



## Shared Functions

For nodes that aren't full clones but only share some functions with other nodes, some inaccuracies may occur, since not all parameters are fully independent of each other.



## Pegs

By default, Pegs can cover the full range of transformations that can arise from applying HarmonyFreeze in 2D Mode. Limitations may arise in the following cases:

+ Scale is set to “Uniform” but requires different x and y values
+ “Ignore parent scaling” is turned on and the parent’s scale is not uniform

Apart from that, there is a bug in Harmony 24 that causes single keyframes to go undetected when they are exclusively set on a quaternion or path3d attribute. This can be avoided by adding a second keyframe.



## Elements

True bitmaps can currently only be transformed in Experimental Mode (no undo/redo possible, see [Settings](#settings)).

This only applies to bitmap drawings in a sublayer set to bitmap. Bitmap drawings encapsulated as vector drawings, such as brush strokes or bitmaps copied into vector sublayers, work as usual.

Elements that have been created while using a value different from the default “24” as the second parameter in “Scene Settings” > “Number of Units” won’t transform correctly. HarmonyFreeze can, however, properly transform Elements when the “Number of Units” value has been changed only after their creation.




<img width="442" height="580" alt="ScreenshotUnitOffset" src="https://github.com/user-attachments/assets/99df5da4-18c2-4193-9139-6151d58bd793" />




## Curve and Envelope Deformers

Limitations for Deformers can only arise from their “Region of Influence” attributes:

+ There are no limitations for Deformers whose “Influence Type” is set to “Infinite” or “Zero”
+ For “Elliptic” regions of influence, changes are approximated
+ Currently, region of influence paths defined by “Shaped” cannot be transformed



## Static Transformations

Harmony doesn’t seem to use a true inverse matrix when “Invert transformation” is selected. The resulting matrix’s x, y and translation columns match the corresponding inverse matrix of the selected node's parameters, however the z column differs. This can lead to changes when using HarmonyFreeze with a Static Transformation node’s 3D parameters.

Static Transformation nodes without “Invert transformation” selected, or nodes that only use 2D parameters, won’t be affected.



## OGL Controllers

OGL Controllers only contain a uniform scale and no rotation. By default, transformations are passed on to the incoming drawings. When, however, this option isn’t selected or when this is not possible, the OGL Controller’s attributes will need to be used, which for rotations, non-uniform scales or skews may not always be able to recreate the transformation precisely.



 
## Additional limitations arising from switching off 2D Mode in HarmonyFreeze




## Pegs

Pegs should be set to 3D when switching off 2D Mode in HarmonyFreeze.

As they, however, only contain one type of skew parameter instead of the three needed to store any possible transformation, some combinations of 3D rotation, scale or skew could result in transformations that cannot fully be captured.

For Pegs with 3D transformations switched off, the limitations are even greater.



## Static Transformations

While the Static Transformation node does include three skew parameters, the third one is not completely independent. Though less likely, cases may still arise, similar to Pegs, where a transformation can’t be fully captured.



## Curve/Envelope Deformers and Elements

Transformations will be orthographically projected into 2D space.




# Roadmap

The next steps in this project are to add support for
+ Point Kinematic Outputs
+ Bone Deformers
+ Free Form Deformers

as well as creating a UI for the settings.



 
# Acknowledgements

Special thanks to my partner for bouncing ideas off, helping with the build systems, and being a second pair of eyes on the code, as well as everyone else who has supported this project and earlier attempts.




This software uses Qt licensed under the GNU General Public License (GPL).

Qt source code and licensing information:
[qt.io](https://www.qt.io)


The Windows installer for this plugin has been created using [Inno Setup](https://jrsoftware.org/isinfo.php).





# License

GPL-3.0-only. See LICENSE.
