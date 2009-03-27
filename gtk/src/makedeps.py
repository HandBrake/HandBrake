#! /usr/bin/python

import collections
import plistlib

DepEntry = collections.namedtuple('DepEntry', 'widget dep enable die hide')
dep_map = (
	DepEntry("title", "queue_add", "none", True, False),
	DepEntry("title", "queue_add_menu", "none", True, False),
	DepEntry("title", "show_picture", "none", True, False),
	DepEntry("title", "show_preview_menu", "none", True, False),
	DepEntry("title", "preview_frame", "none", True, False),
	DepEntry("title", "picture_label", "none", True, False),
	DepEntry("title", "picture_tab", "none", True, False),
	DepEntry("title", "chapters_label", "none", True, False),
	DepEntry("title", "chapters_tab", "none", True, False),
	DepEntry("title", "title", "none", True, False),
	DepEntry("title", "start_chapter", "none", True, False),
	DepEntry("title", "end_chapter", "none", True, False),
	DepEntry("vquality_type_bitrate", "VideoAvgBitrate", "TRUE", False, False),
	DepEntry("vquality_type_target", "VideoTargetSize", "TRUE", False, False),
	DepEntry("vquality_type_constant", "VideoQualitySlider", "TRUE", False, False),
	DepEntry("vquality_type_constant", "constant_rate_factor", "TRUE", False, False),
	DepEntry("vquality_type_constant", "VideoTwoPass", "TRUE", True, False),
	DepEntry("vquality_type_constant", "VideoTurboTwoPass", "TRUE", True, False),
	DepEntry("VideoTwoPass", "VideoTurboTwoPass", "TRUE", False, False),
	DepEntry("FileFormat", "Mp4LargeFile", "mp4|m4v", False, True),
	DepEntry("FileFormat", "Mp4HttpOptimize", "mp4|m4v", False, True),
	DepEntry("FileFormat", "Mp4iPodCompatible", "mp4|m4v", False, True),
	DepEntry("PictureDecomb", "PictureDeinterlace", "none", False, False),
	DepEntry("PictureDecomb", "PictureDeinterlaceCustom", "none", False, True),
	DepEntry("PictureDeinterlace", "PictureDeinterlaceCustom", "custom", False, True),
	DepEntry("PictureDenoise", "PictureDenoiseCustom", "custom", False, True),
	DepEntry("PictureDecomb", "PictureDecombCustom", "custom", False, True),
	DepEntry("PictureDetelecine", "PictureDetelecineCustom", "custom", False, True),
	DepEntry("PictureAutoCrop", "PictureTopCrop", "FALSE", False, False),
	DepEntry("PictureAutoCrop", "PictureBottomCrop", "FALSE", False, False),
	DepEntry("PictureAutoCrop", "PictureLeftCrop", "FALSE", False, False),
	DepEntry("PictureAutoCrop", "PictureRightCrop", "FALSE", False, False),
	DepEntry("autoscale", "scale_width", "FALSE", False, False),
	DepEntry("autoscale", "scale_height", "FALSE", False, False),
	DepEntry("anamorphic", "PictureKeepRatio", "FALSE", False, False),
	## "CHECK" is a dummy value that forces scale_height deps to
	## be re-evaluated whenever anamorphic changes
	DepEntry("anamorphic", "scale_height", "CHECK", True, False),
	DepEntry("PictureKeepRatio", "scale_height", "FALSE", False, False),
	DepEntry("VideoEncoder", "x264_tab", "x264", False, False),
	DepEntry("VideoEncoder", "x264_tab_label", "x264", False, False),
	DepEntry("VideoEncoder", "Mp4iPodCompatible", "x264", False, False),
	DepEntry("AudioEncoder", "AudioBitrate", "ac3|dts", True, False),
	DepEntry("AudioEncoder", "AudioSamplerate", "ac3|dts", True, False),
	DepEntry("AudioEncoder", "AudioMixdown", "ac3|dts", True, False),
	DepEntry("AudioEncoder", "AudioTrackDRCSlider", "ac3|dts", True, False),
	DepEntry("x264_bframes", "x264_weighted_bframes", "0", True, False),
	DepEntry("x264_bframes", "x264_bpyramid", "<2", True, False),
	DepEntry("x264_bframes", "x264_direct", "0", True, False),
	DepEntry("x264_bframes", "x264_b_adapt", "0", True, False),
	DepEntry("x264_refs", "x264_mixed_refs", "<2", True, False),
	DepEntry("x264_cabac", "x264_trellis", "TRUE", False, False),
	DepEntry("x264_subme", "x264_psy_rd", "<6", True, False),
	DepEntry("x264_subme", "x264_psy_trell", "<6", True, False),
	DepEntry("x264_cabac", "x264_psy_trell", "TRUE", False, False),
	DepEntry("x264_trellis", "x264_psy_trell", "0", True, False),
	DepEntry("ChapterMarkers", "chapters_list", "TRUE", False, False),
	DepEntry("use_source_name", "chapters_in_destination", "TRUE", False, False),
	DepEntry("use_source_name", "title_no_in_destination", "TRUE", False, False),
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

