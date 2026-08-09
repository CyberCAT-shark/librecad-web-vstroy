/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 dongxuli2011@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "lc_wasm_pdf_export.h"

// Pull in Qt's qsystemdetection.h so Q_OS_WASM is defined below.  Without
// an early Qt include, the `#ifdef Q_OS_WASM` guard sees an undefined
// macro and the whole implementation is compiled out — which then fails
// to resolve at link time.
#include <QtCore/qglobal.h>

#ifdef Q_OS_WASM

#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QFileDialog>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QString>

#include <emscripten.h>

#include "lc_graphicviewport.h"
#include "lc_printpreviewview.h"
#include "lc_printviewportrenderer.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_painter.h"
#include "rs_settings.h"
#include "rs_units.h"

namespace {

// Mirror of paperToPage in lc_printing.cpp — kept local to avoid pulling
// the PrintSupport-dependent translation unit into the wasm build.
const std::map<RS2::PaperFormat, QPageSize::PageSizeId> paperToPage = {
    {RS2::A0, QPageSize::A0},
    {RS2::A1, QPageSize::A1},
    {RS2::A2, QPageSize::A2},
    {RS2::A3, QPageSize::A3},
    {RS2::A4, QPageSize::A4},
    {RS2::Letter, QPageSize::Letter},
    {RS2::Legal,  QPageSize::Legal},
    {RS2::Tabloid, QPageSize::Tabloid},
    {RS2::Ansi_C, QPageSize::AnsiC},
    {RS2::Ansi_D, QPageSize::AnsiD},
    {RS2::Ansi_E, QPageSize::AnsiE},
    {RS2::Arch_A, QPageSize::ArchA},
    {RS2::Arch_B, QPageSize::ArchB},
    {RS2::Arch_C, QPageSize::ArchC},
    {RS2::Arch_D, QPageSize::ArchD},
    {RS2::Arch_E, QPageSize::ArchE},
};

QPageSize::PageSizeId rsToQtPaperFormat(RS2::PaperFormat paper)
{
    return (paperToPage.count(paper) == 1) ? paperToPage.at(paper) : QPageSize::Custom;
}

} // namespace

namespace lc_wasm {

void downloadData(const QByteArray &data, const QString &fileName)
{
    const QByteArray nameUtf8 = fileName.toUtf8();
    // Copy the bytes out of the (possibly growable) wasm heap into a JS Blob and
    // click a synthetic <a download> link. Runs synchronously within the calling
    // user gesture so the browser allows the download.
    EM_ASM({
        const ptr = $0;
        const size = $1;
        const name = UTF8ToString($2);
        const bytes = HEAPU8.slice(ptr, ptr + size); // detached copy
        const blob = new Blob([bytes], { type: 'application/octet-stream' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = name || 'drawing.dxf';
        a.style.display = 'none';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        setTimeout(function() { URL.revokeObjectURL(url); }, 2000);
    }, reinterpret_cast<uintptr_t>(data.constData()), (int)data.size(), nameUtf8.constData());
}

} // namespace lc_wasm

namespace LC_Printing {

void WasmExportPDF(QC_MDIWindow &mdiWindow)
{
    RS_Graphic *graphic = mdiWindow.getDocument()->getGraphic();
    if (graphic == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "LC_Printing::WasmExportPDF: no graphic");
        return;
    }

    QByteArray pdfData;
    QBuffer buffer(&pdfData);
    if (!buffer.open(QIODevice::WriteOnly)) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "LC_Printing::WasmExportPDF: cannot open buffer");
        return;
    }

    QPdfWriter pdfWriter(&buffer);
    pdfWriter.setResolution(1200);
    pdfWriter.setPageMargins(QMarginsF{}, QPageLayout::Millimeter);

