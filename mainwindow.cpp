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
#include "mainwindow.hpp"
#include "sequence_matcher.hpp"
#include "textitem.hpp"
#ifdef DEBUG
#include <QtDebug>
#endif
#include <QPrinter>

Differ::Differ(
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
    const int bottomMargin) : pdf1(pdf1), pdf2(pdf2), debug(debug),
        comparisonMode(comparisonMode), filename1(filename1),
        filename2(filename2), saveFilename(saveFilename),
        printSeparate(printSeparate), pageRangeDoc1(pageRangeDoc1),
        pageRangeDoc2(pageRangeDoc2), useComposition(useComposition),
        compositionMode(compositionMode),
        combineHighlightedWords(combineHighlightedWords), overlap(overlap),
        squareSize(squareSize), zoom(zoom), opacity(opacity),
        penStyle(penStyle), penColor(penColor), brushStyle(brushStyle),
        brushColor(brushColor), margins(margins), topMargin(topMargin),
        leftMargin(leftMargin), rightMargin(rightMargin),
        bottomMargin(bottomMargin)
{
    const qreal Alpha = opacity / 100.0;
    penColor.setAlphaF(Alpha);
    brushColor.setAlphaF(Alpha);

    pen.setColor(penColor);
    brush.setColor(brushColor);

    pen.setStyle(penStyle);
    brush.setStyle(brushStyle);
}

const QPair<QPixmap, QPixmap> Differ::populatePixmaps(
        const PdfPage &page1, const PdfPage &page2,
        bool hasVisualDifference)
{
    QPixmap pixmap1;
    QPixmap pixmap2;
    const int DPI = POINTS_PER_INCH * zoom;
    const bool compareText = comparisonMode !=
                             CompareVisual;
    QImage plainImage1;
    QImage plainImage2;
    if (hasVisualDifference || !compareText) {
        plainImage1 = page1->renderToImage(DPI, DPI);
        plainImage2 = page2->renderToImage(DPI, DPI);
    }
    pdf1->setRenderHint(Poppler::Document::Antialiasing);
    pdf1->setRenderHint(Poppler::Document::TextAntialiasing);
    pdf2->setRenderHint(Poppler::Document::Antialiasing);
    pdf2->setRenderHint(Poppler::Document::TextAntialiasing);
    QImage image1 = page1->renderToImage(DPI, DPI);
    QImage image2 = page2->renderToImage(DPI, DPI);

    if (comparisonMode != CompareVisual || !useComposition)
    {
        QPainterPath highlighted1;
        QPainterPath highlighted2;
        if (hasVisualDifference || !compareText)
            computeVisualHighlights(&highlighted1, &highlighted2,
                    plainImage1, plainImage2);
        else
            computeTextHighlights(&highlighted1, &highlighted2, page1,
                    page2, DPI);
        if (!highlighted1.isEmpty())
            paintOnImage(highlighted1, &image1);
        if (!highlighted2.isEmpty())
            paintOnImage(highlighted2, &image2);
        if (highlighted1.isEmpty() && highlighted2.isEmpty()) {
            ;
        }
        pixmap1 = QPixmap::fromImage(image1);
        pixmap2 = QPixmap::fromImage(image2);
    } else {
        pixmap1 = QPixmap::fromImage(image1);
        QImage composed(image1.size(), image1.format());
        QPainter painter(&composed);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(composed.rect(), Qt::transparent);
        painter.setCompositionMode(
                QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, image1);
        painter.setCompositionMode(compositionMode);
        painter.drawImage(0, 0, image2);
        painter.setCompositionMode(
                QPainter::CompositionMode_DestinationOver);
        painter.fillRect(composed.rect(), Qt::white);
        painter.end();
        pixmap2 = QPixmap::fromImage(composed);
    }
    return qMakePair(pixmap1, pixmap2);
}

