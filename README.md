# simply-cpp-barcode

Barcodes for simply-cpp, read from pictures: each format is a reader class over the same picture and
result types. [zxing-cpp](https://github.com/zxing-cpp/zxing-cpp) does the decoding and OpenCV the image
work; neither shows in the API. Making QR codes is [sc-qrencode](https://github.com/roelofrossouw/simply-cpp-qrencode)'s,
which stays small (no image library).

| Reader | Reads |
|---|---|
| `sc::barcode::pdf417` | PDF417 (ISO/IEC 15438), also from poor scans |

```cpp
#include <sc/barcode/barcode.h>

const sc::barcode::grey_view picture{pixels, {width, height}, stride};   // 8-bit grey, not owned
const sc::barcode::pdf417 reader{{.max_tries = 1500, .time_limit = std::chrono::seconds{20}}};
for (const auto &code: reader.read(picture)) {
    if (code.decoded()) use(code.bytes);                // the bytes as encoded
    else std::cerr << "a barcode at " << code.where.left() << "," << code.where.top() << " that didn't decode\n";
}
```

## PDF417

- **Finds** every PDF417 in the picture, as it is and enlarged twice and thresholded (which finds small
  ones), and returns them top to bottom (`sc::barcode::code`): where (`sc::rect_i`, the picture's
  pixels), the bytes, the tries it took and the enlargement it decoded at.
- **Decodes poor scans.** A barcode with too few pixels to a module for a decoder as it is (a 200 dpi
  scan of a card has about 1.4) is tried again in its own band of the picture (a tight crop decodes
  worse), enlarged 3.4 to 4.6 times with Lanczos or cubic interpolation and thresholded locally
  (Gaussian or mean, several block sizes and offsets), the settings that read such scans first. Each
  such barcode is tried in a thread of its own, up to `max_tries` (1500, about 4 ms each on a card's
  band) or until `time_limit` (30 s): the reader's `sc::barcode::effort`.
- **Found but not decoded** is still returned, with no bytes (`decoded()` false), so the caller knows a
  barcode is there.
- **Small ones are left out**: a barcode whose long side is under `smallest` pixels (250: a card's,
  about 400 modules across, can't be decoded below it). 0 returns every one found.

For a card, scan at 400 dpi or more (300 at least) or photograph it at least about 1500 pixels wide:
then it decodes at once. A reader holds only its effort, so `read()` may be called from several
threads at once.

## Building and testing

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

It needs sc-core, zxing-cpp (`libzxing-dev`; Homebrew `zxing-cpp`) and OpenCV (`libopencv-dev`;
Homebrew `opencv`), found or installed by CMake. The test makes barcodes with zxing-cpp's writer where
it has one (Homebrew's; Ubuntu's zxing-cpp is built without it, so there the same barcodes come from
`tests/data`, rewritten by `SC_BARCODE_WRITE_FIXTURES=1`) and reads them sharp, as a 200 dpi scan,
blurred past a first reading, two in one picture, too small, and with the tries used up. It passes
with zxing-cpp 2.2 (Ubuntu noble), 2.3 (resolute) and 3.1 (Homebrew); 2.2 decodes the hardest scans
less often.

`sc-barcode-demo PICTURE` prints the barcodes in a picture and what each holds.

## License

Apache License 2.0 (see `LICENSE` and `NOTICE`). zxing-cpp and OpenCV are Apache-2.0 too.
