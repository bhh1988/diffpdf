#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP
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

#include "generic.hpp"
#include "saveform.hpp"
#include <poppler-qt4.h>
#include <QBrush>
#include <QList>
#include <QPainter>
#include <QPen>
#include <QVector>

// TODO(bhuh): find a better home for this class
class PdfLoader
{
public:
    PdfLoader();
    PdfDocument getPdf(const QString &filename);
};

class Differ
{
public:
    Differ(
        const Debug debug,
        const InitialComparisonMode comparisonMode,
        const QString &filename1,
        const QString &filename2,
        const PdfDocument &pdf1,
        const PdfDocument &pdf2,
        const QString saveFilename,
        const bool printSeparate,
        const QString pageRangeDoc1,
        const QString pageRangeDoc2,
        const bool useComposition,
        const QPainter::CompositionMode compositionMode,
        const bool combineHighlightedWords,
        const int overlap,
        const int squareSize,
        const int zoom,
        const int opacity,
        const Qt::PenStyle penStyle,
        QColor penColor,
        const Qt::BrushStyle brushStyle,
        QColor brushColor,
        const bool margins,
        const int topMargin,
        const int leftMargin,
        const int rightMargin,
        const int bottomMargin);

    void diffToPdfs();
    void diffToImages();
protected:

private:
    enum Difference {NoDifference, TextualDifference, VisualDifference};

    void generateDiffStatuses();
    QList<int> getPageList(int which, PdfDocument pdf);
    Difference getTheDifference(PdfPage page1, PdfPage page2);
    void paintOnImage(const QPainterPath &path, QImage *image);
    const QPair<QPixmap, QPixmap> populatePixmaps(const PdfPage &page1,
            const PdfPage &page2, bool hasVisualDifference);
    void computeTextHighlights(QPainterPath *highlighted1,
            QPainterPath *highlighted2, const PdfPage &page1,
            const PdfPage &page2, const int DPI);
    void computeVisualHighlights(QPainterPath *highlighted1,
        QPainterPath *highlighted2, const QImage &plainImage1,
        const QImage &plainImage2);
    void addHighlighting(QRectF *bigRect, QPainterPath *highlighted,
            const QRectF wordOrCharRect, const int DPI);
    void compareAndSaveAsPdfs(const int start, const int end,
            const SavePages savePages);
    bool compareAndPaint(QPainter *painter, const int index,
            const QRect &leftRect, const QRect &rightRect,
            const SavePages savePages);
    void compareAndSaveAsImages(const int start, const int end,
            const SavePages savePages);
    void computeImageOffsets(const QSize &size, int *x, int *y,
            int *width, int *height);
    QRectF pointRectForMargins(const QSize &size);
    QRect pixelRectForMargins(const QSize &size);

    QBrush brush;
    QPen pen;
    QVector<QVariant> diffStatuses;
    PdfDocument pdf1;
    PdfDocument pdf2;

    // ====================================
    // CONFIGURABLE ARGUMENTS
    // ====================================
    Debug debug;
    const InitialComparisonMode comparisonMode;
    const QString filename1;
    const QString filename2;
    const QString saveFilename;
    const bool printSeparate;
    const QString pageRangeDoc1;
    const QString pageRangeDoc2;
    const bool useComposition;
    const QPainter::CompositionMode compositionMode;
    const bool combineHighlightedWords;
    const int overlap; // This together with zoom (for some reason) affects
                       // whether adjacent highlighted words get combined

    // visual preferences
    const int squareSize; // GUI limits >=2. Affects performance
    const int zoom; // GUI limits 1-8. Affects performance ~O(n^2)
    const int opacity; // 0-100%
    const Qt::PenStyle penStyle; // see optionsform.cpp for available options
    const QColor penColor;
    const Qt::BrushStyle brushStyle; // see optionsform.cpp
    const QColor brushColor;

    // margins (to exclude when doing the diff)
    const bool margins;
    const int topMargin;
    const int leftMargin;
    const int rightMargin;
    const int bottomMargin;

    // ====================================
    // END CONFIGURABLE ARGUMENTS
    // ====================================
};

#endif // MAINWINDOW_HPP

