#!/usr/bin/python

import sys, string, os, subprocess, time
from multiprocessing import Process


##### This script uses as input a 'script file'
#####  each line of the script file is a handbrakecli.exe options line (without the handbrakecli.exe)
#####  this is read in, each line is executed and the output results are parsed, pulling out fps, and Global PSNR

inputscript = 'handbrake_test.txt'


#### input image files

###inputimagefiles = ['x264.avi']
inputimagefiles = ['blueangels_20m.m2ps','blueangels.m2ts','BigBuckBunny.VOB']
#### output directory
outdir = 'output/'

#### output CSV file
outcsv = 'testresults.csv'

#### limit to the number of frames processed
endframe = 2000


def run_handbrake(execline, testname):
	#os.chdir(handbrakedir)
	#process = subprocess.Popen(execline, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
	proc = subprocess.Popen(execline, bufsize=-1, stderr=subprocess.PIPE, shell=True)
	
	[sout, serr] =  proc.communicate()
	
	# stdout has the percentage complete for the given task
	# stderr has all of the output for data collection
	
	lines = serr.splitlines();
	
	psnr = 0.0
	for line in lines:
		if line.find("PSNR Mean") > 0: 
			if line.find("kb/s") > 0: 
				x = line.find("Global:")
				psnr =  float(line[x+7:x+7+6])


	
	# find fps
	fps = 0.0
	for line in lines:
		if line.find("work: average") > 0:
			if line.find("fps") > 0:
				x = line.find("job is")
				y = line.find("fps")
				fps = float(line[x+6:y])
	
	
	print "fps = " + str(fps) + ", PSNR = " + str(psnr)
	
	logFILE = open(outdir + testname + '.log', "w")
	logFILE.write(serr)
	logFILE.close()
	
	#if process.returncode == None: process.wait()
	
	return [psnr, fps]
	
	

########## main entry point #######
def mytask():

	if os.path.exists(outdir) == False:
		print 'Creating output directory: ' + outdir
		os.makedirs(outdir)
	else:
		print 'Output directory exists'

	scriptFILE = open(inputscript, "r")

	script = scriptFILE.read()

	outFILE = open(outdir + outcsv, "w")

	script_dict = [s for s in script.splitlines() if s]

	# get the order of the tests
	testname = list()
	for line in script_dict:
		if line.startswith('# '):
			testname.append(line[1:])

	# test code
	#for item in testname:
	#	print item

	outFILE.write("input file, handbrake preset, fps, psnr\n")

	for inimg in inputimagefiles:
		i = iter(testname)
		[filename, part, fileext] = inimg.partition('.')
		for line in script_dict:
			if line.startswith('#') == 0:
				test = i.next()
				ex = line.replace("infile", inimg)
				ex = ex.replace("outfile", outdir+fileext + '_' + test.lstrip())
				ex = "handbrakecli.exe " + ex			
				print "-------------" + test + "----------------"
				print ex
				[psnr, fps] = run_handbrake(ex, fileext+ '_' + test.lstrip())
				outstr = inimg + "," + test + "," + str(fps) + "," + str(psnr) + "\n"			
				outFILE.write(outstr)
				outFILE.flush()	
				os.fsync(outFILE.fileno())
				time.sleep(30)
				print outstr				

	outFILE.close()
	print "done"
		
if __name__ == '__main__':
	p = Process(target = mytask)
	p.start()
	p.join()
	
