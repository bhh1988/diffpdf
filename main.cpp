/*
    Copyright © 2008-13 Qtrac Ltd. All rights reserved.
    This program or module is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 2 of
    the License, or (at your option) any later version. This program is
    distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
    for more details.
*/

#include "mainwindow.hpp"
#include <QApplication>
#include <QTextStream>

int main(int argc, char *argv[])
{
    // ====================================
    // ARGUMENTS
    // ====================================
    InitialComparisonMode comparisonMode = CompareWords;
    QString filename1;
    QString filename2;
    PdfDocument pdf1;
    PdfDocument pdf2;
    QString saveFilename;
    bool printSeparate = false;
    QString pageRangeDoc1;
    QString pageRangeDoc2;
    bool useComposition = false;
    QPainter::CompositionMode compositionMode = QPainter::RasterOp_SourceXorDestination;
    bool combineHighlightedWords = false;
    int overlap = 5; // This together with zoom (don't know why yet) affects
                     // whether adjacent highlighted words get combined

    // visual preferences
    int squareSize = 5; // GUI limits >=2. Affects performance
    int zoom = 2; // GUI limits 1-8. Affects performance ~O(n^2)
    int opacity = 50; // 0-100%
    Qt::PenStyle penStyle = Qt::NoPen; // see optionsform.cpp for available options
    QColor penColor;
    penColor.setNamedColor("tomato");
    Qt::BrushStyle brushStyle = Qt::SolidPattern; // see optionsform.cpp
    QColor brushColor;
    brushColor.setNamedColor("tomato");

    // margins (to exclude when doing the diff)
    bool margins = false;
    int topMargin = 0;
    int leftMargin = 0;
    int rightMargin = 0;
    int bottomMargin = 0;
    // ====================================
    // END ARGUMENTS
    // ====================================




    // ====================================
    // COMMAND LINE PARSING
    // ====================================
    QApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);
    PdfLoader pdfLoader;
    QTextStream out(stdout);

    bool optionsOK = true;
    Debug debug = DebugOff;
    foreach (QString arg, args) {
        if (optionsOK && (arg == "--visual" || arg == "-V"))
            comparisonMode = CompareVisual;
        else if (optionsOK && (arg == "--characters" || arg == "-c"))
            comparisonMode = CompareCharacters;
        else if (optionsOK && (arg == "--words" || arg == "-w"))
            comparisonMode = CompareWords;
        else if (optionsOK && (arg == "--printSeparate" || arg == "-s"))
            printSeparate = true;
        else if (optionsOK && arg.startsWith("--output="))
        {
            // TODO(bhuh): validate path here
            saveFilename = arg.remove(0, 9);
        }
        else if (optionsOK && arg.startsWith("--pagesDoc1="))
            pageRangeDoc1 = arg.remove(0, 12);
        else if (optionsOK && arg.startsWith("--pagesDoc2="))
            pageRangeDoc2 = arg.remove(0, 12);
        else if (optionsOK && arg.startsWith("--compositionMode="))
        {
            useComposition = true;
            QString argCopy(arg);
            QString value = argCopy.remove(0, 18);
            // TODO: investigate the many other QPainter composition modes
            if (value == "Difference")
                compositionMode = QPainter::CompositionMode_Difference;
            else if (value == "Exclusion")
                compositionMode = QPainter::CompositionMode_Exclusion;
            else if (value == "SourceXorDestination")
                compositionMode = QPainter::RasterOp_SourceXorDestination;
            else if (value == "NotSourceXorDestination")
                compositionMode = QPainter::RasterOp_NotSourceXorDestination;
            else
            {
                out << "invalid value for arg '" << argCopy << "'\n";
                return 0;
            }
        }
        else if (optionsOK && (arg == "--combineHighlight" || arg == "-H"))
            combineHighlightedWords = true;
        else if (optionsOK && arg.startsWith("--overlap="))
        {
            bool isInt;
            QString argCopy(arg);
            overlap = argCopy.remove(0, 10).toInt(&isInt);
            if (!isInt)
            {
                out << "value for arg '" << argCopy << "' must be an int.\n";
                return 0;
            }
        }
        else if (optionsOK && arg.startsWith("--squareSize="))
        {
            bool isInt;
            QString argCopy(arg);
            squareSize = arg.remove(0, 13).toInt(&isInt);
            if (!isInt)
            {
                out << "value for arg '" << argCopy << "' must be an int.\n";
                return 0;
            }
            if (squareSize < 2)
            {
                out << "Warning: value for arg '" << argCopy << "' recommended to be >= 2.\n";
            }
        }
        else if (optionsOK && arg.startsWith("--zoom="))
        {
            bool isInt;
            QString argCopy(arg);
            zoom = arg.remove(0, 7).toInt(&isInt);
            if (!isInt)
            {
                out << "value for arg '" << argCopy << "' must be an int.\n";
                return 0;
            }
            if (zoom < 1 || zoom > 8)
            {
                out << "Warning: value for arg '" << argCopy << "' should be between 1 and 8.\n";
            }
        }
        else if (optionsOK && arg.startsWith("--opacity="))
        {
            bool isInt;
            QString argCopy(arg);
            opacity = arg.remove(0, 10).toInt(&isInt);
            if (!isInt)
            {
                out << "value for arg '" << argCopy << "' must be an int.\n";
                return 0;
            }
            if (opacity < 1 || opacity > 100)
            {
                out << "Warning: value for arg '" << argCopy << "' should be between 1 and 100.\n";
            }
        }
        else if (optionsOK && arg.startsWith("--penStyle="))
        {
            QString argCopy(arg);
            QString value = argCopy.remove(0, 11);
            if (value == "NoPen")
                penStyle = Qt::NoPen;
            else if (value == "SolidLine")
                penStyle = Qt::SolidLine;
            else if (value == "DashLine")
                penStyle = Qt::DashLine;
            else if (value == "DotLine")
                penStyle = Qt::DotLine;
            else if (value == "DashDotLine")
                penStyle = Qt::DashDotLine;
            else if (value == "DashDotDotLine")
                penStyle = Qt::DashDotDotLine;
            else
            {
                out << "invalid value for arg '" << arg << "'\n";
                return 0;
            }
        }
        else if (optionsOK && arg.startsWith("--penColor="))
            penColor.setNamedColor(arg.remove(0, 11));
        else if (optionsOK && arg.startsWith("--brushStyle="))
        {
            QString argCopy(arg);
            QString value = argCopy.remove(0, 13);
            if (value == "NoBrush")
                brushStyle = Qt::NoBrush;
            else if (value == "SolidPattern")
                brushStyle = Qt::SolidPattern;
            else if (value == "Dense1Pattern")
                brushStyle = Qt::Dense1Pattern;
            else if (value == "Dense2Pattern")
                brushStyle = Qt::Dense2Pattern;
            else if (value == "Dense3Pattern")
                brushStyle = Qt::Dense3Pattern;
            else if (value == "Dense4Pattern")
                brushStyle = Qt::Dense4Pattern;
            else if (value == "Dense5Pattern")
                brushStyle = Qt::Dense5Pattern;
            else if (value == "Dense6Pattern")
                brushStyle = Qt::Dense6Pattern;
            else if (value == "HorPattern")
                brushStyle = Qt::HorPattern;
            else if (value == "VerPattern")
                brushStyle = Qt::VerPattern;
            else if (value == "CrossPattern")
                brushStyle = Qt::CrossPattern;
            else if (value == "BDiagPattern")
                brushStyle = Qt::BDiagPattern;
            else if (value == "FDiagPattern")
                brushStyle = Qt::FDiagPattern;
            else if (value == "DiagCrossPattern")
                brushStyle = Qt::DiagCrossPattern;
            else
            {
                out << "invalid value for arg '" << arg << "'\n";
                return 0;
            }
        }
        else if (optionsOK && arg.startsWith("--brushColor="))
            brushColor.setNamedColor(arg.remove(0, 13));
        else if (optionsOK && arg.startsWith("--topMargin="))
        {
            bool isInt;
            margins = true;
            QString argCopy(arg);
            topMargin = arg.remove(0, 12).toInt(&isInt);
            if (!isInt || topMargin < 0)
            {
                out << "value for arg '" << argCopy << "' must be a positive int.\n";
                return 0;
            }
        }
        else if (optionsOK && arg.startsWith("--leftMargin="))
        {
            bool isInt;
            margins = true;
            QString argCopy(arg);
            leftMargin = arg.remove(0, 13).toInt(&isInt);
            if (!isInt || leftMargin < 0)
            {
                out << "value for arg '" << argCopy << "' must be a positive int.\n";
                return 0;
            }
        }
        else if (optionsOK && arg.startsWith("--rightMargin="))
        {
            bool isInt;
            margins = true;
            QString argCopy(arg);
            rightMargin = arg.remove(0, 14).toInt(&isInt);
            if (!isInt || rightMargin < 0)
            {
                out << "value for arg '" << argCopy << "' must be a positive int.\n";
                return 0;
            }
        }
        else if (optionsOK && arg.startsWith("--bottomMargin="))
        {
            bool isInt;
            margins = true;
            QString argCopy(arg);
            bottomMargin = arg.remove(0, 15).toInt(&isInt);
            if (!isInt || bottomMargin < 0)
            {
                out << "value for arg '" << argCopy << "' must be a positive int.\n";
                return 0;
            }
        }
        else if (optionsOK && (arg == "--help" || arg == "-h")) {
            out << "usage: diffpdf [options] [file1.pdf [file2.pdf]]\n\n"
                "A GUI program that compares two PDF files and shows "
                "their differences.\n"
                "\nThe files are optional and are normally set "
                "through the user interface.\n\n"
                "options:\n"
                "--help                   -h    show this usage text and terminate "
                "(run the program without this option and press F1 for "
                "online help)\n"
                "--visual                 -a    set the initial comparison mode to "
                "Visual\n"
                "--characters             -c    set the initial comparison mode to "
                "Characters\n"
                "--words                  -w    set the initial comparison mode to "
                "Words\n"
                "--output=<path>                set the output file path for side-"
                "by-side diffs (printSeparate must not be set)\n"
                "--printSeparate          -s    print the diff for each file "
                "in a separate file. Printed to <orig_path>.diff.pdf\n"
                "--pageRangeDoc1=<pages>        perform the diff for this "
                "page range. The format of <pages> is a list of page "
                "ranges, like 1-20 or 1-3,5,7-9 or even 1,5,9. When "
                "the --pageRangeDoc2 arg is also supplied, the pages "
                "from the two page ranges are matched up and diffed "
                "1:1\n"
                "--pageRangeDoc2=<pages>        see --pageRangeDoc1\n"
                "--compositionMode=<mode>       how the diff is presented. "
                "By default, diffs are highlighted, but the two docs "
                "can also be composed. Valid values are Difference, "
                "Exclusion, SourceXorDestination, and NotSourceXorDesti"
                "nation\n"
                "--combineHighlight       -H    coalesce adjacent highlighted "
                "words\n"
                "--overlap=<int>                affects what it means for two words"
                " to be 'adjacent'. Default 5\n"
                "--squareSize=<int>             granularity of highlighting. Recomm"
                "ended to be >=2. Default 5\n"
                "--zoom=<int>                   affects resolution of output. Recommend"
                "ed to be between 1 and 8. Default 2\n"
                "--opacity=<int>                affects opacity of highlights. Betw"
                "een 1 and 100. Default 50\n"
                "--penStyle=<Qt::PenStyle>      affects the outline of the "
                "highlights. Default NoPen\n"
                "--penColor=<color>             color of the outline of the "
                "highlights. Default tomato\n"
                "--brushStyle=<Qt::BrushStyle>  affects the fill of the"
                " highlights. Default SolidPattern\n"
                "--brushColor=<color>           color of the fill of the "
                "highlights. Default tomato\n"
                "--topMargin=<int>              the size of the top margin to be "
                "excluded from diffs\n"
                "--leftMargin=<int>             the size of the left margin to be "
                "excluded from diffs\n"
                "--rightMargin=<int>            the size of the right margin to "
                "be excluded from diffs\n"
                "--bottomMargin=<int>           the size of the bottom margin "
                "to be excluded from diffs\n"
                "coordinates in y, x order\n";
                // TODO(bhuh): Re-enable debug modes
            return 0;
        }
        else if (optionsOK && (arg == "--debug" || arg == "--debug=1" ||
                               arg == "--debug1"))
            ; // basic debug mode currently does nothing (did show zones)
        else if (optionsOK && (arg == "--debug=2" || arg == "--debug2"))
            debug = DebugShowTexts;
        else if (optionsOK && (arg == "--debug=3" || arg == "--debug3"))
            debug = DebugShowTextsAndYX;
        else if (optionsOK && arg == "--")
            optionsOK = false;
        else if (filename1.isEmpty() && arg.toLower().endsWith(".pdf"))
        {
            filename1 = arg;
            pdf1 = pdfLoader.getPdf(filename1);
            if (!pdf1)
            {
                out << "invalid pdf file '" << filename1 << "'\n";
                return 0;
            }
        }
        else if (filename2.isEmpty() && arg.toLower().endsWith(".pdf"))
        {
            filename2 = arg;
            pdf2 = pdfLoader.getPdf(filename2);
            if (!pdf2)
            {
                out << "invalid pdf file '" << filename2 << "'\n";
                return 0;
            }
        }
        else
            out << "unrecognized argument '" << arg << "'\n";
    }

    if (comparisonMode != CompareVisual && useComposition)
    {
        out << "compositionMode can only be used with --visual argument\n";
        return 0;
    }
    if (!printSeparate && saveFilename.isEmpty())
    {
        out << "Must supply an output file if not printing two separate diffs\n";
        return 0;
    }
    if (printSeparate && !saveFilename.isEmpty())
    {
        out << "Cannot supply '--printSeparate' argument together with '--output' argument\n";
        return 0;
    }

    // TODO(bhuh): do stricter validation of the other params as well
    
    // flush any warnings to stdout
    out.flush();

    Differ differ(
        debug,
        comparisonMode,
        filename1,
        filename2,
        pdf1,
        pdf2,
        saveFilename,
        printSeparate,
        pageRangeDoc1,
        pageRangeDoc2,
        useComposition,
        compositionMode,
        combineHighlightedWords,
        overlap,
        squareSize,
        zoom,
        opacity,
        penStyle,
        penColor,
        brushStyle,
        brushColor,
        margins,
        topMargin,
        leftMargin,
        rightMargin,
        bottomMargin);
    differ.diffToPdfs();
    return 0;
}

