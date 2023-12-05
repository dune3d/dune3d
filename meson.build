project('dune3d', ['cpp', 'c'],
  version: '0.1.0',
  meson_version: '>=0.53',
  default_options: ['cpp_std=c++20', 'warning_level=1'],
)

cxx = meson.get_compiler('cpp')
is_windows = target_machine.system() == 'windows'



libm = cxx.find_library('m')
gtk4 = dependency('gtk4', version: '>=4.0')
gtkmm = dependency('gtkmm-4.0', version: '>=4.0.0')
epoxy = dependency('epoxy')
eigen = dependency('eigen3')
glm = dependency('glm')


opencascade = dependency('OpenCASCADE', method : 'cmake')

rangev3 = dependency('range-v3', method : 'cmake')

src = files(
  'src/main.cpp',
  'src/dune3d_application.cpp',
  'src/dune3d_appwindow.cpp',
  'src/editor/editor.cpp',
  'src/editor/editor_workspace_browser.cpp',
  'src/canvas/canvas.cpp',
  'src/canvas/gl_util.cpp',
  'src/canvas/base_renderer.cpp',
  'src/canvas/background_renderer.cpp',
  'src/canvas/face_renderer.cpp',
  'src/canvas/point_renderer.cpp',
  'src/canvas/line_renderer.cpp',
  'src/canvas/glyph_renderer.cpp',
  'src/canvas/box_selection.cpp',
  'src/canvas/bitmap_font_util.cpp',
  'src/canvas/bitmap_font/bitmap_font_desc.c',
  'src/canvas/bitmap_font/bitmap_font_img.c',
  'src/canvas/icon_renderer.cpp',
  'src/canvas/appearance.cpp',
  'src/canvas/selectable_ref.cpp',
  'src/import_step/step_importer.cpp',
  'src/import_step/step_import_manager.cpp',
  'src/util/uuid.cpp',
  'src/document/document.cpp',
  'src/document/entity/entity.cpp',
  'src/document/entity/entity_and_point.cpp',
  'src/document/entity/entity_line3d.cpp',
  'src/document/entity/entity_line2d.cpp',
  'src/document/entity/entity_arc2d.cpp',
  'src/document/entity/entity_circle2d.cpp',
  'src/document/entity/entity_circle3d.cpp',
  'src/document/entity/entity_arc3d.cpp',
  'src/document/entity/entity_workplane.cpp',
  'src/document/entity/entity_step.cpp',
  'src/document/entity/entity_point2d.cpp',
  'src/document/constraint/constraint.cpp',
  'src/document/constraint/constraint_points_coincident.cpp',
  'src/document/constraint/constraint_parallel.cpp',
  'src/document/constraint/constraint_equal_length.cpp',
  'src/document/constraint/constraint_equal_radius.cpp',
  'src/document/constraint/constraint_point_on_line.cpp',
  'src/document/constraint/constraint_point_on_circle.cpp',
  'src/document/constraint/constraint_same_orientation.cpp',
  'src/document/constraint/constraint_hv.cpp',
  'src/document/constraint/constraint_point_distance.cpp',
  'src/document/constraint/constraint_point_distance_hv.cpp',
  'src/document/constraint/constraint_workplane_normal.cpp',
  'src/document/constraint/constraint_midpoint.cpp',
  'src/document/constraint/constraint_diameter_radius.cpp',
  'src/document/constraint/constraint_arc_line_tangent.cpp',
  'src/document/constraint/constraint_arc_arc_tangent.cpp',
  'src/document/constraint/constraint_line_points_perpendicular.cpp',
  'src/document/constraint/constraint_angle.cpp',
  'src/document/solid_model.cpp',
  'src/document/solid_model_occ.cpp',
  'src/document/group/group.cpp',
  'src/document/group/group_sketch.cpp',
  'src/document/group/group_reference.cpp',
  'src/document/group/group_extrude.cpp',
  'src/document/group/group_local_operation.cpp',
  'src/document/group/group_fillet.cpp',
  'src/document/group/group_chamfer.cpp',
  'src/document/group/group_lathe.cpp',
  'src/document/group/group_sweep.cpp',
  'src/document/group/group_linear_array.cpp',
  'src/document/group/group_array.cpp',
  'src/system/system.cpp',
  'src/logger/logger.cpp',
  'src/render/renderer.cpp',
  'src/util/util.cpp',
  'src/util/fs_util.cpp',
  'src/util/json_util.cpp',
  'src/util/glm_util.cpp',
  'src/util/selection_util.cpp',
  'src/util/file_version.cpp',
  'src/core/core.cpp',
  'src/core/tool.cpp',
  'src/core/create_tool.cpp',
  'src/core/tools/tool_common.cpp',
  'src/core/tools/tool_draw_line_3d.cpp',
  'src/core/tools/tool_draw_point_2d.cpp',
  'src/core/tools/tool_draw_arc_2d.cpp',
  'src/core/tools/tool_draw_circle_2d.cpp',
  'src/core/tools/tool_draw_contour.cpp',
  'src/core/tools/tool_delete.cpp',
  'src/core/tools/tool_move.cpp',
  'src/core/tools/tool_move.cpp',
  'src/core/tools/tool_constrain_coincident.cpp',
  'src/core/tools/tool_constrain_hv.cpp',
  'src/core/tools/tool_constrain_distance.cpp',
  'src/core/tools/tool_constrain_same_orientation.cpp',
  'src/core/tools/tool_constrain_parallel.cpp',
  'src/core/tools/tool_constrain_workplane_normal.cpp',
  'src/core/tools/tool_constrain_midpoint.cpp',
  'src/core/tools/tool_constrain_equal_length.cpp',
  'src/core/tools/tool_constrain_equal_radius.cpp',
  'src/core/tools/tool_constrain_diameter_radius.cpp',
  'src/core/tools/tool_constrain_perpendicular.cpp',
  'src/core/tools/tool_enter_datum.cpp',
  'src/core/tools/tool_draw_workplane.cpp',
  'src/core/tools/tool_add_anchor.cpp',
  'src/core/tools/tool_toggle_construction.cpp',
  'src/core/tools/tool_import_step.cpp',
  'src/core/tools/tool_select_edges.cpp',
  'src/core/tools/tool_set_workplane.cpp',
  'src/core/tools/tool_helper_constrain.cpp',
  'src/util/history_manager.cpp',
  'src/action/action.cpp',
  'src/action/action_catalog.cpp',
  'src/in_tool_action/in_tool_action_catalog.cpp',
  'src/util/str_util.cpp',
  'src/util/lut.cpp',
  'src/preferences/preferences.cpp',
  'src/util/action_label.cpp',
  'src/editor/tool_popover.cpp',
  'src/editor/workspace_browser.cpp',
  'src/widgets/constraints_box.cpp',
  'src/editor/document_view.cpp',
  'src/editor/group_editor.cpp',
  'src/editor/selection_editor.cpp',
  'src/dialogs/dialogs.cpp',
  'src/dialogs/enter_datum_window.cpp',
  'src/dialogs/tool_window.cpp',
  'src/widgets/spin_button_dim.cpp',
  'src/widgets/spin_button_angle.cpp',
  'src/util/gtk_util.cpp',
  'src/preferences/preferences_window.cpp',
  'src/preferences/action_editor.cpp',
  'src/preferences/preferences_window_keys.cpp',
  'src/preferences/preferences_window_in_tool_keys.cpp',
  'src/preferences/preferences_window_keys_base.cpp',
  'src/preferences/preferences_window_canvas.cpp',
  'src/preferences/preferences_row.cpp',
  'src/preferences/preferences_window_misc.cpp',
  'src/widgets/capture_dialog.cpp',
  'src/widgets/axes_lollipop.cpp',
  'src/widgets/about_dialog.cpp',
  'src/widgets/recent_item_box.cpp',
  'src/util/version.cpp',
  'src/util/msd.cpp',
  'src/util/msd_animator.cpp',
)

