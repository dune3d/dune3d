#include "picture_load.hpp"
#include "picture_data.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <giomm/file.h>
#include <set>
#include "fs_util.hpp"
#include "uuid.hpp"
#include <algorithm>
#include <png.h>
#include "logger/logger.hpp"
#include "document/document.hpp"
#include "document/entity/entity_picture.hpp"

namespace dune3d {

static std::filesystem::path build_pic_filename(const std::filesystem::path &dir, const UUID &uu)
{
    return dir / path_from_string((std::string)uu + ".png");
}

// adapted from https://gitlab.freedesktop.org/cairo/cairo/-/blob/master/src/cairo-png.c

static void png_simple_error_callback(png_structp png, png_const_charp error_msg)
{
    throw std::runtime_error(std::string("png error: ") + error_msg);
}

static void png_simple_warning_callback(png_structp png, png_const_charp error_msg)
{
}

struct Png {
    png_struct *png = nullptr;
    png_info *info = nullptr;
};

struct PngWrite : Png {
    ~PngWrite()
    {
        if (png)
            png_destroy_write_struct(&png, &info);
    }
};


struct PngRead : Png {
    ~PngRead()
    {
        if (png)
            png_destroy_read_struct(&png, &info, NULL);
    }
};

struct FileWrapper {
    FileWrapper(const char *filename, const char *modes)
    {
#ifdef G_OS_WIN32
        auto wfilename = reinterpret_cast<wchar_t *>(g_utf8_to_utf16(filename, -1, NULL, NULL, NULL));
        auto wmodes = reinterpret_cast<wchar_t *>(g_utf8_to_utf16(modes, -1, NULL, NULL, NULL));
        fp = _wfopen(wfilename, wmodes);
        g_free(wfilename);
        g_free(wmodes);
#else
        fp = fopen(filename, modes);
#endif
        if (!fp) {
            throw std::runtime_error("fopen error");
        }
    }
    FILE *fp = NULL;
    ~FileWrapper()
    {
        if (fp)
            fclose(fp);
    }
};

static void convert_data_to_bytes(png_structp png, png_row_infop row_info, png_bytep data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
        uint8_t *b = &data[i];
        uint32_t pixel;

        memcpy(&pixel, b, sizeof(uint32_t));

        b[2] = (pixel & 0x00ff'0000) >> 16;
        b[1] = (pixel & 0x0000'ff00) >> 8;
        b[0] = (pixel & 0x0000'00ff) >> 0;
        b[3] = (pixel & 0xff00'0000) >> 24;
    }
}

template <bool use_alpha> static void convert_bytes_to_data(png_structp png, png_row_infop row_info, png_bytep data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
        uint8_t *base = &data[i];
        uint8_t red = base[2];
        uint8_t green = base[1];
        uint8_t blue = base[0];
        uint8_t alpha = base[3];
        uint32_t pixel;

        pixel = (red << 16) | (green << 8) | (blue << 0);
        if constexpr (use_alpha) {
            pixel |= alpha << 24;
        }
        else {
            pixel |= (0xff << 24);
        }
        memcpy(base, &pixel, sizeof(uint32_t));
    }
}

static std::shared_ptr<PictureData> read_png(const std::string &filename, const UUID &uu)
{
    FileWrapper fp{filename.c_str(), "rb"};

    int status = 0;
    PngRead png;
    png.png = png_create_read_struct(PNG_LIBPNG_VER_STRING, &status, png_simple_error_callback,
                                     png_simple_warning_callback);
    if (png.png == NULL) {
        throw std::runtime_error("png error png");
    }

    png.info = png_create_info_struct(png.png);
    if (png.info == NULL) {
        throw std::runtime_error("png error info");
    }

    png_init_io(png.png, fp.fp);

    png_read_info(png.png, png.info);
    int depth, color_type, interlace;
    png_uint_32 png_width, png_height;

    png_get_IHDR(png.png, png.info, &png_width, &png_height, &depth, &color_type, &interlace, NULL, NULL);
    if (status) { /* catch any early warnings */
        throw std::runtime_error("png error header");
    }

    switch (color_type) {
    case PNG_COLOR_TYPE_RGB_ALPHA:
        png_set_read_user_transform_fn(png.png, convert_bytes_to_data<true>);
        break;
    case PNG_COLOR_TYPE_RGB:
        png_set_read_user_transform_fn(png.png, convert_bytes_to_data<false>);
        break;
    default:
        throw std::runtime_error("unsupported color type " + std::to_string(color_type));
    }

    if (depth != 8) {
        throw std::runtime_error("unsupported color depth");
    }

    if (interlace != PNG_INTERLACE_NONE)
        throw std::runtime_error("unsupported interlacing");

    png_set_filler(png.png, 0xff, PNG_FILLER_AFTER);

    std::vector<uint32_t> data;
    data.resize(png_width * png_height);

    std::vector<png_byte *> row_pointers;

    for (size_t i = 0; i < png_height; i++) {
        row_pointers.push_back(reinterpret_cast<png_byte *>(data.data() + (i * png_width)));
    }

    png_read_image(png.png, row_pointers.data());
    png_read_end(png.png, png.info);

    return std::make_shared<PictureData>(uu, png_width, png_height, std::move(data));
}

void pictures_load(Document &doc, const std::filesystem::path &dir)
{
    std::map<UUID, std::shared_ptr<const PictureData>> pictures_loaded;
    for (auto &[uu, en] : doc.m_entities) {
        if (auto pic = dynamic_cast<EntityPicture *>(en.get())) {
            if (pictures_loaded.count(pic->m_data_uuid)) {
                pic->m_data = pictures_loaded.at(pic->m_data_uuid);
            }
            else {
                const auto pic_filename = build_pic_filename(dir, pic->m_data_uuid);
                try {
                    if (std::filesystem::is_regular_file(pic_filename)) {
                        auto data = read_png(pic_filename, pic->m_data_uuid);
                        pic->m_data = data;
                        pictures_loaded.emplace(pic->m_data_uuid, data);
                    }
                }
                catch (const std::exception &e) {
                    Logger::log_warning("error loading picture " + (std::string)pic->m_data_uuid,
                                        Logger::Domain::PICTURE, e.what());
                }
                catch (...) {
                    Logger::log_warning("error loading picture " + (std::string)pic->m_data_uuid,
                                        Logger::Domain::PICTURE);
                }
            }
        }
    }
}

static void write_png_to_file(const std::string &filename, const PictureData &data)
{
    /* PNG complains about "Image width or height is zero in IHDR" */
    if (data.m_width == 0 || data.m_height == 0) {
        throw std::runtime_error("image must not have zero width or height");
    }

    FileWrapper fp{filename.c_str(), "wb"};
    PngWrite png;

    int status = 0;
    png.png = png_create_write_struct(PNG_LIBPNG_VER_STRING, &status, png_simple_error_callback,
                                      png_simple_warning_callback);
    if (png.png == NULL) {
        throw std::runtime_error("png error");
    }

    png.info = png_create_info_struct(png.png);
    if (png.info == NULL) {
        throw std::runtime_error("png error");
    }

    png_init_io(png.png, fp.fp);

    const int bpc = 8;
    const int png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;

    png_set_IHDR(png.png, png.info, data.m_width, data.m_height, bpc, png_color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_color_16 white;
    white.gray = (1 << bpc) - 1;
    white.red = white.blue = white.green = white.gray;
    png_set_bKGD(png.png, png.info, &white);

    std::vector<const png_byte *> row_pointers;

    for (size_t i = 0; i < data.m_height; i++) {
        row_pointers.push_back(reinterpret_cast<const png_byte *>(data.m_data.data() + (i * data.m_width)));
    }

    png_set_rows(png.png, png.info, const_cast<png_byte **>(row_pointers.data()));
    png_set_write_user_transform_fn(png.png, convert_data_to_bytes);
    png_write_png(png.png, png.info, PNG_TRANSFORM_IDENTITY, NULL);
}

void pictures_save(const Document &doc, const std::filesystem::path &dir)
{
    std::set<UUID> pictures_to_delete;
    if (std::filesystem::is_directory(dir)) {
        for (const auto &dir_entry : std::filesystem::directory_iterator(dir)) {
            const auto name = path_to_string(dir_entry.path().filename());
            if (dir_entry.is_regular_file() && name.ends_with(".png")) {
                pictures_to_delete.emplace(name.substr(0, 36));
            }
        }
    }


    for (auto &[uu, en] : doc.m_entities) {
        if (auto pic = dynamic_cast<EntityPicture *>(en.get())) {
            if (pic->m_data) {
                try {
                    auto data = pic->m_data;
                    pictures_to_delete.erase(pic->m_data->m_uuid);
                    auto pic_filename = build_pic_filename(dir, pic->m_data->m_uuid);
                    if (!std::filesystem::exists(pic_filename)) {
                        if (!std::filesystem::is_directory(dir))
                            std::filesystem::create_directory(dir);
                        write_png_to_file(pic_filename, *data);
                    }
                }
                catch (const std::exception &e) {
                    Logger::log_warning("error saving picture " + (std::string)pic->m_data->m_uuid,
                                        Logger::Domain::PICTURE, e.what());
                }
                catch (...) {
                    Logger::log_warning("error saving picture " + (std::string)pic->m_data->m_uuid,
                                        Logger::Domain::PICTURE);
                }
            }
        }
    }
    for (const auto &it : pictures_to_delete) {
        const auto filename = build_pic_filename(dir, it);
        try {
            std::filesystem::remove(filename);
        }
        catch (const std::exception &e) {
            Logger::log_warning("error deleting picture " + path_to_string(filename), Logger::Domain::PICTURE,
                                e.what());
        }
        catch (...) {
            Logger::log_warning("error deleting picture " + path_to_string(filename), Logger::Domain::PICTURE);
        }
    }
}


} // namespace dune3d
