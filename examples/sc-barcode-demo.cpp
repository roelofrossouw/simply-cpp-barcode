// sc-barcode's example: the PDF417 barcodes in a picture, with where they are and what they hold.
//
//     sc-barcode-demo PICTURE
#include <sc_barcode.h>

#include <params.h>

#include <opencv2/imgcodecs.hpp>

#include <cctype>
#include <iostream>

int main(int argc, char **argv) {
    const sc::params args{argc, argv, {
        {"description", "Finds and decodes the PDF417 barcodes in a picture (a scan or a photo), and prints what each holds."},
        {"positional", {{"picture", {{"help", "a JPEG, PNG or other picture OpenCV reads"}}}}}}};

    const std::string file = args["picture"];
    const cv::Mat grey = cv::imread(file, cv::IMREAD_GRAYSCALE);
    if (grey.empty()) {
        std::cerr << "can't read " << file << "\n";
        return 1;
    }
    for (const auto &code: sc::barcode::pdf417{}.read({grey.data, {grey.cols, grey.rows}, static_cast<std::ptrdiff_t>(grey.step)})) {
        std::cout << "at " << code.where.left() << "," << code.where.top() << " " << code.where.width() << "x" << code.where.height() << ": ";
        if (!code.decoded()) {
            std::cout << "not decoded after " << code.tries << " tries\n";
            continue;
        }
        std::cout << code.bytes.size() << " bytes (" << code.tries << " tries): ";
        for (const unsigned char c: code.bytes.substr(0, 80)) std::cout << (std::isprint(c) ? static_cast<char>(c) : '.');
        std::cout << "\n";
    }
}