prog_python = find_program('python3')

icon_texture = custom_target(
    'icon_texture',
    output : ['icon_texture_id.hpp', 'icon_texture_map.cpp', 'icon_texture_atlas.png'],
    input : ['make_icon_texture_atlas.py', 'src/texture-icons.svg'],
    command : [prog_python, '@INPUT@', '@OUTPUT@'],
)

color_presets = custom_target(
    'color_presets',
    output : ['color_presets.cpp'],
    input : [
      'make_color_presets.py',
      'src/preferences/color_presets/Default.dark.json',
      'src/preferences/color_presets/Default.light.json'
    ],
    command : [prog_python, '@INPUT@', '@OUTPUT@'],
)

fs = import('fs')
has_git_dir = fs.is_dir('.git')
if has_git_dir
    message('including git commit')
    version_deps = ['.git/HEAD', '.git/index']
else
    version_deps = []
endif

version_gen_cpp = custom_target(
    'version_gen.cpp',
    output : 'version_gen.cpp',
    input : ['make_version.py'],
    depend_files: version_deps,
    command : [prog_python, '@INPUT@', '@OUTPUT@', '@SOURCE_ROOT@', meson.project_version()],
)
src += version_gen_cpp

gnome = import('gnome')
resources = gnome.compile_resources(
  'resources',
  'src/dune3d.gresource.xml',
  dependencies: [icon_texture],
  c_name: 'dune3d_resources',
  source_dir: 'src'
)

cpp_args = ['-DLIBRARY', '-DUSINGZ', '-D_USE_MATH_DEFINES']

stdlibs = []
is_libstdcpp = cxx.get_define('__GLIBCXX__', prefix: '#include <vector>') != ''
if is_libstdcpp
    message('using libstdc++')
else
    message('not using libstdc++')
endif
if is_libstdcpp
    stdlibs += cxx.find_library('stdc++')
    stdlibs += cxx.find_library('stdc++fs')
endif

build_dependencies = [libm, gtk4, gtkmm, epoxy, opencascade, eigen, glm, rangev3, stdlibs]
if not is_windows
	uuid = dependency('uuid')
	build_dependencies += uuid
else
	build_dependencies += [cxx.find_library('rpcrt4')]
	src += 'src/util/uuid_win32.cpp'
	cpp_args += '-DWIN32_UUID'
endif
include_directories = [
  include_directories('src'),
  include_directories('3rd_party'),
  include_directories('3rd_party/Clipper2Lib/include'),
]

subdir('3rd_party/solvespace')
subdir('3rd_party/Clipper2Lib')



dune3d = executable('dune3d',
    [src,resources,icon_texture, color_presets],
    dependencies: [build_dependencies],
    link_with: [solvespace, clipper],
    cpp_args: cpp_args,
    include_directories: include_directories,
    gui_app: true, 
    install: true
)

