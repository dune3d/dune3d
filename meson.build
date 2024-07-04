project('dune3d', ['cpp', 'c'],
  version: '1.1.0',
  meson_version: '>=0.53',
  default_options: ['cpp_std=c++20', 'warning_level=1'],
)

cxx = meson.get_compiler('cpp')
is_windows = target_machine.system() == 'windows'



gtk4 = dependency('gtk4', version: '>=4.10')
gtkmm = dependency('gtkmm-4.0', version: '>=4.10')
epoxy = dependency('epoxy')
eigen = dependency('eigen3')
glm = dependency('glm')


opencascade = dependency('OpenCASCADE', method : 'cmake')
if target_machine.system() == 'darwin'
  message('meson\'s framework handling on macos with the cmake method is broken, replacing')
  opencascade_link_args = []
  brew = find_program('brew', required : false)
  if brew.found()
    brew_prefix_cmd = run_command('brew', '--prefix', check: true)
    brew_prefix = brew_prefix_cmd.stdout().strip() + '/lib'
    opencascade_link_args += ['-Wl,-L' + brew_prefix]
  endif
  opencascade_link_args += ['-lTKSTEP', '-lTKernel', '-lTKXCAF', '-lTKXSBase', '-lTKBRep', '-lTKCDF', '-lTKXDESTEP', '-lTKLCAF', '-lTKMath', '-lTKMesh', '-lTKTopAlgo', '-lTKPrim', '-lTKBO', '-lTKShHealing', '-lTKG3d', '-lTKGeomBase', '-lTKHLR', '-lTKSTL', '-lTKFillet']
  opencascade = declare_dependency(dependencies: opencascade.partial_dependency(compile_args: true, includes:true), link_args: opencascade_link_args)
endif

spnav = dependency('spnav', required: false)
if not spnav.found()
    spnav = cxx.find_library('spnav', required: false)
endif

