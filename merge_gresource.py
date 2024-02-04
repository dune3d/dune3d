#!/usr/bin/python
from __future__ import print_function
import sys
import os
import xml.etree.ElementTree as ET

base_file, *patch_files, platform, outfile = sys.argv[1:]
base_xml = ET.parse(base_file).getroot()

for file in patch_files:
  file_name = os.path.splitext(file)[0]
  if file_name.endswith(platform):
    for patched_file in ET.parse(file).getroot().find("gresource").iter("file"):
      target = patched_file.get("alias")
      source = base_xml.find(f'.//file[.="{target}"]')
      source.set("alias", target)
      source.text = patched_file.text

ET.ElementTree(base_xml).write(outfile)
