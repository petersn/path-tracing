A simple unidirectional Monte Carlo path tracer that's not useful to anyone.
Uses k-d trees for acceleration.

The following 1920x1080 image of Suzanne subdivided to form a scene with 1.1 million triangles took just under 33 minutes, with 1000 samples per pixel.
It is lit by three lights with no ambient (or background) light, with diffuse bounces (global illumination) being the only thing lighting the underside of the model.
The depth of field was computed by sampling a Gaussian aperture.

![Simple render](https://raw.githubusercontent.com/petersn/path-tracing/master/demos/dof_passes1000.png)

