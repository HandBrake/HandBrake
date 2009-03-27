#! /usr/bin/python

import collections
import plistlib

DepMap = collections.namedtuple('DepMap', 'widget dep enable die hide')
dep_map = (
	DepMap("title", "queue_add", "none", True, False),
	DepMap("title", "queue_add_menu", "none", True, False),
	DepMap("title", "show_picture", "none", True, False),
	DepMap("title", "show_preview_menu", "none", True, False),
	DepMap("title", "preview_frame", "none", True, False),
	DepMap("title", "picture_label", "none", True, False),
	DepMap("title", "picture_tab", "none", True, False),
	DepMap("title", "chapters_label", "none", True, False),
	DepMap("title", "chapters_tab", "none", True, False),
	DepMap("title", "title", "none", True, False),
	DepMap("title", "start_chapter", "none", True, False),
	DepMap("title", "end_chapter", "none", True, False),
	DepMap("vquality_type_bitrate", "VideoAvgBitrate", "TRUE", False, False),
	DepMap("vquality_type_target", "VideoTargetSize", "TRUE", False, False),
	DepMap("vquality_type_constant", "VideoQualitySlider", "TRUE", False, False),
	DepMap("vquality_type_constant", "constant_rate_factor", "TRUE", False, False),
	DepMap("vquality_type_constant", "VideoTwoPass", "TRUE", True, False),
	DepMap("vquality_type_constant", "VideoTurboTwoPass", "TRUE", True, False),
	DepMap("VideoTwoPass", "VideoTurboTwoPass", "TRUE", False, False),
	DepMap("FileFormat", "Mp4LargeFile", "mp4|m4v", False, True),
	DepMap("FileFormat", "Mp4HttpOptimize", "mp4|m4v", False, True),
	DepMap("FileFormat", "Mp4iPodCompatible", "mp4|m4v", False, True),
	DepMap("PictureDecomb", "PictureDeinterlace", "none", False, False),
	DepMap("PictureDecomb", "PictureDeinterlaceCustom", "none", False, True),
	DepMap("PictureDeinterlace", "PictureDeinterlaceCustom", "custom", False, True),
	DepMap("PictureDenoise", "PictureDenoiseCustom", "custom", False, True),
	DepMap("PictureDecomb", "PictureDecombCustom", "custom", False, True),
	DepMap("PictureDetelecine", "PictureDetelecineCustom", "custom", False, True),
	DepMap("PictureAutoCrop", "PictureTopCrop", "FALSE", False, False),
	DepMap("PictureAutoCrop", "PictureBottomCrop", "FALSE", False, False),
	DepMap("PictureAutoCrop", "PictureLeftCrop", "FALSE", False, False),
	DepMap("PictureAutoCrop", "PictureRightCrop", "FALSE", False, False),
	DepMap("autoscale", "scale_width", "FALSE", False, False),
	DepMap("autoscale", "scale_height", "FALSE", False, False),
	DepMap("anamorphic", "PictureKeepRatio", "FALSE", False, False),
	## "CHECK" is a dummy value that forces scale_height deps to
	## be re-evaluated whenever anamorphic changes
	DepMap("anamorphic", "scale_height", "CHECK", True, False),
	DepMap("PictureKeepRatio", "scale_height", "FALSE", False, False),
	DepMap("VideoEncoder", "x264_tab", "x264", False, False),
	DepMap("VideoEncoder", "x264_tab_label", "x264", False, False),
	DepMap("VideoEncoder", "Mp4iPodCompatible", "x264", False, False),
	DepMap("AudioEncoder", "AudioBitrate", "ac3|dts", True, False),
	DepMap("AudioEncoder", "AudioSamplerate", "ac3|dts", True, False),
	DepMap("AudioEncoder", "AudioMixdown", "ac3|dts", True, False),
	DepMap("AudioEncoder", "AudioTrackDRCSlider", "ac3|dts", True, False),
	DepMap("x264_bframes", "x264_weighted_bframes", "0", True, False),
	DepMap("x264_bframes", "x264_bpyramid", "<2", True, False),
	DepMap("x264_bframes", "x264_direct", "0", True, False),
	DepMap("x264_bframes", "x264_b_adapt", "0", True, False),
	DepMap("x264_refs", "x264_mixed_refs", "<2", True, False),
	DepMap("x264_cabac", "x264_trellis", "TRUE", False, False),
	DepMap("x264_subme", "x264_psy_rd", "<6", True, False),
	DepMap("x264_subme", "x264_psy_trell", "<6", True, False),
	DepMap("x264_cabac", "x264_psy_trell", "TRUE", False, False),
	DepMap("x264_trellis", "x264_psy_trell", "0", True, False),
	DepMap("ChapterMarkers", "chapters_list", "TRUE", False, False),
	DepMap("use_source_name", "chapters_in_destination", "TRUE", False, False),
	DepMap("use_source_name", "title_no_in_destination", "TRUE", False, False),
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

