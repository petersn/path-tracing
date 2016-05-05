A simple unidirectional Monte Carlo path tracer that's not useful to anyone.
Uses k-d trees for acceleration.

The following 1920x1080 image of Suzanne subdivided to form a scene with 1.1 million triangles took just under 33 minutes, with 300 samples, 2.5 billion rays cast, and 27.6 billion Möller-Trumbore evaluations.
It is lit by three lights with no ambient (or background) light, with diffuse bounces (global illumination) being the only thing lighting the underside of the model.
The depth of field was computed by sampling a Gaussian aperture.
There is no normal (Phong) interpolation used -- the model is simply so subdivided that you can't tell that the surface normals are discontinuous.

![Simple render](https://raw.githubusercontent.com/petersn/path-tracing/master/demos/dof_passes300.png)

