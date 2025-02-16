//pdfviewer.cpp
#include "pdfviewer.hpp"
#include <mupdf/fitz.h>
#include <tesla.hpp>
#include <iostream>
#include <string>

// Structure to hold PDF data
struct PDFData {
    fz_context* ctx;
    fz_document* document;
    int currentPage = 0;
    int totalPages = 0;
    bool fullScreen = true;
    bool doublePageView = true;
    float zoomLevel = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
};

// Global PDF data structure
PDFData pdf;

// Function to open a PDF file
void OpenPDF(const std::string& path) {
    pdf.ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!pdf.ctx) {
        std::cerr << "Error: Cannot initialize MuPDF context" << std::endl;
        return;
    }

    fz_register_document_handlers(pdf.ctx);
    fz_try(pdf.ctx) {
        pdf.document = fz_open_document(pdf.ctx, path.c_str());
        pdf.totalPages = fz_count_pages(pdf.ctx, pdf.document);
        pdf.currentPage = 0;
    } fz_catch(pdf.ctx) {
        std::cerr << "Error: Cannot open PDF document" << std::endl;
        fz_drop_context(pdf.ctx);
        return;
    }
}

// Function to handle user input for PDF navigation
bool HandlePDFInput(u64 keysDown, u64 keysHeld, const HidTouchState&, HidAnalogStickState joyStickPosLeft, HidAnalogStickState) {
    // Zoom in and out
    if (keysHeld & HidNpadButton_ZR) {
        pdf.zoomLevel += 0.1f;
    }
    if (keysHeld & HidNpadButton_ZL) {
        pdf.zoomLevel -= 0.1f;
        if (pdf.zoomLevel < 0.1f) pdf.zoomLevel = 0.1f;
    }

    // Pan the view using the left analog stick
    pdf.panX += joyStickPosLeft.x * 0.05f;
    pdf.panY += joyStickPosLeft.y * 0.05f;

    // Toggle fullscreen
    if (keysDown & HidNpadButton_R) {
        pdf.fullScreen = !pdf.fullScreen;
    }

    // Toggle double-page view
    if (keysDown & HidNpadButton_StickR) {
        pdf.doublePageView = !pdf.doublePageView;
    }

    // Navigate pages
    if (keysDown & HidNpadButton_L) {
        if (pdf.currentPage > 0) {
            pdf.currentPage--;
        }
    }
    if (keysDown & HidNpadButton_R) {
        if (pdf.currentPage < pdf.totalPages - 1) {
            pdf.currentPage++;
        }
    }

    // Navigate page pairs
    if (keysDown & HidNpadButton_StickL) {
        pdf.currentPage += (pdf.currentPage % 2 == 0) ? 2 : -1;
        if (pdf.currentPage >= pdf.totalPages) pdf.currentPage = pdf.totalPages - 1;
    }

    // Exit on B
    if (keysDown & HidNpadButton_B) {
        return false;
    }

    return true;
}

// Function to render the current PDF page
void RenderPDF(tsl::gfx::Renderer* renderer) {
    if (!pdf.document) return;

    fz_page* page = nullptr;
    fz_pixmap* pixmap = nullptr;

    fz_try(pdf.ctx) {
        page = fz_load_page(pdf.ctx, pdf.document, pdf.currentPage);
        fz_rect bbox = fz_bound_page(pdf.ctx, page);
        fz_irect ibbox = fz_round_rect(bbox);

        pixmap = fz_new_pixmap_with_bbox(pdf.ctx, fz_device_rgb(pdf.ctx), ibbox, nullptr, pdf.zoomLevel);
        fz_device* dev = fz_new_draw_device(pdf.ctx, fz_identity, pixmap);
        fz_run_page(pdf.ctx, page, dev, fz_identity, nullptr);
        fz_drop_device(pdf.ctx, dev);

        // Render the page to the screen
        // (This should be implemented properly, drawing the pixmap)

        // Display the current page number
        std::string pageText = "Page " + std::to_string(pdf.currentPage + 1) + " / " + std::to_string(pdf.totalPages);
        renderer->drawString(pageText.c_str(), false, 20, 30, 20, renderer->a(0xFFFF));

    } fz_catch(pdf.ctx) {
        std::cerr << "Error: Failed to render page" << std::endl;
    }

    if (pixmap) {
        fz_drop_pixmap(pdf.ctx, pixmap);
    }
    if (page) {
        fz_drop_page(pdf.ctx, page);
    }
}
