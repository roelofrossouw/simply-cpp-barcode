#ifndef SC_PDF417_H
#define SC_PDF417_H
// PDF417 barcodes (ISO/IEC 15438) found in a picture and decoded, also from poor scans: a barcode
// with too few pixels to a module for a decoder as it is (a 200 dpi scan of a card: about 1.4) is
// tried again, in its own band of the picture, enlarged and thresholded locally, with the settings
// that read such scans first, each in a thread of its own, until it decodes, its tries run out or
// the time is up. A barcode found but not decoded is still returned, with no bytes, so the caller
// knows one is there. zxing-cpp (Apache-2.0) does the decoding and OpenCV the image work; neither
// shows here.
//
//     const sc::pdf417::grey_view picture{pixels, {width, height}};
//     for (const auto &code: sc::pdf417::read(picture))
//         if (code.decoded()) use(code.bytes);  // the barcode's bytes, as they were encoded

#include <rect.h>
#include <size.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace sc::pdf417 {
    // A grey picture in memory: 8-bit, one channel, rows stride bytes apart (0: the width), not
    // owned (it must outlive read()).
    struct grey_view {
        const std::uint8_t *pixels = nullptr;
        sc::size_i size;
        std::ptrdiff_t stride = 0;
    };

    // A barcode found: where (in the picture's pixels), its bytes (empty when it couldn't be decoded),
    // the decoding attempts it took or used up (1: at once), and the enlargement it decoded at.
    struct barcode {
        sc::rect_i where;
        std::string bytes;
        int tries = 0;
        double scale = 1;
        [[nodiscard]] bool decoded() const { return !bytes.empty(); }
    };

    struct options {
        // Attempts for each barcode that doesn't decode at once (each tries one enlargement and
        // threshold: about 4 ms on a card's band).
        int max_tries = 1500;
        // When to stop trying; the default: 30 seconds after read() starts.
        std::chrono::steady_clock::duration time_limit = std::chrono::seconds{30};
        // A barcode whose long side is shorter than this (pixels) isn't returned: one of about 400
        // modules across (a card's) can't be decoded below it however hard it's tried, and is
        // likely not a barcode at all. 0: every one found.
        int smallest = 250;
    };

    // The PDF417 barcodes in picture, top to bottom. Throws std::invalid_argument for an empty
    // picture or a stride shorter than its width.
    [[nodiscard]] std::vector<barcode> read(const grey_view &picture, const options &how = {});
}

#endif
