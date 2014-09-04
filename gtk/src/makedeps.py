#! /usr/bin/python

import collections
import plistlib
import sys

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
    DepEntry("use_dvdnav", "angle", "FALSE", True, True),
    DepEntry("use_dvdnav", "angle_label", "FALSE", True, True),
    DepEntry("angle_count", "angle", "1", True, True),
    DepEntry("angle_count", "angle_label", "1", True, True),
    DepEntry("vquality_type_bitrate", "VideoAvgBitrate", "TRUE", False, False),
    DepEntry("vquality_type_constant", "VideoQualitySlider", "TRUE", False, False),
    DepEntry("vquality_type_constant", "VideoTwoPass", "TRUE", True, False),
    DepEntry("vquality_type_constant", "VideoTurboTwoPass", "TRUE", True, False),
    DepEntry("VideoFramerate", "VideoFrameratePFR", "source", True, True),
    DepEntry("VideoFramerate", "VideoFramerateVFR", "source", False, True),
    DepEntry("VideoTwoPass", "VideoTurboTwoPass", "TRUE", False, False),
    DepEntry("PictureDecombDeinterlace", "PictureDeinterlace", "TRUE", True, True),
    DepEntry("PictureDecombDeinterlace", "PictureDeinterlaceCustom", "TRUE", True, True),
    DepEntry("PictureDecombDeinterlace", "PictureDeinterlaceLabel", "TRUE", True, True),
    DepEntry("PictureDecombDeinterlace", "PictureDecomb", "FALSE", True, True),
    DepEntry("PictureDecombDeinterlace", "PictureDecombCustom", "FALSE", True, True),
    DepEntry("PictureDecombDeinterlace", "PictureDecombLabel", "FALSE", True, True),
    DepEntry("PictureDeinterlace", "PictureDeinterlaceCustom", "custom", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoisePreset", "off", True, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoisePresetLabel", "off", True, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseTune", "nlmeans", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseTuneLabel", "nlmeans", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseCustom", "off", True, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseCustom", "custom", False, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseTune", "custom", True, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseTuneLabel", "custom", True, True),
    DepEntry("PictureDecomb", "PictureDecombCustom", "custom", False, True),
    DepEntry("PictureDetelecine", "PictureDetelecineCustom", "custom", False, True),
    DepEntry("PictureWidthEnable", "PictureWidth", "TRUE", False, False),
    DepEntry("PictureHeightEnable", "PictureHeight", "TRUE", False, False),
    DepEntry("PictureAutoCrop", "PictureTopCrop", "FALSE", False, False),
    DepEntry("PictureAutoCrop", "PictureBottomCrop", "FALSE", False, False),
    DepEntry("PictureAutoCrop", "PictureLeftCrop", "FALSE", False, False),
    DepEntry("PictureAutoCrop", "PictureRightCrop", "FALSE", False, False),
    DepEntry("AudioTrackQualityEnable", "AudioBitrate", "TRUE", True, True),
    DepEntry("AudioTrackQualityEnable", "AudioTrackQualityBox", "FALSE", True, True),
    DepEntry("AudioEncoder", "AudioBitrateLabel", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioBitrate", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioSamplerateLabel", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioSamplerate", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioMixdownLabel", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioMixdown", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioTrackGainLabel", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioTrackGainSlider", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("AudioEncoder", "AudioTrackGainValue", "copy:mp3|copy:aac|copy:ac3|copy:dts|copy:dtshd", True, False),
    DepEntry("x264_bframes", "x264_bpyramid", "<2", True, False),
    DepEntry("x264_bframes", "x264_direct", "0", True, False),
    DepEntry("x264_bframes", "x264_b_adapt", "0", True, False),
    DepEntry("x264_subme", "x264_psy_rd", "<6", True, False),
    DepEntry("x264_subme", "x264_psy_trell", "<6", True, False),
    DepEntry("x264_trellis", "x264_psy_trell", "0", True, False),
    DepEntry("VideoEncoder", "VideoPresetSlider", "x264|x265", False, True),
    DepEntry("VideoEncoder", "VideoPresetLabel", "x264|x265", False, True),
    DepEntry("VideoEncoder", "VideoTune", "x264|x265", False, True),
    DepEntry("VideoEncoder", "VideoTuneLabel", "x264|x265", False, True),
    DepEntry("VideoEncoder", "VideoProfile", "x264|x265", False, True),
    DepEntry("VideoEncoder", "VideoProfileLabel", "x264|x265", False, True),
    DepEntry("VideoEncoder", "VideoLevel", "x264|x265", False, True),
    DepEntry("VideoEncoder", "VideoLevelLabel", "x264|x265", False, True),
    DepEntry("VideoEncoder", "x264FastDecode", "x264", False, True),
    DepEntry("VideoEncoder", "x264UseAdvancedOptions", "x264", False, True),
    DepEntry("HideAdvancedVideoSettings", "x264UseAdvancedOptions", "TRUE", True, True),
    DepEntry("VideoEncoder", "VideoOptionExtraWindow", "x264|x265|mpeg4|mpeg2|VP8", False, True),
    DepEntry("VideoEncoder", "VideoOptionExtraLabel", "x264|x265|mpeg4|mpeg2|VP8", False, True),
    DepEntry("x264UseAdvancedOptions", "VideoSettingsTable", "TRUE", True, False),
    DepEntry("VideoEncoder", "x264_box", "x264", False, True),
    DepEntry("x264UseAdvancedOptions", "x264_box", "FALSE", True, False),
    DepEntry("auto_name", "autoname_box", "TRUE", False, False),
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
    plistlib.writePlist(top, depsfile)

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
    plistlib.writePlist(top, revfile)

main()

