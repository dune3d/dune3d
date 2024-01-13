import version as v
import os
import sys
import subprocess
from string import Template

rc_template = """2 ICON "src/icons/dune3d.ico"

1 VERSIONINFO
FILEVERSION     $major,$minor,$micro,0
PRODUCTVERSION  $major,$minor,$micro,0
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904E4"
    BEGIN
      VALUE "FileDescription", "Dune 3D"
      VALUE "InternalName", "dune3d"
      VALUE "OriginalFilename", "dune3d.exe"
      VALUE "ProductName", "Dune 3D"
      VALUE "ProductVersion", "$major.$minor.$micro"
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x809, 1252
  END
END
"""

outfile = sys.argv[1]
with open(outfile, "w", encoding="utf-8") as fi:
	tmpl = Template(rc_template)
	fi.write(tmpl.substitute(major=v.major, minor=v.minor, micro=v.micro))
