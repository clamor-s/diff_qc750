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
import traceback

def main():
    try:
        # print version
        print "nvcamera version: %s " % nvcamera.__version__

        oGraph = nvcamera.Graph()

        oGraph.setImager(0)
        oGraph.preview()
        oGraph.still()
        oGraph.run()

        oCamera = nvcamera.Camera()

        oCamera.setAttr(nvcamera.attr_exposuretime, 0.1)

        # set focus to CAF
        oCamera.setAttr(nvcamera.attr_continuousautofocus, 1)

        oCamera.startPreview()
        oCamera.waitForEvent(12000, nvcamera.CamConst_FIRST_PREVIEW_FRAME)

        oCamera.halfpress(10000)
        oCamera.waitForEvent(12000, nvcamera.CamConst_ALGS)

        oCamera.still("/data/test.jpg")
        oCamera.waitForEvent(12000, nvcamera.CamConst_CAP_READY, nvcamera.CamConst_CAP_FILE_READY)

        oCamera.hp_release()
        oCamera.stopPreview()
        oCamera.waitForEvent(12000, nvcamera.CamConst_PREVIEW_EOS)

        oGraph.stop()
        oGraph.close()
    except Exception, err:
        traceback.print_exc()
        print "Error: %s" % err

if __name__ == '__main__':
    main()
