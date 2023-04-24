#include <pu/ui/elm/elm_Image.hpp>
#include <qrcodegen/cpp/qrcodegen.cpp>
#include <png.h>

using qrcodegen::QrCode;
using pu::ui::elm::Image;

namespace inst::drive {

static void png_output_flush(png_structp png_ptr, png_bytep data, png_size_t length) {
    std::vector<uint8_t> *p = (std::vector<uint8_t>*)png_get_io_ptr(png_ptr);
    p->insert(p->end(), data, data + length);
}

void renderQr(const std::string& text, Image::Ref& image) {
    const QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::LOW);

    const int margin = 2;
    const int size   = 10;
    int width        = qr.getSize();

    int realwidth = (width + margin * 2) * size;
    std::vector<uint8_t> row((realwidth + 7) / 8);
    std::vector<uint8_t> out;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr  = png_create_info_struct(png_ptr);
    setjmp(png_jmpbuf(png_ptr));
    png_set_IHDR(png_ptr, info_ptr, realwidth, realwidth, 1,
                 PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_write_fn(png_ptr, &out, png_output_flush, NULL);
    png_write_info(png_ptr, info_ptr);

    /* top margin */
    std::fill(row.begin(), row.end(), 0xff);
    for (int y = 0; y < margin * size; y++) {
        png_write_row(png_ptr, row.data());
    }

    unsigned char* q;
    int pixel, bit, x, y, i;
    for (y = 0; y < width; y++) {
        std::fill(row.begin(), row.end(), 0xff);
        q   = row.data() + (margin * size / 8);
        bit = 7 - (margin * size % 8);
        for (x = 0; x < width; x++) {
            pixel = qr.getModule(x, y) ? 1 : 0;
            for (i = 0; i < size; i++) {
                *q ^= pixel << bit;
                bit--;
                if (bit < 0) {
                    q++;
                    bit = 7;
                }
            }
        }
        for (i = 0; i < size; i++) {
            png_write_row(png_ptr, row.data());
        }
    }

    /* bottom margin */
    std::fill(row.begin(), row.end(), 0xff);
    for (int y = 0; y < margin * size; y++) {
        png_write_row(png_ptr, row.data());
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    image->SetJpegImage(out.data(), out.size());
}

}