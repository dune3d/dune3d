#!/usr/bin/python3
import re
from string import Template
import argparse
parser = argparse.ArgumentParser(
                    prog='Tool adder',
                    description='Adds a tool')

parser.add_argument('-c', '--class-name', required=True)
#parser.add_argument('-t', '--tool-id', required=True)
#parser.add_argument('-f', '--file-name', required=True)
parser.add_argument('-n', '--name', required=True)

def camel_to_snake(camel_string):
    return re.sub(r'([A-Z])', r'_\1', camel_string).lower().lstrip("_")

args = parser.parse_args()
class_name = f"Tool{args.class_name}"
filename = f"tool_{camel_to_snake(args.class_name)}"
tool_id = camel_to_snake(args.class_name).upper()

def add_tool_id():
    src = "src/core/tool_id.hpp"
    lines = [l.rstrip() for l in open(src, "r").readlines()]
    idx = lines.index("};")
    new_lines = lines[:idx]
    new_lines.append(f"    {tool_id},")
    new_lines += lines[idx:]
    new_lines.append("")
    
    with open(src, "w") as fi:
        fi.write("\n".join(new_lines))
    
def find_index(lines, first, before) :
    have_first = False
    for i,l in enumerate(lines) :
        if not have_first :
            if l == first :
                have_first = True
        else :
            if l == before :
                return i
        

def add_action():
    src = "src/action/action_catalog.cpp"
    lines = [l.rstrip() for l in open(src, "r").readlines()]
    idx1 = find_index(lines, "const std::map<ActionToolID, ActionCatalogItem> action_catalog = {", "};")
    idx2 = find_index(lines, "const LutEnumStr<ToolID> tool_lut = {", "};")
    
    new_lines = lines[:idx1]
    new_lines.append(f"        {{ToolID::{tool_id}, {{\"{args.name}\", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}}}},")
    new_lines += lines[idx1:idx2]
    new_lines.append(f"        TOOL_LUT_ITEM({tool_id}),")
    new_lines += lines[idx2:]
    
    new_lines.append("")
    
    with open(src, "w") as fi:
        fi.write("\n".join(new_lines))



def create_files():
    cpp = f"src/core/tools/{filename}.cpp"
    hpp = f"src/core/tools/{filename}.hpp"
    hpp_template = """#include "tool_common.hpp"

namespace dune3d {

class Tool$class_name : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }

    CanBegin can_begin() override;

private:
};

} // namespace dune3d
"""

    cpp_template = """#include "$filename.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin Tool$class_name::can_begin()
{
    return true;
}

ToolResponse Tool$class_name::begin(const ToolArgs &args)
{

    return ToolResponse();
}

ToolResponse Tool$class_name::update(const ToolArgs &args)
{
  
    return ToolResponse();
}

} // namespace dune3d
"""
    with open(hpp, "w") as fi :
        fi.write(Template(hpp_template).substitute(class_name = class_name))
        
    with open(cpp, "w") as fi :
        fi.write(Template(cpp_template).substitute(class_name = class_name, filename=filename))
        

def add_to_meson():
    src = "meson.build"
    lines = [l.rstrip() for l in open(src, "r").readlines()]
    idx = max(i for i,l in enumerate(lines) if l.strip().startswith("'src/core/tools/tool_"))
    
    new_lines = lines[:idx]
    new_lines.append(f"  'src/core/tools/{filename}.cpp',")
    new_lines += lines[idx:]
    
    new_lines.append("")
    
    with open(src, "w") as fi:
        fi.write("\n".join(new_lines))

def add_to_create_tool():
    src = "src/core/create_tool.cpp"
    lines = [l.rstrip() for l in open(src, "r").readlines()]
    idx1 = lines.index('#include "tool_id.hpp"')
    idx2 = lines.index("    }")
    
    new_lines = lines[:idx1]
    new_lines.append(f'#include "tools/{filename}.hpp"')
    new_lines += lines[idx1:idx2]
    new_lines.append("")
    new_lines.append(f"    case ToolID::{tool_id}:")
    new_lines.append(f"        return std::make_unique<Tool{class_name}>(tool_id, *this, m_intf, flags);")
    new_lines += lines[idx2:]
    
    new_lines.append("")
    
    with open(src, "w") as fi:
        fi.write("\n".join(new_lines))

add_tool_id()
add_action()
create_files()
add_to_meson()
add_to_create_tool()
