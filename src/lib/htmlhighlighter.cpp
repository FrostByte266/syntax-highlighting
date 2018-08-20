/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "htmlhighlighter.h"
#include "definition.h"
#include "format.h"
#include "state.h"
#include "theme.h"
#include "ksyntaxhighlighting_logging.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace KSyntaxHighlighting;

class KSyntaxHighlighting::HtmlHighlighterPrivate
{
public:
    std::unique_ptr<QTextStream> out;
    std::unique_ptr<QFile> file;
    QString currentLine;
};

HtmlHighlighter::HtmlHighlighter()
    : d(new HtmlHighlighterPrivate())
{
}

HtmlHighlighter::~HtmlHighlighter()
{
}

void HtmlHighlighter::setOutputFile(const QString& fileName)
{
    d->file.reset(new QFile(fileName));
    if (!d->file->open(QFile::WriteOnly | QFile::Truncate)) {
        qCWarning(Log) << "Failed to open output file" << fileName << ":" << d->file->errorString();
        return;
    }
    d->out.reset(new QTextStream(d->file.get()));
    d->out->setCodec("UTF-8");
}

void HtmlHighlighter::setOutputFile(FILE *fileHandle)
{
    d->out.reset(new QTextStream(fileHandle, QIODevice::WriteOnly));
    d->out->setCodec("UTF-8");
}

void HtmlHighlighter::highlightFile(const QString& fileName)
{
    if (!d->out) {
        qCWarning(Log) << "No output stream defined!";
        return;
    }

    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(Log) << "Failed to open input file" << fileName << ":" << f.errorString();
        return;
    }

    State state;
    *d->out << "<!DOCTYPE html>\n";
    *d->out << "<html><head>\n";
    *d->out << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n";
    QFileInfo fi(fileName);
    *d->out << "<title>" << fi.fileName() << "</title>\n";
    *d->out << "<meta name=\"generator\" content=\"KF5::SyntaxHighlighting (" << definition().name() << ")\"/>\n";
    *d->out << "</head><body";
    if (theme().textColor(Theme::Normal))
        *d->out << " style=\"color:" << QColor(theme().textColor(Theme::Normal)).name() << "\"";
    *d->out << "><pre>\n";

    QTextStream in(&f);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        d->currentLine = in.readLine();
        state = highlightLine(d->currentLine, state);
        *d->out << "\n";
    }

    *d->out << "</pre></body></html>\n";
    d->out->flush();

    d->out.reset();
    d->file.reset();
}

void HtmlHighlighter::applyFormat(int offset, int length, const Format& format)
{
    if (length == 0)
        return;

    if (!format.isDefaultTextStyle(theme())) {
        *d->out << "<span style=\"";
        if (format.hasTextColor(theme()))
            *d->out << "color:" << format.textColor(theme()).name() << ";";
        if (format.hasBackgroundColor(theme()))
            *d->out << "background-color:" << format.backgroundColor(theme()).name() << ";";
        if (format.isBold(theme()))
            *d->out << "font-weight:bold;";
        if (format.isItalic(theme()))
            *d->out << "font-style:italic;";
        if (format.isUnderline(theme()))
            *d->out << "text-decoration:underline;";
        if (format.isStrikeThrough(theme()))
            *d->out << "text-decoration:line-through;";
        *d->out << "\">";
    }

    *d->out << d->currentLine.mid(offset, length).toHtmlEscaped();

    if (!format.isDefaultTextStyle(theme()))
        *d->out << "</span>";
}
