foreach pic : [true, false]
    clipper_t = static_library('clipper' + (pic?'-pic':''),
        files(
            'src/clipper.engine.cpp',
            'src/clipper.offset.cpp',
            'src/clipper.rectclip.cpp',
        ),
        cpp_args: ['-DUSINGZ'],
        include_directories: [include_directories('include')],
        pic: pic
    )
    if pic
        clipper_pic = clipper_t
    else
        clipper_nopic = clipper_t
    endif
endforeach
