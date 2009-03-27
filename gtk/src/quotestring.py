#! /usr/bin/python

import re
import getopt
import sys

def usage():
	print >> sys.stderr, (
		"Usage: %s <input> [output]\n"
		"Summary:\n"
		"    Creates a quoted string suitable for inclusion in a C char*\n\n"
		"Options:\n"
		"    <input>   Input file to quote\n"
		"    <output>  Output quoted string [stdout]\n"
		% sys.argv[0]
	)

def main():
	global inc_list

	OPTS = ""
	try:
		opts, args = getopt.gnu_getopt(sys.argv[1:], OPTS)
	except getopt.GetoptError, err:
		print >> sys.stderr, str(err)
		usage()
		sys.exit(2)

	for o, a in opts:
		usage()
		assert False, "unhandled option"

	if len(args) > 2 or len(args) < 1:
		usage()
		sys.exit(2)

	try:
		infile = open(args[0])
	except Exception, err:
		print >> sys.stderr, ( "Error: %s"  % str(err) )
		sys.exit(1)

	if len(args) > 1:
		try:
			outfile = open(args[1], "w")
		except Exception, err:
			print >> sys.stderr, ( "Error: %s"  % str(err))
			sys.exit(1)
	else:
		outfile = sys.stdout

	ss = infile.read()
	ss = re.sub("\"", "\\\"", ss)
	pattern = re.compile("$", re.M)
	# the replacement string below seems a bit strange, but it seems to be
	# the only way to get the litteral chars '\' 'n' inserted into the string
	ss = re.sub(pattern, "\\\\n\"", ss)
	pattern = re.compile("^", re.M)
	ss = re.sub(pattern, "\"", ss)
	outfile.write(ss)

main()

