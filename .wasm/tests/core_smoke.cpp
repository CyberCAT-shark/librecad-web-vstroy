/*
 * LibreCAD WASM Core Smoke Test
 *
 * Loads a DXF file from MEMFS, walks entities, computes bounding box,
 * re-serializes to DXF, and generates SVG. Output goes to JS console.
 */

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include <QCoreApplication>
#include <QString>

#include "rs.h"
#include "rs_graphic.h"
#include "rs_filterdxfrw.h"
#include "rs_entitycontainer.h"
#include "rs_entity.h"
#include "rs_settings.h"
#include "rs_vector.h"
#include "lc_makercamsvg.h"
#include "lc_xmlwriterqxmlstreamwriter.h"

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    std::printf("=== LibreCAD WASM Core Smoke Test ===\n");

    // Qt application + settings singleton must exist before constructing
    // RS_Graphic, whose constructor reads default settings (unit, grid, ...).
    static int qargc = 1;
    static char qarg0[] = "core_smoke";
    static char *qargv[] = {qarg0, nullptr};
    if (!QCoreApplication::instance())
        new QCoreApplication(qargc, qargv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    std::printf("    Settings initialised\n");

    // -----------------------------------------------------------------------
    // 1. Create a graphic document
    // -----------------------------------------------------------------------
    RS_Graphic graphic;

    // -----------------------------------------------------------------------
    // 2. Import the preloaded DXF file via RS_FilterDXFRW
    //    Signature: bool fileImport(RS_Graphic& g, const QString& file,
    //                               RS2::FormatType type)
    // -----------------------------------------------------------------------
    RS_FilterDXFRW filter;
    const QString inputFile = QStringLiteral("/sample.dxf");

    std::printf("[1] Importing DXF: %s\n", inputFile.toUtf8().constData());
    if (!filter.fileImport(graphic, inputFile, RS2::FormatDXFRW)) {
        std::printf("ERROR: DXF import failed\n");
        return 1;
    }
    std::printf("    DXF import: OK\n");

    // -----------------------------------------------------------------------
    // 3. Calculate bounding box
    //    RS_EntityContainer::calculateBorders() updates minV/maxV on the
    //    graphic (inherited from RS_Entity).
    // -----------------------------------------------------------------------
    graphic.calculateBorders();

    // -----------------------------------------------------------------------
    // 4. Print entity count and bounding box
    //    count()      -> unsigned (RS_EntityContainer)
    //    getMin()     -> RS_Vector (RS_Entity, inline)
    //    getMax()     -> RS_Vector (RS_Entity, inline)
    //    getSize()    -> RS_Vector (RS_Entity)
    //    RS_Vector has public double x, y, z and bool valid.
    // -----------------------------------------------------------------------
    const unsigned entityCount = graphic.count();
    std::printf("[2] Entity count: %u\n", entityCount);

    const RS_Vector minV = graphic.getMin();
    const RS_Vector maxV = graphic.getMax();
    const RS_Vector sizeV = graphic.getSize();
    std::printf("[3] Bounding box:\n");
    std::printf("    min  = (%.4f, %.4f)\n", minV.x, minV.y);
    std::printf("    max  = (%.4f, %.4f)\n", maxV.x, maxV.y);
    std::printf("    size = (%.4f, %.4f)\n", sizeV.x, sizeV.y);

    // Walk the first few entities and print their type IDs
    const unsigned walkCount = entityCount < 5 ? entityCount : 5;
    std::printf("[4] First %u entities (rtti type id):\n", walkCount);
    for (unsigned i = 0; i < walkCount; ++i) {
        RS_Entity* e = graphic.entityAt(static_cast<int>(i));
        if (e) {
            std::printf("    [%u] rtti=%d\n", i,
                        static_cast<int>(e->rtti()));
        }
    }

    // -----------------------------------------------------------------------
    // 5. Export to /output.dxf (MEMFS) via RS_FilterDXFRW::fileExport
    //    Signature: bool fileExport(RS_Graphic& g, const QString& file,
    //                               RS2::FormatType type)
    // -----------------------------------------------------------------------
    const QString outputFile = QStringLiteral("/output.dxf");
    std::printf("[5] Exporting DXF: %s\n", outputFile.toUtf8().constData());
    if (!filter.fileExport(graphic, outputFile, RS2::FormatDXFRW2018)) {
        std::printf("ERROR: DXF export failed\n");
        return 1;
    }
    std::printf("    DXF export: OK\n");

    // -----------------------------------------------------------------------
    // 6. Generate SVG via LC_MakerCamSVG
    //    Constructor: LC_MakerCamSVG(unique_ptr<LC_XMLWriterInterface>, ...)
    //    generate(RS_Graphic*) -> bool
    //    resultAsString()      -> std::string
    // -----------------------------------------------------------------------
    std::printf("[6] Generating SVG...\n");
    auto xmlWriter = std::make_unique<LC_XMLWriterQXmlStreamWriter>();
    LC_MakerCamSVG svgGenerator(std::move(xmlWriter));
    const bool svgOk = svgGenerator.generate(&graphic);
    const std::string svgResult = svgGenerator.resultAsString();

    if (!svgOk) {
        std::printf("    WARNING: SVG generate() returned false\n");
    }
    std::printf("    SVG length: %zu characters\n", svgResult.size());
    if (svgResult.size() > 0) {
        const size_t previewLen = svgResult.size() < 200 ? svgResult.size() : 200;
        std::printf("    SVG preview:\n");
        std::printf("    ");
        std::fwrite(svgResult.c_str(), 1, previewLen, stdout);
        std::printf("\n");
    }

    // -----------------------------------------------------------------------
    // 7. Success
    // -----------------------------------------------------------------------
    std::printf("\nSMOKE TEST PASSED\n");
    return 0;
}
