// sc::barcode::pdf417 on barcodes made here (zxing-cpp's writer, 2.3 or newer; skipped (77) without
// it): sharp, at about 1.5 pixels to a module and softened as a 200 dpi scan, two in one picture
// (top to bottom), one too small, a stride wider than the picture, tries used up (found, not
// decoded), nothing there, and the arguments refused.
#include <sc_barcode.h>
#include <sc_test.h>

#include <iostream>
#include <string>

#if __has_include(<ZXing/CreateBarcode.h>)
#include <ZXing/CreateBarcode.h>
#include <ZXing/WriteBarcode.h>
#include <opencv2/imgproc.hpp>

namespace {
    // A card's back: about 400 modules across (20 data columns), 3 pixels to a module, a white margin
    cv::Mat drawn(const std::string &bytes) {
        const auto symbol = ZXing::CreateBarcodeFromBytes(bytes, ZXing::CreatorOptions{ZXing::BarcodeFormat::PDF417, "columns=20"});
        const auto image = ZXing::WriteBarcodeToImage(symbol, ZXing::WriterOptions{}.scale(3).addQuietZones(true));
        cv::Mat out{image.height(), image.width(), CV_8UC1, const_cast<uint8_t *>(image.data()), static_cast<size_t>(image.rowStride())};
        out = out.clone();
        cv::copyMakeBorder(out, out, 60, 60, 60, 60, cv::BORDER_CONSTANT, 255);
        return out;
    }

    sc::barcode::grey_view view_of(const cv::Mat &grey) {
        return {grey.data, {grey.cols, grey.rows}, static_cast<std::ptrdiff_t>(grey.step)};
    }

    // Bytes as a card's barcode holds them: a length, binary (a ZIP), and random-looking bytes
    std::string payload(const char seed) {
        std::string out = "224";
        for (int i = 0; i < 700; ++i) out += static_cast<char>((i * 37 + seed * 11) % 256);
        return out;
    }
}

int main() {
    const auto bytes = payload(1);
    const auto sharp = drawn(bytes);

    SECTION("Sharp: decoded at once");
    {
        const auto found = sc::barcode::pdf417{}.read(view_of(sharp));
        CHECK_EQ(found.size(), std::size_t{1});
        CHECK(!found.empty() && found[0].decoded() && found[0].bytes == bytes && found[0].tries == 1);
        CHECK(!found.empty() && found[0].where.left() >= 40 && found[0].where.width() > 1000);
    }

    SECTION("1.5 pixels to a module, softened as a scanner does (a 200 dpi scan)");
    {
        cv::Mat soft;
        cv::resize(sharp, soft, {}, 0.5, 0.5, cv::INTER_AREA);
        cv::GaussianBlur(soft, soft, {0, 0}, 0.5);
        const auto found = sc::barcode::pdf417{}.read(view_of(soft));
        CHECK(found.size() == 1 && found[0].bytes == bytes);
    }

    // About 1.3 pixels to a module and blurred: found, but decoded only enlarged and thresholded, on a later try
    cv::Mat scan;
    cv::resize(sharp, scan, {}, 0.43, 0.43, cv::INTER_AREA);
    cv::GaussianBlur(scan, scan, {0, 0}, 0.7);

    SECTION("1.3 pixels to a module, blurred: decoded on a later try, enlarged");
    {
        const auto found = sc::barcode::pdf417{sc::barcode::effort{.max_tries = 400}}.read(view_of(scan));
        CHECK_EQ(found.size(), std::size_t{1});
        CHECK_MSG(!found.empty() && found[0].bytes == bytes, "after " + std::to_string(found.empty() ? 0 : found[0].tries) + " tries");
        CHECK(!found.empty() && found[0].scale > 3 && found[0].tries > 1);
    }

    SECTION("Tries used up: found, not decoded");
    {
        const auto found = sc::barcode::pdf417{sc::barcode::effort{.max_tries = 0}}.read(view_of(scan));
        CHECK(found.size() == 1 && !found[0].decoded() && found[0].tries == 0);
        const auto late = sc::barcode::pdf417{sc::barcode::effort{.time_limit = std::chrono::seconds{0}}}.read(view_of(scan));
        CHECK(late.size() == 1 && !late[0].decoded());
    }

    SECTION("Two in one picture, top to bottom; a stride wider than the picture");
    {
        const auto other = payload(2);
        cv::Mat both{sharp.rows * 2, sharp.cols + 37, CV_8UC1, cv::Scalar{255}};
        drawn(other).copyTo(both(cv::Rect{0, 0, sharp.cols, sharp.rows}));
        sharp.copyTo(both(cv::Rect{0, sharp.rows, sharp.cols, sharp.rows}));
        const cv::Mat left = both(cv::Rect{0, 0, sharp.cols, both.rows}); // its rows are both.cols bytes apart
        const auto found = sc::barcode::pdf417{}.read(view_of(left));
        CHECK_EQ(found.size(), std::size_t{2});
        CHECK(found.size() == 2 && found[0].bytes == other && found[1].bytes == bytes && found[0].where.top() < found[1].where.top());
    }

    SECTION("Too small for smallest: left out; smallest = 0 keeps it");
    {
        cv::Mat small;
        cv::resize(sharp, small, {}, 0.15, 0.15, cv::INTER_AREA);
        CHECK(sc::barcode::pdf417{sc::barcode::effort{.max_tries = 5}}.read(view_of(small)).empty());
        CHECK_EQ(sc::barcode::pdf417{sc::barcode::effort{.smallest = 0}}.read(view_of(sharp)).size(), std::size_t{1});
    }

    SECTION("Nothing there; the arguments refused");
    {
        const cv::Mat blank{300, 400, CV_8UC1, cv::Scalar{255}};
        CHECK(sc::barcode::pdf417{}.read(view_of(blank)).empty());
        CHECK_THROWS_AS((void) sc::barcode::pdf417{}.read({}), std::invalid_argument);
        CHECK_THROWS_AS((void) sc::barcode::pdf417{}.read({blank.data, {400, 300}, 100}), std::invalid_argument);
    }

    TEST_SUMMARY();
}
#else
int main() {
    std::cout << "SKIPPED: zxing-cpp has no barcode writer before 2.3\n";
    return 77;
}
#endif
