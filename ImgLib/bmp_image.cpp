#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {
    PACKED_STRUCT_BEGIN BitmapFileHeader {
        uint16_t signature;
        uint32_t file_size;
        uint32_t reserved;
        uint32_t data_offset;
    }
    PACKED_STRUCT_END

    PACKED_STRUCT_BEGIN BitmapInfoHeader {
        uint32_t header_size;
        int32_t width;
        int32_t height;
        uint16_t planes;
        uint16_t bit_count;
        uint32_t compression;
        uint32_t image_size;
        int32_t x_pixels_per_m;
        int32_t y_pixels_per_m;
        int32_t colors_used;
        int32_t colors_important;
    }
    PACKED_STRUCT_END

    // функция вычисления отступа по ширине
    static int GetBMPStride(int w) {
        return 4 * ((w * 3 + 3) / 4);
    }

    bool SaveBMP(const Path &file, const Image &image) {
        ofstream ofs(file, ios::binary);
        if (!ofs) {
            return false;
        }

        const int w = image.GetWidth();
        const int h = image.GetHeight();
        const int stride = GetBMPStride(w);

        BitmapInfoHeader info_header;
        info_header.header_size = sizeof(BitmapInfoHeader);
        info_header.width = w;
        info_header.height = h;
        info_header.planes = 1;
        info_header.bit_count = 24;
        info_header.compression = 0;
        info_header.image_size = stride * h;
        info_header.x_pixels_per_m = 11811;
        info_header.y_pixels_per_m = 11811;
        info_header.colors_used = 0;
        info_header.colors_important = 0x1000000;

        BitmapFileHeader file_header;
        file_header.signature = ('M' << 8) | 'B';
        file_header.file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + info_header.image_size;
        file_header.reserved = 0;
        file_header.data_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

        ofs.write(reinterpret_cast<const char *>(&file_header), sizeof(file_header));
        ofs.write(reinterpret_cast<const char *>(&info_header), sizeof(info_header));

        std::vector<char> row(stride, 0);
        for (int y = h - 1; y >= 0; --y) {
            for (int x = 0; x < w; ++x) {
                const Color &color = image.GetPixel(x, y);
                row[x * 3 + 0] = static_cast<char>(color.b);
                row[x * 3 + 1] = static_cast<char>(color.g);
                row[x * 3 + 2] = static_cast<char>(color.r);
            }
            ofs.write(row.data(), stride);
        }

        return ofs.good();
    }

    Image LoadBMP(const Path &file) {
        ifstream ifs(file, ios::binary);
        if (!ifs) {
            return {};
        }

        BitmapFileHeader file_header;
        BitmapInfoHeader info_header;

        ifs.read(reinterpret_cast<char *>(&file_header), sizeof(file_header));
        ifs.read(reinterpret_cast<char *>(&info_header), sizeof(info_header));

        if (!ifs) {
            return {};
        }

        if (file_header.reserved != 0) {
            return {};
        }

        if (info_header.header_size != sizeof(BitmapInfoHeader)) {
            return {};
        }
        if (info_header.planes != 1) {
            return {};
        }
        if (info_header.bit_count != 24) {
            return {};
        }
        if (info_header.compression != 0) {
            return {};
        }

        const int w = info_header.width;
        const int h = info_header.height;
        const int stride = GetBMPStride(w);

        Image image(w, h, Color::Black());

        std::vector<char> row(stride);
        for (int y = h - 1; y >= 0; --y) {
            ifs.read(row.data(), stride);

            for (int x = 0; x < w; ++x) {
                auto &color = image.GetPixel(x, y);
                color.b = std::byte{row[x * 3 + 0]};
                color.g = std::byte{row[x * 3 + 1]};
                color.r = std::byte{row[x * 3 + 2]};
            }
        }

        return image;
    }
} // namespace img_lib
