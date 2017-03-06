#
# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
import nvcamera
import time
import os
import nvrawfile
import array
import traceback

def main():
    NUMBER_OF_ITERATION = 10

    # change filename to point to correct raw file
    filename = "/data/raw/IMG_20000223_224642.nvraw"

    header = None
    # ---- Read and load raw file ---- #
    nvrf = nvrawfile.NvRawFile()
    if nvrf.readFile(filename):
        header = nvrf.makeLegacyHeader()

        try:
            # print version
            print "nvcamera version: %s " % nvcamera.__version__

            # ---- create OpenMax graph ---- #
            oGraph = nvcamera.Graph()

            oGraph.setImager("host")
            oGraph.still(nvrf._width, nvrf._height)

            oGraph.run()

            # ---- Pass host images through ISP ---- #
            oCamera = nvcamera.Camera()

            # disable ANR until the last execution
            oCamera.setAttr(nvcamera.attr_anr, 0)

            for i in range(NUMBER_OF_ITERATION):
                # set the iso from raw file
                # set the exposure time from the raw file
                print "Setting the exposure time to: %f" % nvrf._exposureTime
                print "Setting the iso to: %d" % nvrf._iso

                oCamera.setAttr(nvcamera.attr_iso, nvrf._iso)
                oCamera.setAttr(nvcamera.attr_exposuretime, nvrf._exposureTime)

                if (i == NUMBER_OF_ITERATION - 1):
                    # enable ANR for the last iteration
                    oCamera.setAttr(nvcamera.attr_anr, 1)

                # pass the raw image header, data and iteration information
                oCamera.setRawImage(header, nvrf._pixelData, i)
                oCamera.still("/data/test_%d.jpg" % i)
                oCamera.waitForEvent(12000, nvcamera.CamConst_CAP_READY)

                # wait here for ALL_CAPTURE_DONE event
                oCamera.waitForEvent(12000, nvcamera.CamConst_ALL_CAPTURE_DONE)

        except Exception, err:
            traceback.print_exc()

        # ---- stop the camera graph ---- #
        oGraph.stop()
        oGraph.close()
    else:
        print "ERROR: could not open file"


if __name__ == '__main__':
    main()