void Differ::computeTextHighlights(QPainterPath *highlighted1,
        QPainterPath *highlighted2, const PdfPage &page1,
        const PdfPage &page2, const int DPI)
{
    const bool ComparingWords = comparisonMode ==
                                CompareWords;
    QRectF rect1;
    QRectF rect2;
    QRectF rect;
    if (margins)
        rect = pointRectForMargins(page1->pageSize());
    const TextBoxList list1 = getTextBoxes(page1, rect);
    const TextBoxList list2 = getTextBoxes(page2, rect);
    TextItems items1 = ComparingWords ? getWords(list1)
                                      : getCharacters(list1);
    TextItems items2 = ComparingWords ? getWords(list2)
                                      : getCharacters(list2);
    const int ToleranceY = 10;
    if (debug >= DebugShowTexts) {
        const bool Yx = debug == DebugShowTextsAndYX;
        items1.debug(1, ToleranceY, ComparingWords, Yx);
        items2.debug(2, ToleranceY, ComparingWords, Yx);
    }

    SequenceMatcher matcher(items1.texts(), items2.texts());
    RangesPair rangesPair = computeRanges(&matcher);
    rangesPair = invertRanges(rangesPair.first, items1.count(),
                              rangesPair.second, items2.count());

    foreach (int index, rangesPair.first)
        addHighlighting(&rect1, highlighted1, items1.at(index).rect, DPI);
    if (!rect1.isNull() && !rangesPair.first.isEmpty())
        highlighted1->addRect(rect1);
    foreach (int index, rangesPair.second)
        addHighlighting(&rect2, highlighted2, items2.at(index).rect, DPI);
    if (!rect2.isNull() && !rangesPair.second.isEmpty())
        highlighted2->addRect(rect2);
}

void Differ::addHighlighting(QRectF *bigRect,
        QPainterPath *highlighted, const QRectF wordOrCharRect,
        const int DPI)
{
    QRectF rect = wordOrCharRect;
    scaleRect(DPI, &rect);
    if (combineHighlightedWords &&
        rect.adjusted(-overlap, -overlap, overlap, overlap)
            .intersects(*bigRect))
    {
        *bigRect = bigRect->united(rect);
    }
    else {
        highlighted->addRect(*bigRect);
        *bigRect = rect;
    }
}

void Differ::computeVisualHighlights(QPainterPath *highlighted1,
        QPainterPath *highlighted2, const QImage &plainImage1,
        const QImage &plainImage2)
{
    QRect box;
    if (margins)
        box = pixelRectForMargins(plainImage1.size());
    QRect target;
    for (int x = 0; x < plainImage1.width(); x += squareSize) {
        for (int y = 0; y < plainImage1.height(); y += squareSize) {
            const QRect rect(x, y, squareSize, squareSize);
            if (!box.isEmpty() && !box.contains(rect))
                continue;
            QImage temp1 = plainImage1.copy(rect);
            QImage temp2 = plainImage2.copy(rect);
            if (temp1 != temp2) {
                if (rect.adjusted(-1, -1, 1, 1).intersects(target))
                    target = target.united(rect);
                else {
                    highlighted1->addRect(target);
                    highlighted2->addRect(target);
                    target = rect;
                }
            }
        }
    }
    if (!target.isNull()) {
        highlighted1->addRect(target);
        highlighted2->addRect(target);
    }
}

QRect Differ::pixelRectForMargins(const QSize &size)
{
    const int DPI = POINTS_PER_INCH * zoom;
    int top = pixelOffsetForPointValue(DPI, topMargin);
    int left = pixelOffsetForPointValue(DPI, leftMargin);
    int right = pixelOffsetForPointValue(DPI, rightMargin);
    int bottom = pixelOffsetForPointValue(DPI,
            bottomMargin);
    return QRect(QPoint(left, top),
                 QPoint(size.width() - right, size.height() - bottom));
}

