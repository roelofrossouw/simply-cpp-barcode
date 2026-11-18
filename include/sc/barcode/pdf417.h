#ifndef SC_BARCODE_PDF417_H
#define SC_BARCODE_PDF417_H
// sc::barcode::pdf417, PDF417 barcodes (ISO/IEC 15438) read from a picture; part of barcode.h.

#include <sc/barcode/barcode.h>

namespace sc::barcode {
    // The PDF417 barcodes in a picture. A reader holds only its effort: read() may be called from
    // several threads at once.
    class pdf417 {
    public:
        pdf417() = default;
        explicit pdf417(const effort &how) : how_{how} {}

        // The barcodes in picture, top to bottom. Throws std::invalid_argument for an empty picture or
        // a stride shorter than its width.
        [[nodiscard]] std::vector<code> read(const grey_view &picture) const;

        [[nodiscard]] const effort &how() const { return how_; }

    private:
        effort how_;
    };
}

#endif
