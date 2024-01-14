#!/usr/bin/python3

import version as v
import xml.etree.ElementTree as ET
tree = ET.parse('org.dune3d.dune3d.metainfo.xml')
root = tree.getroot()
version_from_xml = root.find("releases").find("release").attrib["version"]

rc = 0
if version_from_xml == v.string :
	print("Version okay")
else:
	print("Version mismatch %s != %s"%(v.string, version_from_xml))
	rc = 1

#Check changelog versions

for filename in ("CHANGELOG.md", "scripts/CHANGELOG.md.in") :
	first_line = next(open(filename, "r")).strip()
	if first_line != f"# Version {v.string}" :
		print(f"{filename} version mismatch")
		rc = 1
	else :
		print(f"{filename} version okay")

exit(rc)
