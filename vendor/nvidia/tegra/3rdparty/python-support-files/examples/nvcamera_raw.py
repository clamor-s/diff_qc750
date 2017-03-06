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

def main():
    # print version
    print "nvcamera version: %s " % nvcamera.__version__

    oGraph = nvcamera.Graph()

    oGraph.setImager(0)
    oGraph.preview()
    oGraph.still()
    oGraph.run()

    oCamera = nvcamera.Camera()
    oCamera.setAttr(nvcamera.attr_concurrentrawdumpflag, 7)
    oCamera.setAttr(nvcamera.attr_pauseaftercapture, 1)

    # set 3A to Auto
    oCamera.setAttr(nvcamera.attr_whitebalance, "Auto")
    oCamera.setAttr(nvcamera.attr_exposuretime, -1)
    oCamera.setAttr(nvcamera.attr_continuousautofocus, 1)

    # create dirctory /data/raw because drive dumps
    # raw images in this directory
    if(not os.path.exists("/data/raw")):
        os.mkdir("/data/raw")

    oCamera.startPreview()
    oCamera.waitForEvent(12000, nvcamera.CamConst_FIRST_PREVIEW_FRAME)

    # set the flash mode to Auto, which will help assist the
    # halfpress to converge using flash and then do a strobe
    # during the capture
    oCamera.setAttr(nvcamera.attr_flashmode, "Auto")

    oCamera.halfpress(10000)
    oCamera.waitForEvent(12000, nvcamera.CamConst_ALGS)

    oCamera.still("/data/test.jpg")
    oCamera.waitForEvent(12000, nvcamera.CamConst_CAP_READY, nvcamera.CamConst_CAP_FILE_READY)

    oCamera.hp_release()
    oCamera.stopPreview()
    oCamera.waitForEvent(12000, nvcamera.CamConst_PREVIEW_EOS)

    oGraph.stop()
    oGraph.close()

if __name__ == '__main__':
	main()
