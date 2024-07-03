# Picture Perfector

Path-tracing rendering engine created for the Computer Graphics 1 lecture at Saarland University by [Lukas Auer](https://github.com/LeS4kul) and [me](https://github.com/JoeJoeTV).
This renderer is based upon the lighwave framework that was provided for the course.

![Scene rendered with picture_perfector](https://sonic.joejoetv.de/uploads/2eef1e9f-3b9a-4bcc-9f88-36fa6eb62012.png)

## Features

### Provided by Lighwave

Some base functionality was already provided by lightwave:

* Modularity
  * Modern APIs flexible enough for sophisticated algorithms
  * Shapes, materials, etc are implemented as plugins
* Basic math library
  * Vector operations
  * Matrix operations
  * Transforms
* File I/O
  * An XML parser for the lightwave scene format
  * Reading and writing various image formats
  * Reading and representing triangle meshes
  * Streaming images to the [tev](https://github.com/Tom94/tev) image viewer
* Multi-threading
  * Rendering is parallelized across all available cores
  * Parallelized scene loading (image loading, BVH building, etc)
* BVH acceleration structure
  * Data-structure and traversal is supplied by us
  * Split-in-the-middle build is supplied as well
* Useful utilities
  * Thread-safe logger functionality
  * Assert statements that provide extra context
  * An embedded profiler to identify bottlenecks of your code
  * Random number generators
* A Blender exporter
  * Export Scenes built in Blender to the XML format used by lightwave

### Implemented in Picture Perfector

More features required for a working renderer were implemented into Picture Perfector in the practical assignments and advanced ones for the rendering competition:

* Camera Models
    - Basic Perspective Camera
    - Thinlens Camera
* Basic Primitives
    - Sphere
    - Mesh/Triagle
* Instancing Support
* Different Integerators
    - Normals integrator: Only renders normals of scene objects
    - Direct lighting integrator: Only renders using direct light
    - Path tracing imtegrator: Full path tracer using bounces
    - Albedo integrator
    - SDF bounce count integrator
    - Path tracing integrator for volumetric rendering
* BSDFs:
    - Diffuse
    - Conductor
    - Dielectric
    - Rough Conductor
    - Simple Principled
* Lambertian Emission
* Textures:
    - Checkerboard Texture
    - Image Texture
* Lights:
    - Environment Map
    - Area Lights
    - Point Light
    - Directional Light
* Next Event Estimation
* Image denoising using OpenImageDenoise
* Acceleration Structures:
    - SAH Bounding Volume Hierarchy
* Volumentric Rendering (semi heterogeneous)
* Shading Normals
* Signed Distance Fields and Ray-Marching
    - Basic Ray-Marching encapsulated in primitive object
    - Shape configurable in XML with hierarchy of SDF primitives and operations
    - SDF Primitives:
        - Box
        - Cylinder
        - Mandelbulb fractal
        - Sphere
    - SDF Operations:
        - Combine (Union, Intersection, Subtraction) of 2 SDF shapes
        - Thicken
        - Transform

## Examples

The `features` folder contains example scenes including rendered results that show most of the features of the renderer.
In addition, there was a rendering competition at the end of the course for which a scene was created.
The results of this and credits for used assets can be found [here](https://sonic.joejoetv.de/s/cg-2324_rendering-competition).

## Credits

Lightwave was written by [Alexander Rath](https://graphics.cg.uni-saarland.de/people/rath.html), with contributions from [Ömercan Yazici](https://graphics.cg.uni-saarland.de/people/yazici.html) and [Philippe Weier](https://graphics.cg.uni-saarland.de/people/weier.html).
Many of our design decisions were heavily inspired by [Nori](https://wjakob.github.io/nori/), a great educational renderer developed by Wenzel Jakob.
We would also like to thank the teams behind our dependencies: [ctpl](https://github.com/vit-vit/CTPL), [miniz](https://github.com/richgel999/miniz), [stb](https://github.com/nothings/stb), [tinyexr](https://github.com/syoyo/tinyexr), [tinyformat](https://github.com/c42f/tinyformat), and [pcg32](https://github.com/wjakob/pcg32).
