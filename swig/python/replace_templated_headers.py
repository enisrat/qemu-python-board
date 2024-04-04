import re
import sys
import os

header_directory = sys.argv[1]

os.chdir(header_directory)

for line in sys.stdin:
	m = re.match(r".*#include.*\"(.*\.h\.inc)\"", line)
	if m:
		with open(m.group(1), "r") as f:
			sys.stdout.write(f.read())
	else:
		sys.stdout.write(line)