The purpose of these scripts is to test NVCS (Nvidia Camera Scripting) interface.
NVCS is a python scripting interface for camera. We can either run group of sanity
tests or pick and choose tests that we want to run. The usage and the description
of the tests and scripts are given below.

Usage: nvcstestmain.py <options>

Options:
  -h, --help    show this help message and exit
  -s            Run Sanity tests
  -t TEST_NAME  Test name to execute
  -d TEST_NAME  Disable the test
  -i IMAGER_ID  set imager id. 0 for rear facing, 1 for front facing


==============================================
options description:

-s
    runs the sanity tests
        "jpeg_capture"
        "raw_capture"
        "concurrent_raw"
        "exposuretime"
        "iso"
        "focuspos"

-t TEST_NAME

    runs the TEST_NAME. Multiple -t arguments are supported.

-d TEST_NAME

    disables the TEST_NAME. multiple -d arguments are supported.

==============================================
Result:
    Result images and logs are stored under /data/NVCSTest directory.

    Each test will have directory under /data/NVCSTest, which should have
    result images for that test

    Summary log is saved at /data/NVCSTest/summarylog.txt


==============================================
List of tests:

"jpeg_capture"
    captures a jpeg image

"raw_capture"
    captures a raw image and checks for the existence of the raw image.

"concurrent_raw"
    captures a jpeg+raw image concurrently. Checks the max pixel value (should be <= 1023)

"exposuretime"
    captures concurrent jpeg+raw images, setting the exposuretime values [0.5, 1.0, 5, 10, 37.5, 100, 200]ms. Checks the exposure value in the header
    and value returned from the get attribute function

"gain"
    captures concurrent jpeg+raw images, setting the gains within supported gain range linearly. Checks the sensor gains value in the raw header
    and value returned from the get attribute function.

"focuspos"
    captures concurrent jpeg+raw images, setting the focuspos in the range [0, 1000] in the intervals of 100. Checks the focuspos in the raw header
    and value returned from the get attribute function.

"multiple_raw"
    captures 100 concurrent jpeg+raw images. Checks for the existence of raw images and the max pixel value (should be <= 1023)

"linearity"
    captures a series of jpeg+raw images. Linearily increases gain and exposure values and analyzes the raw images for correlating linear pixel values.

"sharpness"
    captures concurrent raw+jpeg images from min to max focus positions and checks for the change in sharpness with the change in focus position.
    Performs the same steps going from max to min focus position and also checks if the sharpenss measurements are the same as previous iteration for the same focus position. 

==============================================
VERSION HISTORY

4.0.0
    Added sharpness test. focuspos test is changed to use physical range of focuser.
3.1.0
    Added --odm option for ODM conformance tests.  Linearity test is ran multiple times to compensate for frame corruption.  Added missing math module.
    Refactored the code for better maintainability
3.0.0
    Refactored the code for better maintainability
2.0.0
    Linearity test is added.  NvCSTestConfiguration class is added to help capture images with specific exposure and gain values.
