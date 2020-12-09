# ASI Camera Boost

A wrapper for libASICamera2 library that improves performance at higher FPS.

# What is the problem in the original library?
When running the library on Raspberry PI 4, the problem becomes reaching the declared maximum FPS, e.g. for the ASI178 camera.
The reasons are:
- copying the same frame several times
- synchronous communication between frames on USB
- problems with handling bad frames

# What has been improved?
After an in-depth analysis of the original library, frame copying has been completely removed in libASICamera2Boost. In addition, more bulks transfers to USB ensure more stable operation.

# What is the difference in use?
Just swap the libraries!
If you want to get rid of unnecessary data copying from buffer to buffer, be sure to use ASIGetVideoDataPointer function.
With this function, you will get an address to the frame buffer. The data is available until the next function call. You can edit the data in the buffer for image transformations.

# Building

TODO

