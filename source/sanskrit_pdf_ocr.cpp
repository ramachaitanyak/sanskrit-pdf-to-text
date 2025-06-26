#include <iostream>
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./sanskrit_pdf_ocr file.pdf\n";
        return 1;
    }

    std::string pdf_path = argv[1];
    std::unique_ptr<poppler::document> doc = poppler::document::load_from_file(pdf_path);
    if (!doc) {
        std::cerr << "Failed to open PDF file\n";
        return 1;
    }

    tesseract::TessBaseAPI ocr;
    if (ocr.Init(NULL, "hin")) {  // 'hin' works well for Devanagari (Sanskrit)
        std::cerr << "Tesseract OCR initialization failed\n";
        return 1;
    }

    poppler::page_renderer renderer;
    renderer.set_render_hint(poppler::page_renderer::text_antialiasing);

    for (int i = 0; i < doc->pages(); ++i) {
        std::unique_ptr<poppler::page> pg = doc->create_page(i);
        if (!pg) continue;

        // Try direct text extraction
        std::string text = pg->text().to_utf8();
        if (!text.empty() && text.find_first_not_of(" \n\t") != std::string::npos) {
            std::cout << "[Text Page " << i + 1 << "]:\n" << text << "\n\n";
            continue;
        }

        // Render image and OCR
        poppler::image img = renderer.render_page(pg.get(), 300, 300);
        if (!img.is_valid()) continue;

        // Convert poppler::image to Leptonica Pix
        Pix *pixs = pixCreate(img.width(), img.height(), 32);
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                poppler::rgba color = img.pixel(x, y);
                pixSetPixel(pixs, x, y, (color.alpha << 24) | (color.red << 16) | (color.green << 8) | color.blue);
            }
        }

        ocr.SetImage(pixs);
        char* outText = ocr.GetUTF8Text();
        std::cout << "[OCR Page " << i + 1 << "]:\n" << outText << "\n\n";

        delete[] outText;
        pixDestroy(&pixs);
    }

    ocr.End();
    return 0;
}

