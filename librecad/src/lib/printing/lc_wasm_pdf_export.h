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

#ifndef LC_WASM_PDF_EXPORT_H
#define LC_WASM_PDF_EXPORT_H

// Wasm-only PDF export via QPdfWriter (QtGui) + QFileDialog::saveFileContent.
// On the wasm target Qt6::PrintSupport (QPrinter/QPrintDialog) is not
// available, so the regular lc_printing.cpp path is compiled out via
// LC_NO_PRINT. This module provides an equivalent render path that writes
// the PDF to a QByteArray and triggers a browser download.
//
// Build only under Q_OS_WASM; the .cpp guards the entire implementation
// with `#ifdef Q_OS_WASM`, so on desktop this header is a no-op.

class QC_MDIWindow;
class QByteArray;
class QString;

namespace LC_Printing
{
    // Export the drawing in `mdiWindow` to a PDF and trigger a browser
    // download via QFileDialog::saveFileContent().  No-op on non-wasm targets.
    void WasmExportPDF(QC_MDIWindow &mdiWindow);
}

namespace lc_wasm
{
    // Trigger a browser download of `data` as `fileName`. Implemented in JS
    // (Blob + <a download>) instead of QFileDialog::saveFileContent(), which on
    // the JSPI build prefers showSaveFilePicker() + a chunked writable-stream
    // write that does not reliably deliver the bytes. No-op on non-wasm targets.
    void downloadData(const QByteArray &data, const QString &fileName);
}

#endif // LC_WASM_PDF_EXPORT_H
