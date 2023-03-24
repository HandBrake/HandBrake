#!/usr/bin/env python

from __future__ import print_function
import collections
import sys
import json

DepEntry = collections.namedtuple('DepEntry', 'widget dep enable die hide')
dep_map = (
    DepEntry("title", "preview_frame", "none", True, False),
    DepEntry("title", "chapters_tab", "none", True, False),
    DepEntry("title", "start_point", "none", True, False),
    DepEntry("title", "end_point", "none", True, False),
    DepEntry("title", "angle", "none", True, False),
    DepEntry("title", "angle_label", "none", True, False),
    DepEntry("angle_count", "angle", "1", True, True),
    DepEntry("angle_count", "angle_label", "1", True, True),
    DepEntry("vquality_type_bitrate", "VideoAvgBitrate", "1", False, False),
    DepEntry("vquality_type_constant", "VideoQualitySlider", "1", False, False),
    DepEntry("vquality_type_constant", "VideoMultiPass", "1", True, False),
    DepEntry("vquality_type_constant", "VideoTurboMultiPass", "1", True, False),
    DepEntry("VideoFramerate", "VideoFrameratePFR", "auto", True, True),
    DepEntry("VideoFramerate", "VideoFramerateVFR", "auto", False, True),
    DepEntry("VideoMultiPass", "VideoTurboMultiPass", "1", False, False),
    DepEntry("PictureCombDetectPreset", "PictureCombDetectCustom", "custom", False, True),
    DepEntry("PictureDeinterlaceFilter", "PictureDeinterlacePreset", "off", True, True),
    DepEntry("PictureDeinterlaceFilter", "PictureDeinterlacePresetLabel", "off", True, True),
    DepEntry("PictureDeinterlaceFilter", "PictureDeinterlaceCustom", "off", True, True),
    DepEntry("PictureDeinterlacePreset", "PictureDeinterlaceCustom", "custom", False, True),
    DepEntry("PictureDeblockPreset", "PictureDeblockTune", "off|custom", True, True),
    DepEntry("PictureDeblockPreset", "PictureDeblockTuneLabel", "off|custom", True, True),
    DepEntry("PictureDeblockPreset", "PictureDeblockCustom", "custom", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoisePreset", "off", True, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoisePresetLabel", "off", True, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseTune", "nlmeans", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseTuneLabel", "nlmeans", False, True),
    DepEntry("PictureDenoiseFilter", "PictureDenoiseCustom", "off", True, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseCustom", "custom", False, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseTune", "custom", True, True),
    DepEntry("PictureDenoisePreset", "PictureDenoiseTuneLabel", "custom", True, True),
    DepEntry("PictureChromaSmoothPreset", "PictureChromaSmoothTune", "off|custom", True, True),
    DepEntry("PictureChromaSmoothPreset", "PictureChromaSmoothTuneLabel", "off|custom", True, True),
    DepEntry("PictureChromaSmoothPreset", "PictureChromaSmoothCustom", "custom", False, True),

    DepEntry("PictureSharpenFilter", "PictureSharpenPreset", "off", True, True),
    DepEntry("PictureSharpenFilter", "PictureSharpenPresetLabel", "off", True, True),
    DepEntry("PictureSharpenFilter", "PictureSharpenTune", "off", True, True),
    DepEntry("PictureSharpenFilter", "PictureSharpenTuneLabel", "off", True, True),
    DepEntry("PictureSharpenFilter", "PictureSharpenCustom", "off", True, True),
    DepEntry("PictureSharpenPreset", "PictureSharpenCustom", "custom", False, True),
    DepEntry("PictureSharpenPreset", "PictureSharpenTune", "custom", True, True),
    DepEntry("PictureSharpenPreset", "PictureSharpenTuneLabel", "custom", True, True),
    DepEntry("PictureDetelecine", "PictureDetelecineCustom", "custom", False, True),
    DepEntry("PictureColorspacePreset", "PictureColorspaceCustom", "custom", False, True),
    DepEntry("VideoEncoder", "x264FastDecode", "svt_av1|svt_av1_10bit|x264|x264_10bit", False, True),
    DepEntry("VideoEncoder", "VideoOptionExtraWindow", "svt_av1|svt_av1_10bit|x264|x264_10bit|x265|x265_10bit|x265_12bit|x265_16bit|mpeg4|mpeg2|VP8|VP9|VP9_10bit|qsv_av1|qsv_av1_10bit|qsv_h264|qsv_h265|qsv_h265_10bit", False, True),
    DepEntry("VideoEncoder", "VideoOptionExtraLabel", "svt_av1|svt_av1_10bit|x264|x264_10bit|x265|x265_10bit|x265_12bit|x265_16bit|mpeg4|mpeg2|VP8|VP9|VP9_10bit|qsv_av1|qsv_av1_10bit|qsv_h264|qsv_h265|qsv_h265_10bit", False, True),
    DepEntry("auto_name", "autoname_box", "1", False, False),
    DepEntry("CustomTmpEnable", "CustomTmpDir", "1", False, False),
    DepEntry("PresetCategory", "PresetCategoryName", "new", False, True),
    DepEntry("PresetCategory", "PresetCategoryEntryLabel", "new", False, True),
    DepEntry("DiskFreeCheck", "DiskFreeLimit", "1", False, False),
    )

def main():

    try:
        depsfile = open("widget.deps", "w")
    except Exception as err:
        print("Error: %s" % str(err), file=sys.stderr)
        sys.exit(1)

    try:
        revfile = open("widget_reverse.deps", "w")
    except Exception as err:
        print("Error: %s" % str(err), file=sys.stderr)
        sys.exit(1)

    fwd = dict()
    for ii in dep_map:
        if ii.widget in fwd:
            continue
        fwd.update({
            ii.widget: [jj.dep for jj in dep_map if jj.widget == ii.widget]
        })
    json.dump(fwd, depsfile, indent=4)

    rev = dict()
    for ii in dep_map:
        if ii.dep in rev:
            continue
        rev.update({
            ii.dep: [
                list([jj.widget, jj.enable, jj.die, jj.hide])
                for jj in dep_map if ii.dep == jj.dep
            ]
        })
    json.dump(rev, revfile, indent=4)

main()

