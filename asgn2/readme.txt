Seena Abed (ssabed)
Aladdin Ismael (aaismael)

A real thread management system would take advantage of multiple cores,
whereas this program manages threads concurrently.

One way to improve the library is to allow for multiple arguments to be passed
to a thread function.

Another improvement would be to make use of multiple cores, and run threads in parallel


Addtional details:

Our code does not pass test #27 of Nico's test suite
despite all of our lwps having a tid assigned to them