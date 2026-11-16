set(SC_TEST_LINK_LIBRARIES sc-barcode)

# Barcodes made with zxing-cpp's writer where it has one, else the same ones from tests/data.
add_sc_test(pdf417 LABELS unit TIMEOUT 120)
target_include_directories(test-pdf417 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(test-pdf417 PRIVATE ${OpenCV_LIBS})
target_compile_definitions(test-pdf417 PRIVATE SC_BARCODE_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data")
