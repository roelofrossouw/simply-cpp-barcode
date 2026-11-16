#include <sc_pdf417.h>

#include <ZXing/ReadBarcode.h>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <functional>
#include <future>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace sc::pdf417 {
    namespace {
        // zxing-cpp 3 calls a result Barcode, 2.x Result: whatever ReadBarcodes() gives.
        using code = std::decay_t<decltype(ZXing::ReadBarcodes(std::declval<ZXing::ImageView>(), std::declval<ZXing::ReaderOptions>()).front())>;

        struct box {
            int x = 0, y = 0, width = 0, height = 0;
            [[nodiscard]] long long area() const { return static_cast<long long>(width) * height; }
        };

        box bounds(const ZXing::Position &position, const double scale) {
            int left = 1 << 30, top = 1 << 30, right = 0, bottom = 0;
            for (const auto &corner: {position.topLeft(), position.topRight(), position.bottomLeft(), position.bottomRight()}) {
                left = std::min(left, static_cast<int>(corner.x / scale));
                top = std::min(top, static_cast<int>(corner.y / scale));
                right = std::max(right, static_cast<int>(corner.x / scale));
                bottom = std::max(bottom, static_cast<int>(corner.y / scale));
            }
            return {left, top, std::max(1, right - left), std::max(1, bottom - top)};
        }

        // Most of the smaller one inside the other: the same barcode
        bool overlaps(const box &a, const box &b) {
            const int left = std::max(a.x, b.x), top = std::max(a.y, b.y);
            const int right = std::min(a.x + a.width, b.x + b.width), bottom = std::min(a.y + a.height, b.y + b.height);
            if (right <= left || bottom <= top) return false;
            const long long common = static_cast<long long>(right - left) * (bottom - top);
            return common * 2 >= std::min(a.area(), b.area());
        }

        ZXing::ReaderOptions options_for(const bool with_errors) {
            ZXing::ReaderOptions out;
            out.setFormats(ZXing::BarcodeFormat::PDF417);
            out.setTryHarder(true);
            out.setTryRotate(true);
            out.setTryDownscale(false);
            out.setReturnErrors(with_errors);
            return out;
        }

        auto decode(const cv::Mat &grey, const bool with_errors) {
            return ZXing::ReadBarcodes(ZXing::ImageView(grey.data, grey.cols, grey.rows, ZXing::ImageFormat::Lum, static_cast<int>(grey.step)),
                                       options_for(with_errors));
        }

        cv::Mat enlarged(const cv::Mat &grey, const double scale, const int interpolation, const int block, const int offset, const bool mean) {
            cv::Mat out;
            cv::resize(grey, out, {}, scale, scale, interpolation);
            cv::adaptiveThreshold(out, out, 255, mean ? cv::ADAPTIVE_THRESH_MEAN_C : cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY,
                                  static_cast<int>(block * scale) | 1, offset);
            return out;
        }

        std::string bytes_of(const code &found) {
            const auto &bytes = found.bytes();
            return {bytes.begin(), bytes.end()};
        }

        struct working {
            box where;
            std::string bytes;
            int tries = 0;
            double scale = 1;
        };
    }

    std::vector<barcode> read(const grey_view &picture, const options &how) {
        if (!picture.pixels || picture.size.width() <= 0 || picture.size.height() <= 0)
            throw std::invalid_argument{"sc::pdf417::read: an empty picture"};
        const auto stride = picture.stride == 0 ? static_cast<std::ptrdiff_t>(picture.size.width()) : picture.stride;
        if (stride < picture.size.width()) throw std::invalid_argument{"sc::pdf417::read: a stride shorter than the width"};
        const cv::Mat grey{picture.size.height(), picture.size.width(), CV_8UC1, const_cast<std::uint8_t *>(picture.pixels), static_cast<std::size_t>(stride)};
        const auto deadline = std::chrono::steady_clock::now() + how.time_limit;

        std::vector<working> out;
        const auto add = [&](const code &found, const double scale) {
            const auto where = bounds(found.position(), scale);
            for (auto &known: out)
                if (overlaps(known.where, where)) {
                    if (known.bytes.empty() && found.isValid()) {
                        known.bytes = bytes_of(found);
                        known.scale = scale;
                    }
                    return;
                }
            if (std::max(where.width, where.height) < how.smallest || std::min(where.width, where.height) < how.smallest / 4) return;
            out.push_back({where, found.isValid() ? bytes_of(found) : std::string{}, 1, scale});
        };
        // Where they are: as it is, and twice the size thresholded (finds the small ones).
        for (const auto &found: decode(grey, true)) add(found, 1);
        for (const auto &found: decode(enlarged(grey, 2, cv::INTER_CUBIC, 31, 10, false), true)) add(found, 2);

        // Each one not decoded, in its own band across the picture (a tight crop decodes worse).
        const auto retry = [&grey, &how, deadline](working &one) {
            const int margin = std::max(40, std::min(one.where.width, one.where.height) / 4);
            const int top = std::max(0, one.where.y - margin);
            const int bottom = std::min(grey.rows, one.where.y + one.where.height + margin);
            const cv::Mat band = grey.rowRange(top, bottom);
            one.tries = 0;
            for (const int block: {31, 28, 34, 25, 37})
                for (const int offset: {10, 9, 11, 7, 13})
                    for (const int tenths: {40, 38, 42, 36, 44, 34, 46})
                        for (const int interpolation: {cv::INTER_LANCZOS4, cv::INTER_CUBIC})
                            for (const bool mean: {false, true}) {
                                if (one.tries >= how.max_tries || std::chrono::steady_clock::now() >= deadline) return;
                                ++one.tries;
                                const double scale = tenths / 10.0;
                                for (const auto &decoded: decode(enlarged(band, scale, interpolation, block, offset, mean), false)) {
                                    auto where = bounds(decoded.position(), scale);
                                    where.y += top;
                                    if (!overlaps(one.where, where)) continue;
                                    one.bytes = bytes_of(decoded);
                                    one.scale = scale;
                                    return;
                                }
                            }
        };
        std::vector<std::future<void>> running;
        for (auto &one: out)
            if (one.bytes.empty()) running.push_back(std::async(std::launch::async, retry, std::ref(one)));
        for (auto &each: running) each.get();
        std::ranges::sort(out, {}, [](const working &each) { return each.where.y; });

        std::vector<barcode> result;
        result.reserve(out.size());
        for (auto &one: out)
            result.push_back({sc::rect_i{one.where.x, one.where.y, one.where.width, one.where.height}, std::move(one.bytes), one.tries, one.scale});
        return result;
    }
}
