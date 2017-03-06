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
    oGraph.video(1280, 720, "mpeg4", "/data/nvcs_try.3gp")
    oGraph.run()

    oCamera = nvcamera.Camera()

    oCamera.startPreview()
    oCamera.waitForEvent(12000, nvcamera.CamConst_FIRST_PREVIEW_FRAME)

    print "Start video recording..."
    oCamera.startVideoRecording()

    # record video for 5 seconds
    time.sleep(5)

    print "Stop video recording..."
    oCamera.stopVideoRecording()

    oCamera.stopPreview()
    oCamera.waitForEvent(12000, nvcamera.CamConst_PREVIEW_EOS)

    oGraph.stop()
    oGraph.close()

if __name__ == '__main__':
	main()
