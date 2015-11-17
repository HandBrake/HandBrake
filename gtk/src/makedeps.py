#! /usr/bin/python

import collections
import sys
import json

DepEntry = collections.namedtuple('DepEntry', 'widget dep enable die hide')
dep_map = (
    DepEntry("title", "queue_add", "none", True, False),
    DepEntry("title", "queue_add_menu", "none", True, False),
    DepEntry("title", "queue_add_multiple_menu", "none", True, False),
    DepEntry("title", "preview_frame", "none", True, False),
    DepEntry("title", "picture_summary", "none", True, False),
    DepEntry("title", "picture_summary2", "none", True, False),
    DepEntry("title", "chapters_tab", "none", True, False),
    DepEntry("title", "start_point", "none", True, False),
    DepEntry("title", "end_point", "none", True, False),
    DepEntry("title", "angle", "none", True, False),
    DepEntry("title", "angle_label", "1", True, False),
    DepEntry("use_dvdnav", "angle", "0", True, True),
    DepEntry("use_dvdnav", "angle_label", "0", True, True),
    DepEntry("angle_count", "angle", "1", True, True),
    DepEntry("angle_count", "angle_label", "1", True, True),
    DepEntry("vquality_type_bitrate", "VideoAvgBitrate", "1", False, False),
    DepEntry("vquality_type_constant", "VideoQualitySlider", "1", False, False),
    DepEntry("vquality_type_constant", "VideoTwoPass", "1", True, False),
    DepEntry("vquality_type_constant", "VideoTurboTwoPass", "1", True, False),
    DepEntry("VideoFramerate", "VideoFrameratePFR", "auto", True, True),
    DepEntry("VideoFramerate", "VideoFramerateVFR", "auto", False, True),
    DepEntry("VideoTwoPass", "VideoTurboTwoPass", "1", False, False),
    DepEntry("PictureDeinterlaceFilter", "PictureDeinterlacePreset", "off", True, True),
    DepEntry("PictureDeinterlaceFilter", "PictureDeinterlacePresetLabel", "off", True, True),
    DepEntry("PictureDeinterlaceFilter", "PictureDeinterlaceCustom", "off", True, True),
    DepEntry("PictureDeinterlacePreset", "PictureDeinterlaceCustom", "custom", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoisePreset", "off", True, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoisePresetLabel", "off", True, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseTune", "nlmeans", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseTuneLabel", "nlmeans", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseCustom", "off", True, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseCustom", "custom", False, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseTune", "custom", True, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseTuneLabel", "custom", True, True),
    DepEntry("PictureDetelecine", "PictureDetelecineCustom", "custom", False, True),
    DepEntry("PictureWidthEnable", "PictureWidth", "1", False, False),
    DepEntry("PictureHeightEnable", "PictureHeight", "1", False, False),
    DepEntry("PictureAutoCrop", "PictureTopCrop", "0", False, False),
    DepEntry("PictureAutoCrop", "PictureBottomCrop", "0", False, False),
    DepEntry("PictureAutoCrop", "PictureLeftCrop", "0", False, False),
    DepEntry("PictureAutoCrop", "PictureRightCrop", "0", False, False),
    DepEntry("x264_bframes", "x264_bpyramid", "<2", True, False),
    DepEntry("x264_bframes", "x264_direct", "0", True, False),
    DepEntry("x264_bframes", "x264_b_adapt", "0", True, False),
    DepEntry("x264_subme", "x264_psy_rd", "<6", True, False),
    DepEntry("x264_subme", "x264_psy_trell", "<6", True, False),
    DepEntry("x264_trellis", "x264_psy_trell", "0", True, False),
    DepEntry("VideoEncoder", "x264FastDecode", "x264|x264_10bit", False, True),
    DepEntry("VideoEncoder", "x264UseAdvancedOptions", "x264|x264_10bit", False, True),
    DepEntry("HideAdvancedVideoSettings", "x264UseAdvancedOptions", "1", True, True),
    DepEntry("VideoEncoder", "VideoOptionExtraWindow", "x264|x264_10bit|x265|x265_10bit|x265_12bit|x265_16bit|mpeg4|mpeg2|VP8", False, True),
    DepEntry("VideoEncoder", "VideoOptionExtraLabel", "x264|x264_10bit|x265|x265_10bit|x265_12bit|x265_16bit|mpeg4|mpeg2|VP8", False, True),
    DepEntry("x264UseAdvancedOptions", "VideoSettingsTable", "1", True, False),
    DepEntry("VideoEncoder", "x264_box", "x264|x264_10bit", False, True),
    DepEntry("x264UseAdvancedOptions", "x264_box", "0", True, False),
    DepEntry("auto_name", "autoname_box", "1", False, False),
    )

def main():

    try:
        depsfile = open("widget.deps", "w")
    except Exception, err:
        print >> sys.stderr, ( "Error: %s"  % str(err) )
        sys.exit(1)

    try:
        revfile = open("widget_reverse.deps", "w")
    except Exception, err:
        print >> sys.stderr, ( "Error: %s"  % str(err))
        sys.exit(1)

    top = dict()
    for ii in dep_map:
        if ii.widget in top:
            continue
        deps = list()
        for jj in dep_map:
            if jj.widget == ii.widget:
                deps.append(jj.dep)
        top[ii.widget] = deps
    json.dump(top, depsfile, indent=4)

    top = dict()
    for ii in dep_map:
        if ii.dep in top:
            continue
        deps = list()
        for jj in dep_map:
            if ii.dep == jj.dep:
                rec = list()
                rec.append(jj.widget)
                rec.append(jj.enable)
                rec.append(jj.die)
                rec.append(jj.hide)
                deps.append(rec)
        top[ii.dep] = deps
    json.dump(top, revfile, indent=4)

main()