    bool landscape = false;
    RS2::PaperFormat paperformat = graphic->getPaperFormat(&landscape);
    QPageSize::PageSizeId paperSizeName = rsToQtPaperFormat(paperformat);
    RS_Vector paperSize = graphic->getPaperSize();
    RS2::Unit unit = graphic->getUnit();
    if (paperSizeName == QPageSize::Custom) {
        RS_Vector s = RS_Units::convert(paperSize, unit, RS2::Millimeter);
        if (landscape) s = s.flipXY();
        pdfWriter.setPageSize(QPageSize{QSizeF(s.x, s.y), QPageSize::Millimeter});
    } else {
        pdfWriter.setPageSize(QPageSize{paperSizeName});
    }
    pdfWriter.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);

    const auto printMargins = graphic->activeLayoutMargins();
    QMarginsF paperMargins{printMargins[0],   // left
                           printMargins[1],   // top
                           printMargins[2],   // right
                           printMargins[3]};  // bottom
    pdfWriter.setPageMargins(paperMargins, QPageLayout::Millimeter);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    std::shared_ptr<void> cursorGuard{nullptr, []([[maybe_unused]] void *) {
        QApplication::restoreOverrideCursor();
    }};

    RS_Painter painter(&pdfWriter);

    QG_GraphicView *graphicView = mdiWindow.getGraphicView();
    RS2::DrawingMode drawingMode = RS2::DrawingMode::ModeAuto;
    if (graphicView->isPrintPreview()){
        LC_PrintPreviewView* printPreview = dynamic_cast<LC_PrintPreviewView *>(graphicView);
        if (printPreview != nullptr){
            drawingMode = printPreview->getDrawingMode();
        }
    }
    painter.setDrawingMode(drawingMode);

    QMarginsF margins = pdfWriter.pageLayout().margins(QPageLayout::Millimeter);

    double printerWidth  = pdfWriter.width();
    double printerHeight = pdfWriter.height();

    double printerFx = (double) printerWidth  / pdfWriter.widthMM();
    double printerFy = (double) printerHeight / pdfWriter.heightMM();

    painter.setClipRect(margins.left() * printerFx, margins.top() * printerFy,
                        printerWidth  - (margins.left() + margins.right())  * printerFx,
                        printerHeight - (margins.top()  + margins.bottom()) * printerFy);

    LC_GraphicViewport viewport;
    viewport.setContainer(graphic);
    viewport.setBorders(0, 0, 0, 0);
    viewport.setSize(printerWidth, printerHeight);

    LC_PrintViewportRenderer renderer(&viewport, &painter);
    viewport.loadSettings();
    renderer.loadSettings();

    bool scaleLineWidth = mdiWindow.getGraphicView()->getLineWidthScaling();
    renderer.setLineWidthScaling(scaleLineWidth);

    double fx = printerFx * RS_Units::getFactorToMM(unit);
    double fy = printerFy * RS_Units::getFactorToMM(unit);
    double f = (fx + fy) / 2.0;

    double scale  = graphic->getPaperScale();
    double factor = f * scale;

    double baseX = graphic->getPaperInsertionBase().x;
    double baseY = graphic->getPaperInsertionBase().y;

    int numX = graphic->getPagesNumHoriz();
    int numY = graphic->getPagesNumVert();
    RS_Vector printArea = graphic->getPrintAreaSize(false);

    for (int pY = 0; pY < numY; pY++) {
        double offsetY = printArea.y * pY;
        for (int pX = 0; pX < numX; pX++) {
            double offsetX = printArea.x * pX;
            // First page is created automatically; extra pages need newPage().
            if (pX > 0 || pY > 0) {
                pdfWriter.newPage();
            }
            viewport.justSetOffsetAndFactor((int) ((baseX - offsetX) * f),
                                            (int) ((baseY - offsetY) * f),
                                            factor);
            painter.setViewPort(&viewport);
            renderer.render();
        }
    }

    painter.end();
    buffer.close();

    LC_GROUP_GUARD("Print");
    {
        LC_SET("FileName", "librecad_export.pdf");
    }

    // Trigger a browser download of the generated PDF.
    lc_wasm::downloadData(pdfData, "librecad_export.pdf");
}

} // namespace LC_Printing

#endif // Q_OS_WASM
