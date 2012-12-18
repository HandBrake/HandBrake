Handbrake automation scripts

this script is written in Python (2.7). It should work with any open source python distribution. 
I prefer to use enthought.com's free version of Python. 

To use this script:
1) copy handbrake_test.txt to the handbrakecli.exe directory
2) copy run_handbrake.py to the handbrakecli.exe directory
3) copy the input video files to the handbrakecli.exe directory
4) in a command window, go to the handbrakecli.exe directory
5) type:run_handbrake.py and hit return

you should see the command lines and % complete in the command window. Output files are located in the ./output directory. 
there will be a separate output video file, log file (the handbrake output), and a testresults.csv file

When the test is complete, import the testresults.csv into an Excel spreadsheet. 

Common Problems:

you will see zero fps and psnr if handbrakecli.exe exists with a failure. Common problems are missing video files from the current directory.



Notes:

The run_handbrake.py is a text file python script. The top of the file contains configuration parameters such as 
the input video files to use, etc. change these at will.

The handbrake_test.txt is a "script" file that contains the individual tests to run. The format is:

#- is a comment

# name of test (also used as output log file name, so don't use strange characters here

-i this is the handbrakecli.exe options line


blank lines are ignored.

To enable UVD decoding, add -P -U to each execution line in handbrake_test.txt.
Example:
# universal
-i "infile" -t 1  -o "outfile.m4v"  -P -U -f mp4 -w 720 --loose-anamorphic  -e x264 -q 20 --vfr  -a 1,1 -E faac,ffac3 -B 160,0 -6 dpl2,auto -R Auto,Auto -D 0,0 --gain=0,0 --audio-copy-mask none --audio-fallback ffac3 --markers="C:\Users\jgallmeier\AppData\Local\Temp\Blueangels-1-1-chapters.csv" -x cabac=0:ref=2:me=umh:bframes=0:weightp=0:8x8dct=0:trellis=0:subq=6 --verbose=1
