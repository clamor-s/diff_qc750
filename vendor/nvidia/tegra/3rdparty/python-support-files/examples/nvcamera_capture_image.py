# Copyright (c) 2011 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
#
from optparse import OptionParser
from optparse import OptionValueError
import traceback
import nvcamera
import time
import os

def main():
    try:
        # parse arguments
        usage = "Usage: %prog <options>\nThe script captures image/video with AE, AWB and AF set to Auto by default."
        parser = OptionParser(usage)

        parser.add_option('-c', '--imager', dest='imager_id', default=0, type="int", metavar="IMAGER_ID",
                    help = 'select camera, 0 for rear facing, 1 for front facing')

        parser.add_option('-p', '--preview', dest='preview', default=(-1, -1), type="int", metavar="PREVIEW_RES",
                    nargs=2, help = 'set preview resolution (--preview=<width> <height>)\n\
                                     , default is maximum supported preview resolution')

        parser.add_option('-w', '--preview_wait', dest='preview_wait', default=0, type="int", metavar="PREVIEW_WAIT",
                    help = 'number of seconds to wait after starting preview')

        parser.add_option('-s', '--still', dest='still', default=(-1, -1), type="int", metavar="STILL_RES",
                    nargs=2, help = 'capture still image at specified resolution (--still=<width> <height>)')

        parser.add_option('-i', '--iso', dest='iso', default=-1, type="int", metavar="ISO",
                    help = 'set iso value, default is Auto')

        parser.add_option('-e', '--exp', dest='exp_time', default=-1, type="float", metavar="EXP_TIME",
                    help = 'exposure time in seconds, default is Auto')

        parser.add_option('-g', '--gain', dest='gain', default=-1, type="float", metavar="GAIN",
                    help = 'gain, default is Auto')

        parser.add_option('-f', '--focus', dest='focus', default=-1, type="int", metavar="FOCUS",
                    help = 'focus position, , default is Auto')

        parser.add_option('-n', '--name', dest='fname', default="nvcs_test", type="str", metavar="FILE_NAME",
                    help = 'name of the file, default is nvcs_test')

        parser.add_option('-r', '--raw', dest='concurrent_raw', action='store_true', metavar="CONCURRENT_RAW",
                    default = False,
                    help = 'captures concurrent raw image')

        parser.add_option('--caf', dest='continuous_af', action='store_true', metavar="CONTINUOUS_AF",
                    default = False,
                    help = 'sets continuous auto focus mode, , default is False')

        parser.add_option('-o', '--outdir', dest='output_dir',  type="str", metavar="OUTPUT_DIR",
                    default = '/data/raw',
                    help = 'sets output directory for the image(s), default is /data/raw')

        parser.add_option('--video', dest='video', default=(-1, -1), type="int", metavar="VIDEO_RES",
                    nargs=2, help = 'capture video at specified resolution (--video=<width> <height>)')

        parser.add_option('--video-enc', dest='video_enc', default="Mpeg4", type="string", metavar="VIDEO_ENC",
                    help = 'set video encoder, default is Mpeg4 encoder')

        parser.add_option('--video-time', dest='video_time', default=5, type="int", metavar="VIDEO_TIME",
                    help = 'set video recording time in seconds, default is 5 seconds')

        parser.add_option('-v', '--version', dest='version', action='store_true', metavar="VERSION",
                    default = False,
                    help = 'print nvcamera module version')

        # parse the command line arguments
        (options, args) = parser.parse_args()

        # print nvcamera module version
        if (options.version):
            print "nvcamera version: %s" %nvcamera.__version__
            return

        # create and run camera graph
        oGraph = nvcamera.Graph()

        oGraph.setImager(options.imager_id)

        # set to default resolutions unless user has specified
        if(options.preview[0] == -1):
            oGraph.preview()
        else:
            oGraph.preview(options.preview[0], options.preview[1])

        # set to default resolutions unless user has specified
        if(options.still[0] == -1):
            oGraph.still()
        else:
            oGraph.still(options.still[0], options.still[1])

        videoFileName = options.output_dir + "/" + options.fname + ".3gp"
        if(options.video[0] == -1):
            oGraph.video(640, 480, options.video_enc, videoFileName)
        else:
            oGraph.video(options.video[0], options.video[1], options.video_enc, videoFileName)

        oGraph.run()

        oCamera = nvcamera.Camera()
        oCamera.setAttr(nvcamera.attr_pauseaftercapture, 1)

        rawImageDir = "/data/raw"
        if(options.concurrent_raw):
            oCamera.setAttr(nvcamera.attr_concurrentrawdumpflag, 7)

            # create dirctory /data/raw because drive dumps
            # raw images in this directory
            if(not os.path.exists(rawImageDir)):
                os.mkdir(rawImageDir)

        if(not os.path.exists(options.output_dir)):
            os.mkdir(options.output_dir)

        if(options.concurrent_raw):
            rawFilesList = os.listdir(rawImageDir)

        oCamera.startPreview()
        oCamera.waitForEvent(12000, nvcamera.CamConst_FIRST_PREVIEW_FRAME)

        # set the required attributes
        oCamera.setAttr(nvcamera.attr_iso, options.iso)
        if (options.gain != -1):
            oCamera.setAttr(nvcamera.attr_bayergains, [options.gain, options.gain, options.gain, options.gain])
        oCamera.setAttr(nvcamera.attr_exposuretime, options.exp_time)

        hasFocuser = True
        try:
            oCamera.setAttr(nvcamera.attr_focuspos, options.focus)
        except nvcamera.NvCameraException, err:
            if (err.value == nvcamera.NvError_NotSupported):
                hasFocuser = False
                if (options.focus != -1 or options.continuous_af):
                    raise OptionValueError("ERROR: Focuser is not supported..can't use -f, --focus or --caf")
                else:
                    pass
            else:
                raise

        if(options.continuous_af and hasFocuser):
            oCamera.setAttr(nvcamera.attr_continuousautofocus, 1)

        time.sleep(options.preview_wait);

        if(options.still[0] != -1):
            # ---- IMAGE CAPTURE START ---- #
            oCamera.halfpress(5000)
            oCamera.waitForEvent(12000, nvcamera.CamConst_ALGS)

            # capture an image
            jpegFileName = options.output_dir + "/" + options.fname + ".jpg"
            oCamera.still(jpegFileName)

            oCamera.waitForEvent(12000, nvcamera.CamConst_CAP_READY, nvcamera.CamConst_CAP_FILE_READY)

            oCamera.hp_release()
            # ---- IMAGE CAPTURE STOP ---- #

        if(options.video[0] != -1):
            # ---- VIDEO CAPTURE START ---- #
            oCamera.startVideoRecording()
            time.sleep(options.video_time)
            oCamera.stopVideoRecording()
            # ---- VIDEO CAPTURE STOP ---- #

        oCamera.stopPreview()
        oCamera.waitForEvent(12000, nvcamera.CamConst_PREVIEW_EOS)

        # stop and close the graph
        oGraph.stop()
        oGraph.close()

        newList = os.listdir(rawImageDir)
        if(options.concurrent_raw):
            for key in newList:
                if key not in rawFilesList:
                    if 'nvraw' in key:
                        os.rename(os.path.join(rawImageDir, key), os.path.join(options.output_dir, options.fname + ".nvraw"))
                        break
    except Exception, err:
        oGraph.stop()
        oGraph.close()

        print traceback.print_exc()
        print err

if __name__ == '__main__':
    main()
