import gi
gi.require_version('Rsvg', '2.0')
from gi.repository import Rsvg

import cairo

import sys
import math
import xml.etree.ElementTree as ET


filename = sys.argv[1]
#subs = sys.argv[2:]
h = Rsvg.Handle.new_from_file(filename)


subs = [
]


tree = ET.parse(filename)
root = tree.getroot()
for elem in root.findall('{http://www.w3.org/2000/svg}g/') :
    if 'id' in elem.attrib :
        subs.append(elem.attrib['id'])

sub_pos = {}

texture_n = int(math.ceil(math.sqrt(len(subs))))
icon_size = 16
icon_border = 2

surf = cairo.ImageSurface(cairo.FORMAT_RGB24, (icon_size+2*icon_border)*texture_n, (icon_size+2*icon_border)*texture_n)


ctx = cairo.Context(surf)
ctx.set_source_rgb(1,1,1)
ctx.paint()

for i,sub in enumerate(subs) :
    #ctx.save()
    x = i%texture_n
    y = i//texture_n
    sub_pos[sub] = (x,y)
    #ctx.translate(x*icon_size, y*icon_size)
    rect=Rsvg.Rectangle()
    rect.width = icon_size
    rect.height = icon_size
    rect.x = (x*(icon_size+2*icon_border)) + icon_border
    rect.y = (y*(icon_size+2*icon_border)) + icon_border
    h.render_element(ctx, "#"+sub, rect)
    
    #ctx.restore()
with open(sys.argv[2], "w") as fi :
    fi.write('#pragma once\n') 
    fi.write('namespace dune3d::IconTexture {\n') 
    fi.write('enum class IconTextureID{\n')
    for sub in subs :
        fi.write(f'  {sub},\n')
    fi.write('};\n')
    fi.write('}\n')

with open(sys.argv[3], "w") as fi :
    fi.write('#include "canvas/icon_texture_map.hpp"\n')
    fi.write('#include "icon_texture_id.hpp"\n')
    fi.write('namespace dune3d::IconTexture {\n') 
    fi.write('const std::map<IconTextureID, Position> icon_texture_map = {\n')
    for sub in subs :
        x,y = sub_pos[sub]
        fi.write(f'{{ IconTextureID::{sub}, {{ {x}, {y} }} }},\n')
    fi.write('};\n')
    fi.write(f'const unsigned int icon_size = {icon_size};\n')
    fi.write(f'const unsigned int icon_border = {icon_border};\n')
    fi.write('}\n')
surf.write_to_png(sys.argv[4])