void Differ::paintOnImage(const QPainterPath &path, QImage *image)
{
    QPainter painter(image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(pen);
    painter.setBrush(brush);

    QRectF rect = path.boundingRect();
    if (rect.width() < squareSize && rect.height() < squareSize) {
        rect.setHeight(squareSize);
        rect.setWidth(squareSize);
        painter.drawRect(rect);
    }
    else {
        QPainterPath path_(path);
        path_.setFillRule(Qt::WindingFill);
        painter.drawPath(path_);
    }
    painter.end();
}

QList<int> Differ::getPageList(int which, PdfDocument pdf)
{
    // Poppler has 0-based page numbers; the UI has 1-based page numbers
    QString page_string = (which == 1 ? pageRangeDoc1 : pageRangeDoc2);
    bool error = false;
    QList<int> pages;
    page_string = page_string.replace(QRegExp("\\s+"), "");
    QStringList page_list = page_string.split(",");
    bool ok;
    if (page_string.isEmpty())
    {
        for (int page = 0; page < pdf->numPages(); ++page)
            pages.append(page);
    }
    else
    {
        foreach (const QString &page, page_list) {
            int hyphen = page.indexOf("-");
            if (hyphen > -1) {
                int p1 = page.left(hyphen).toInt(&ok);
                if (!ok || p1 < 1) {
                    error = true;
                    break;
                }
                int p2 = page.mid(hyphen + 1).toInt(&ok);
                if (!ok || p2 < 1 || p2 < p1) {
                    error = true;
                    break;
                }
                if (p1 == p2)
                    pages.append(p1 - 1);
                else {
                    for (int p = p1; p <= p2; ++p) {
                        if (p > pdf->numPages())
                            break;
                        pages.append(p - 1);
                    }
                }
            }
            else {
                int p = page.toInt(&ok);
                if (ok && p > 0 && p <= pdf->numPages())
                    pages.append(p - 1);
                else {
                    error = true;
                    break;
                }
            }
        }
    }
    if (error) {
        pages.clear();
        /**
         *
         * TODO(bhuh): Throw here
        writeError(tr("Failed to understand page range '%1'.")
                   .arg(pagesEdit->text()));
        for (int page = 0; page < pdf->numPages(); ++page)
            pages.append(page);
         */
    }
    return pages;
}

void Differ::diffToPdfs()
{
    generateDiffStatuses(); // populates diffStatuses
    int start = 0;
    int end = diffStatuses.size();

    if (printSeparate)
    {
        // TODO(bhuh): there might be a lot of repeated work happening this way.
        // Investigate.
        compareAndSaveAsPdfs(start, end, SaveLeftPages);
        compareAndSaveAsPdfs(start, end, SaveRightPages);
    }
    else
    {
        compareAndSaveAsPdfs(start, end, SaveBothPages);
    }
}

void Differ::diffToImages()
{
    generateDiffStatuses(); // populates diffStatuses
    int start = 0;
    int end = diffStatuses.size();

    if (printSeparate)
    {
        // TODO(bhuh): there might be a lot of repeated work happening this way.
        // Investigate.
        compareAndSaveAsImages(start, end, SaveLeftPages);
        compareAndSaveAsImages(start, end, SaveRightPages);
    }
    else
    {
        compareAndSaveAsImages(start, end, SaveBothPages);
    }
}

// NOTE(bhuh): Are we doing too much work here? getTheDifference looks
// like it could be an expensive function, maybe too much work for just
// deciding whether we want to diff the two pages or not...
void Differ::generateDiffStatuses()
{
    QList<int> pages1 = getPageList(1, pdf1);
    QList<int> pages2 = getPageList(2, pdf2);
    while (!pages1.isEmpty() && !pages2.isEmpty()) {
        int p1 = pages1.takeFirst();
        PdfPage page1(pdf1->page(p1));
        if (!page1) {
        /**
         * TODO(bhuh): throw here
            writeError(tr("Failed to read page %1 from '%2'.")
                          .arg(p1 + 1).arg(filename1));
         */
            continue;
        }
        int p2 = pages2.takeFirst();
        PdfPage page2(pdf2->page(p2));
        if (!page2) {
        /**
         * TODO(bhuh): throw here
            writeError(tr("Failed to read page %1 from '%2'.")
                          .arg(p2 + 1).arg(filename2));
         */
            continue;
        }
        Difference difference = getTheDifference(page1, page2);
        if (difference != NoDifference) {
            QVariant v;
            v.setValue(PagePair(p1, p2, difference == VisualDifference));
            diffStatuses.push_back(v);
        }
    }
}

Differ::Difference Differ::getTheDifference(PdfPage page1, PdfPage page2)
{
    QRectF rect;
    if (margins)
        rect = pointRectForMargins(page1->pageSize());
    const TextBoxList list1 = getTextBoxes(page1, rect);
    const TextBoxList list2 = getTextBoxes(page2, rect);
    if (list1.count() != list2.count())
        return TextualDifference;
    for (int i = 0; i < list1.count(); ++i)
        if (list1[i]->text() != list2[i]->text())
            return TextualDifference;

    if (comparisonMode == CompareVisual) {
        int x = -1;
        int y = -1;
        int width = -1;
        int height = -1;
        if (margins)
            computeImageOffsets(page1->pageSize(), &x, &y, &width,
                    &height);
        QImage image1 = page1->renderToImage(POINTS_PER_INCH,
                POINTS_PER_INCH, x, y, width, height);
        QImage image2 = page2->renderToImage(POINTS_PER_INCH,
                POINTS_PER_INCH, x, y, width, height);
        if (image1 != image2)
            return VisualDifference;
    }
    return NoDifference;
}


QRectF Differ::pointRectForMargins(const QSize &size)
{
    return rectForMargins(size.width(), size.height(),
            topMargin, bottomMargin,
            leftMargin, rightMargin);
}


void Differ::computeImageOffsets(const QSize &size, int *x, int *y,
        int *width, int *height)
{
    const int DPI = POINTS_PER_INCH * zoom;
    *y = pixelOffsetForPointValue(DPI, topMargin);
    *x = pixelOffsetForPointValue(DPI, leftMargin);
    *width = pixelOffsetForPointValue(DPI, size.width() -
            (leftMargin + rightMargin));
    *height = pixelOffsetForPointValue(DPI, size.height() -
            (topMargin + bottomMargin));
}

void Differ::compareAndSaveAsImages(const int start, const int end,
        const SavePages savePages)
{
    PdfPage page1(pdf1->page(0));
    if (!page1)
        return;
    PdfPage page2(pdf2->page(0));
    if (!page2)
        return;
    // NOTE(bhuh): The following lines are a hack because it assumes all
    // page sizes are the same for both documents
    int width = 2 * (savePages == SaveBothPages
            ? page1->pageSize().width() + page2->pageSize().width()
            : page1->pageSize().width());
    // NOTE(bhuh): I'm not sure if I need this...
    //const int y = fontMetrics().lineSpacing();
    const int y = 0;
    const int height = (2 * page1->pageSize().height()) - y;
    const int gap = 0;
    const QRect rect(0, 0, width, height);
    if (savePages == SaveBothPages)
        width = (width / 2) - gap;
    const QRect leftRect(0, y, width, height);
    const QRect rightRect(width + gap, y, width, height);
    int count = 0;
    QString imageFilename = saveFilename;
    int i = imageFilename.lastIndexOf(".");
    if (i > -1)
        imageFilename.insert(i, "-%1");
    else
        imageFilename += "-%1.png";
    for (int index = start; index < end; ++index) {
        QImage image(rect.size(), QImage::Format_ARGB32);
        QPainter painter(&image);
        painter.fillRect(rect, Qt::white);
        if (!compareAndPaint(&painter, index, leftRect, rightRect, savePages))
            continue;
        QString filename = imageFilename;
        filename = filename.arg(++count);
        if (!image.save(filename))
            ;
            /**
             * TODO(bhuh): throw here
             */
    }
}

void Differ::compareAndSaveAsPdfs(const int start, const int end,
        const SavePages savePages)
{
    QPrinter printer(QPrinter::HighResolution);
    if (savePages == SaveBothPages)
    {
        printer.setOutputFileName(saveFilename);
    }
    else if (savePages == SaveLeftPages)
    {
        printer.setOutputFileName(filename1 + ".diff.pdf");
    }
    else // savePages == saveRightPages
    {
        printer.setOutputFileName(filename2 + ".diff.pdf");
    }

    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setColorMode(QPrinter::Color);
    printer.setFullPage(true);

    // NOTE(bhuh): The following lines are a hack because it assumes all
    // page sizes are the same for both documents
    PdfPage page(pdf1->page(0));
    const int gap = 0;
    QSizeF printPageSize;
    if (savePages == SaveBothPages)
    {
        QSizeF singlePage(page->pageSizeF());
        printPageSize = QSizeF(singlePage.width()*2 + gap, singlePage.height());
    }
    else
    {
        printPageSize = QSizeF(page->pageSizeF());
    }
    printer.setPaperSize(printPageSize, QPrinter::Point);

    QPainter painter(&printer);
    // NOTE(bhuh): I'm not sure if I need this...
    //const int y = painter.fontMetrics().lineSpacing();
    const int y = 0;
    const int height = painter.viewport().height() - y;
    int width = (painter.viewport().width()-gap) / 2;
    if (savePages != SaveBothPages)
        width = painter.viewport().width();
    const QRect leftRect(0, y, width, height);
    const QRect rightRect(width + gap, y, width, height);
    for (int index = start; index < end; ++index) {
        if (!compareAndPaint(&painter, index, leftRect, rightRect, savePages))
            continue;
        if (index + 1 < end)
            printer.newPage();
    }
}

bool Differ::compareAndPaint(QPainter *painter, const int index,
        const QRect &leftRect, const QRect &rightRect,
        const SavePages savePages)
{
    PagePair pair = diffStatuses[index].value<PagePair>();
    if (pair.isNull())
        return false;
    PdfPage page1(pdf1->page(pair.left));
    if (!page1)
        return false;
    PdfPage page2(pdf2->page(pair.right));
    if (!page2)
        return false;
    const QPair<QPixmap, QPixmap> pixmaps = populatePixmaps(page1,
        page2, pair.hasVisualDifference);
    if (savePages == SaveBothPages) {
        QRect rect = resizeRect(leftRect, pixmaps.first.size());
        painter->drawPixmap(rect, pixmaps.first);
        rect = resizeRect(rightRect, pixmaps.second.size());
        painter->drawPixmap(rect, pixmaps.second);
        painter->drawRect(rightRect.adjusted(2.5, 2.5, 2.5, 2.5));
    } else if (savePages == SaveLeftPages) {
        QRect rect = resizeRect(leftRect, pixmaps.first.size());
        painter->drawPixmap(rect, pixmaps.first);
    } else { // (savePages == SaveRightPages)
        QRect rect = resizeRect(leftRect, pixmaps.second.size());
        painter->drawPixmap(rect, pixmaps.second);
    }
    return true;
}

PdfLoader::PdfLoader() {}
PdfDocument PdfLoader::getPdf(const QString &filename)
{
    PdfDocument pdf(Poppler::Document::load(filename));
    if (!pdf)
    /**
     * TODO(bhuh): Throw here
        QMessageBox::warning(this, tr("DiffPDF — Error"),
                tr("Cannot load '%1'.").arg(filename));
     */
        ;
    else if (pdf->isLocked()) {
    /**
     * TODO(bhuh): Throw here
        QMessageBox::warning(this, tr("DiffPDF — Error"),
                tr("Cannot read a locked PDF ('%1').").arg(filename));
     */
        ;
#if QT_VERSION >= 0x040600
        pdf.clear();
#else
        pdf.reset();
#endif
    }
    return pdf;
}
