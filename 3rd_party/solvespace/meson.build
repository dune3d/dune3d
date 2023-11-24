
eigen = dependency('eigen3')

is_windows = target_machine.system() == 'windows'
slvs_cpp_args = ['-DLIBRARY', '-D_USE_MATH_DEFINES']
if is_windows
    slvs_cpp_args += '-DWIN32'
endif

solvespace = static_library('solvespace',
    files(
        'src/util.cpp',
        'src/entity.cpp',
        'src/expr.cpp',
        'src/constraint.cpp',
        'src/constrainteq.cpp',
        'src/system.cpp',
        'src/platform/platform.cpp',
    ),
    cpp_args: slvs_cpp_args,
    include_directories: [include_directories('src')],
    dependencies: [eigen],
    pic: false
)
