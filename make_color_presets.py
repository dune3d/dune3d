#!/usr/bin/python
from __future__ import print_function

import json
import sys
import os

out = {}
for filename in sys.argv[1:-1] :
	with open(filename, "r") as fi :
		j = json.load(fi)
	bn = os.path.basename(filename).split(".")
	name = bn[0].replace("_", " ")
	dark = bn[1] == "dark"
	if name not in out :
		out[name] = {}
	out[name][dark] = j["colors"]

def format_color(c) :
	return f"{{ {c['r']}, {c['g']}, {c['b']} }}"

def format_colors(c) :
	return "{" + ",\n".join(f"{{ColorP::{color_name}, {format_color(color)} }}" for color_name, color in c.items()) + "}"


with open(sys.argv[-1], "w") as ofi:
	ofi.write('#include "preferences/color_presets.hpp"\n')
	ofi.write("namespace dune3d{\n")
	ofi.write("const std::map<std::string, ColorTheme> color_themes = {\n")
	for name, themes in out.items() :
		ofi.write(f"{{ \"{name}\", {{ \n")
		ofi.write(f".dark = {format_colors(themes[True])},")
		ofi.write(f".light = {format_colors(themes[False])},")
		ofi.write("}},\n")
	ofi.write("};\n")
	ofi.write("}\n")