src = files(
  'src/main.cpp',
  'src/dune3d_application.cpp',
  'src/dune3d_appwindow.cpp',
  'src/editor/editor.cpp',
  'src/editor/editor_workspace_browser.cpp',
  'src/editor/editor_export.cpp',
  'src/editor/editor_workspace_view.cpp',
  'src/canvas/canvas.cpp',
  'src/canvas/gl_util.cpp',
  'src/canvas/base_renderer.cpp',
  'src/canvas/background_renderer.cpp',
  'src/canvas/face_renderer.cpp',
  'src/canvas/point_renderer.cpp',
  'src/canvas/line_renderer.cpp',
  'src/canvas/glyph_renderer.cpp',
  'src/canvas/glyph_3d_renderer.cpp',
  'src/canvas/box_selection.cpp',
  'src/canvas/bitmap_font_util.cpp',
  'src/canvas/bitmap_font/bitmap_font_desc.c',
  'src/canvas/bitmap_font/bitmap_font_img.c',
  'src/canvas/icon_renderer.cpp',
  'src/canvas/selection_texture_renderer.cpp',
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
  'src/document/entity/entity_document.cpp',
  'src/document/constraint/constraint.cpp',
  'src/document/constraint/constraint_points_coincident.cpp',
  'src/document/constraint/constraint_parallel.cpp',
  'src/document/constraint/constraint_equal_length.cpp',
  'src/document/constraint/constraint_equal_radius.cpp',
  'src/document/constraint/constraint_point_on_line.cpp',
  'src/document/constraint/constraint_point_on_line_base.cpp',
  'src/document/constraint/constraint_point_on_circle.cpp',
  'src/document/constraint/constraint_same_orientation.cpp',
  'src/document/constraint/constraint_hv.cpp',
  'src/document/constraint/constraint_point_distance.cpp',
  'src/document/constraint/constraint_point_distance_aligned.cpp',
  'src/document/constraint/constraint_point_distance_hv.cpp',
  'src/document/constraint/constraint_workplane_normal.cpp',
  'src/document/constraint/constraint_midpoint.cpp',
  'src/document/constraint/constraint_diameter_radius.cpp',
  'src/document/constraint/constraint_arc_line_tangent.cpp',
  'src/document/constraint/constraint_arc_arc_tangent.cpp',
  'src/document/constraint/constraint_line_points_perpendicular.cpp',
  'src/document/constraint/constraint_angle.cpp',
  'src/document/constraint/constraint_point_in_plane.cpp',
  'src/document/constraint/constraint_point_line_distance.cpp',
  'src/document/constraint/constraint_point_plane_distance.cpp',
  'src/document/constraint/constraint_lock_rotation.cpp',
  'src/document/constraint/constraint_point_in_workplane.cpp',
  'src/document/constraint/constraint_symmetric_hv.cpp',
  'src/document/constraint/constraint_symmetric_line.cpp',
  'src/document/solid_model.cpp',
  'src/document/solid_model_occ.cpp',
  'src/document/solid_model_array.cpp',
  'src/document/solid_model_local_operation.cpp',
  'src/document/solid_model_lathe.cpp',
  'src/document/solid_model_extrude.cpp',
  'src/document/solid_model_util.cpp',
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
  'src/document/group/group_polar_array.cpp',
  'src/document/group/group_array.cpp',
  'src/document/export_paths.cpp',
  'src/system/system.cpp',
  'src/logger/logger.cpp',
  'src/logger/log_dispatcher.cpp',
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
  'src/core/tools/tool_constrain_point_in_plane.cpp',
  'src/core/tools/tool_constrain_lock_rotation.cpp',
  'src/core/tools/tool_constrain_point_in_workplane.cpp',
  'src/core/tools/tool_constrain_symmetric_hv.cpp',
  'src/core/tools/tool_constrain_symmetric_line.cpp',
  'src/core/tools/tool_constrain_distance_aligned.cpp',
  'src/core/tools/tool_enter_datum.cpp',
  'src/core/tools/tool_draw_workplane.cpp',
  'src/core/tools/tool_add_anchor.cpp',
  'src/core/tools/tool_move_anchor.cpp',
  'src/core/tools/tool_toggle_construction.cpp',
  'src/core/tools/tool_import_step.cpp',
  'src/core/tools/tool_select_edges.cpp',
  'src/core/tools/tool_set_workplane.cpp',
  'src/core/tools/tool_helper_constrain.cpp',
  'src/core/tools/tool_rotate.cpp',
  'src/core/tools/tool_draw_regular_polygon.cpp',
  'src/core/tools/tool_draw_rectangle.cpp',
  'src/core/tools/tool_flip_arc.cpp',
  'src/core/tools/tool_link_document.cpp',
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
  'src/workspace/document_view.cpp',
  'src/editor/group_editor.cpp',
  'src/editor/selection_editor.cpp',
  'src/editor/selection_menu_creator.cpp',
  'src/dialogs/dialogs.cpp',
  'src/dialogs/enter_datum_window.cpp',
  'src/dialogs/rotate_window.cpp',
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
  'src/widgets/log_view.cpp',
  'src/widgets/log_window.cpp',
  'src/widgets/clipping_plane_window.cpp',
  'src/widgets/selection_filter_window.cpp',
  'src/util/version.cpp',
  'src/util/msd.cpp',
  'src/util/msd_animator.cpp',
  'src/util/exception_util.cpp',
  'src/util/key_util.cpp',
  'src/workspace/entity_view.cpp',
  'src/workspace/workspace_view.cpp',
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
version_deps = ['version.py']
if has_git_dir
    message('including git commit')
    version_deps += ['.git/HEAD', '.git/index']
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

cpp_args = ['-DLIBRARY', '-DUSINGZ', '-D_USE_MATH_DEFINES', '-DGLM_ENABLE_EXPERIMENTAL']

if spnav.found()
    cpp_args += '-DHAVE_SPNAV'
endif

stdlibs = []
is_libstdcpp = cxx.get_define('__GLIBCXX__', prefix: '#include <vector>') != ''
if is_libstdcpp
    message('using libstdc++')
else
    message('not using libstdc++')
endif
if is_libstdcpp
    stdlibs += cxx.find_library('stdc++')
    stdlibs += cxx.find_library('stdc++fs', required:false)
endif

build_dependencies = [gtk4, gtkmm, epoxy, opencascade, eigen, glm, stdlibs, spnav]
if not is_windows
	uuid = dependency('uuid')
	build_dependencies += uuid
else
	build_dependencies += [cxx.find_library('rpcrt4')]
	src += 'src/util/uuid_win32.cpp'
	cpp_args += '-DWIN32_UUID'
endif

# on darwin, the math lib is included in libSystem, which is linked by default
if target_machine.system() != 'darwin'
  libm = cxx.find_library('m')
  build_dependencies += libm
endif

include_directories = [
  include_directories('src'),
  include_directories('3rd_party'),
  include_directories('3rd_party/Clipper2Lib/include'),
]

subdir('3rd_party/solvespace')
subdir('3rd_party/Clipper2Lib')

if is_windows
	windows = import('windows')

	dune3d_rc = custom_target(
        'dune3d.rc',
        output : 'dune3d.rc',
        input : ['make_rc.py'],
        depend_files: files('version.py'),
        command : [prog_python,  '@INPUT@', '@OUTPUT@'],
    )

	rc_compiled =  windows.compile_resources(dune3d_rc, include_directories:include_directories('.'))
else
	rc_compiled = []
endif

datadir = get_option('datadir')
install_data('org.dune3d.dune3d.desktop', install_dir:datadir / 'applications')
install_data('org.dune3d.dune3d.metainfo.xml', install_dir:datadir / 'metainfo')
icondir = datadir / 'icons/hicolor'

install_data('src/icons/scalable/apps/dune3d.svg', install_dir: icondir /  'scalable/apps', rename: 'org.dune3d.dune3d.svg')


dune3d = executable('dune3d',
    [src,resources,icon_texture, color_presets, rc_compiled],
    dependencies: [build_dependencies],
    link_with: [solvespace, clipper],
    cpp_args: cpp_args,
    include_directories: include_directories,
    gui_app: true, 
    install: true
)

