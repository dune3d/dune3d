
foreach pic : [true, false]
    dxflib_t = static_library('dxflib' + (pic?'-pic':''),
        files(
            'dl_dxf.cpp',
            'dl_writer_ascii.cpp'
        ), pic: pic
    )

    if pic
        dxflib_pic = dxflib_t
    else
        dxflib_nopic = dxflib_t
    endif
endforeach
