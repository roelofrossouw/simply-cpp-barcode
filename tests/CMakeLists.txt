set(SC_TEST_LINK_LIBRARIES sc-pdf417)

# Barcodes made with zxing-cpp's writer (2.3 or newer; skipped, 77, before it).
add_sc_test(pdf417 LABELS unit TIMEOUT 120)
set_tests_properties(test-pdf417 PROPERTIES SKIP_RETURN_CODE 77)
target_include_directories(test-pdf417 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(test-pdf417 PRIVATE ${OpenCV_LIBS})
