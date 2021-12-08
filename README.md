# OpenGL Hello Texture Demo using PVR_PSP2 Driver layer GPU libraries 

This is a openGL hello texture program. It was made as a tech demo using [PVR\_PSP2 Driver layer GPU libraries](https://github.com/GrapheneCt/PVR_PSP2). 

The PVR\_PSP2 initialization and library handling was adapted from the work of [ZeroNinx](https://github.com/ZeroNinx/VitaTest). The hello texture implements openGL Getting started/Textures tutorial from [LearnOpenGL](https://learnopengl.com/Getting-started/Hello-Triangle), and has been adapted to work with openglES 2 and GLSL version 100. PS Vita code was based on [SonicMastr's](https://github.com/SonicMastr/Pigs-In-A-Blanket/tree/master/samples/cube-egl-vitasdk) implementation. 


## Features

- Implements PVR\_PSP2 libraries, instead of VitaGL, or Pigs in a Blanket.
- No more CG to GLSL shader conversion.
- Barebones example for those interested in using the GPU libraries, and following along some basic examples to get started into GLES 2.0 graphics programming.

## Compiling

Requires [VitaSDK](https://vitasdk.org/) and PVR\_PSP2 >=v3.5.
Once you have installed VitaSDK, clone [PVR_PSP2](https://github.com/GrapheneCt/PVR_PSP2/releases/tag/v3.5) repository, and download [VitaSDK Stubs](https://github.com/GrapheneCt/PVR_PSP2/releases/download/v3.5/vitasdk_stubs.zip) from the releases section of PVR\_PSP2.
Copy all .a files from vitasdk_stubs.zip to your $VITASDK lib path,

```sh
$VITASDK/arm-vita-eabi/lib
```

Copy include folder from PVR\_PSP2 repository to your VitaSDK include folder, and rename it to PVR\_PSP2

```sh
$VITASDK/arm-vita-eabi/include/
```

Compile using cmake and make,
```sh
mkdir cmake_build
cmake ..
make
```

## Running

Copy the generated VPK to your PS Vita using your preferred method.
Install using VitaShell.
Enjoy the simple demo.

## Credits
Thanks to all the great people that made this possible,
* GrapheneCt, Bythos, and Jaylon Gowie for developing PVR_PSP2.
* ZeroNix's code to properly setup the graphics library.
* SonicMastr for cube demo on PS Vita.
* JoeyDeVries LearnOpenGL demo files, and learnopengl.com

## License

MIT

