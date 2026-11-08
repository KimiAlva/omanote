#include "editor.h"
#include <QPainter>
#include <QApplication>
#include <QTextLayout>
#include <QTextBlock>
#include <QFont>
#include <QFontMetrics>

Editor::Editor(QWidget *parent) : QPlainTextEdit(parent) {
    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(document());
    m_highlighter->setTheme(m_repo.themeForPalette(qApp->palette()));
    m_currentLineHighlight = QColor(255,255,255,25);

    setWordWrapMode(QTextOption::NoWrap);
    setTabStopDistance(m_tabSize * QFontMetrics(font()).horizontalAdvance(' '));

    connect(this, &QPlainTextEdit::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::updateBracketMatch);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    updateBracketMatch();
}

void Editor::setFontFamily(const QString &family, int size) {
    QFont f(family, size);
    setFont(f);
    setTabStopDistance(4 * QFontMetrics(f).horizontalAdvance(' '));
    updateLineNumberAreaWidth(0);
}

void Editor::setCurrentLineHighlight(const QColor &c) {
    m_currentLineHighlight = c;
    highlightCurrentLine();
}

void Editor::setTabSize(int spaces) {
    if (spaces < 1) spaces = 1;
    m_tabSize = spaces;
    setTabStopDistance(m_tabSize * QFontMetrics(font()).horizontalAdvance(' '));
    updateLineNumberAreaWidth(0);
}

void Editor::setInsertSpaces(bool on) { m_insertSpaces = on; }
void Editor::setAutoIndent(bool on) { m_autoIndent = on; }
void Editor::setBracketMatching(bool on) { m_bracketMatching = on; updateBracketMatch(); }
void Editor::setRightMargin(bool on, int column) {
    m_rightMargin = on;
    if (column > 0) m_rightMarginColumn = column;
    viewport()->update();
}
void Editor::setDrawWhitespace(bool on) { m_drawWhitespace = on; viewport()->update(); }
void Editor::setAutoBracketCompletion(bool on) { m_autoBracket = on; }
void Editor::setHighlightColors(const QColor &keyword, const QColor &type, const QColor &string,
                               const QColor &comment, const QColor &number, const QColor &function,
                               const QColor &preprocessor) {
    // KSyntaxHighlighting uses theme; colors derived from palette/theme handled in refreshHighlightTheme.
    Q_UNUSED(keyword); Q_UNUSED(type); Q_UNUSED(string); Q_UNUSED(comment);
    Q_UNUSED(number); Q_UNUSED(function); Q_UNUSED(preprocessor);
}

void Editor::refreshHighlightTheme() {
    if (m_highlighter) m_highlighter->setTheme(m_repo.themeForPalette(qApp->palette()));
}

void Editor::setSyntaxHighlighting(bool on) {
    m_syntaxHighlighting = on;
    if (!m_highlighter) return;
    if (on) {
        if (!m_highlightPath.isEmpty()) setHighlightDefinition(m_highlightPath);
        else setLanguage(m_lang);
    } else {
        m_highlighter->setDefinition(KSyntaxHighlighting::Definition());
    }
}
void Editor::setSmartHomeEnd(int mode) { m_smartHomeEnd = mode; }

void Editor::setSearchHighlights(const QList<QTextEdit::ExtraSelection> &sel) {
    m_searchSelections = sel;
    highlightCurrentLine();
}

void Editor::setLineNumbersVisible(bool on) {
    m_lineNumbersVisible = on;
    if (on) {
        m_lineNumberArea->show();
        updateLineNumberAreaWidth(0);
    } else {
        m_lineNumberArea->hide();
        setViewportMargins(0, 0, 0, 0);
    }
    viewport()->update();
}

void Editor::setCursorPosition(int pos) {
    QTextCursor c = textCursor();
    c.setPosition(qBound(0, pos, document()->characterCount() - 1));
    setTextCursor(c);
}

void Editor::setLanguage(const QString &lang) {
    m_lang = lang;
    setHighlightDefinition(QString("file.") + lang);
}

void Editor::setHighlightDefinition(const QString &fileName) {
    m_highlightPath = fileName;
    KSyntaxHighlighting::Definition def = m_repo.definitionForFileName(fileName);
    if (!def.isValid()) {
        def = m_repo.definitionForName(fileName);
    }
    if (def.isValid()) m_highlighter->setDefinition(def);
}

QString Editor::highlightLanguageName() const {
    if (m_highlighter) {
        KSyntaxHighlighting::Definition def = m_highlighter->definition();
        if (def.isValid()) return def.name();
    }
    return m_lang;
}

int Editor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; digits++; }
    int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 1);
    return space;
}

void Editor::updateLineNumberAreaWidth(int /*newBlockCount*/) {
    if (!m_lineNumbersVisible) { setViewportMargins(0,0,0,0); return; }
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Editor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) m_lineNumberArea->scroll(0, dy);
    else m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void Editor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::paintEvent(QPaintEvent *e) {
    QPlainTextEdit::paintEvent(e);
    QPainter p(viewport());
    if (m_drawWhitespace) {
        p.setPen(QColor(108,112,134,120));
        QTextBlock block = firstVisibleBlock();
        while (block.isValid()) {
            QRectF br = blockBoundingGeometry(block).translated(contentOffset());
            if (br.top() > e->rect().bottom()) break;
            if (br.bottom() >= e->rect().top()) {
                QTextLayout *layout = block.layout();
                if (layout) {
                    QString text = block.text();
                    for (int li = 0; li < layout->lineCount(); ++li) {
                        QTextLine line = layout->lineAt(li);
                        int start = line.textStart();
                        int len = line.textLength();
                        for (int j = 0; j < len; ++j) {
                            QChar ch = (start + j < text.size()) ? text[start + j] : QChar();
                            if (ch == ' ' || ch == '\t') {
                                qreal x = line.cursorToX(j) + contentOffset().x();
                                qreal y = br.top() + line.y() + line.ascent();
                                if (ch == ' ')
                                    p.drawPoint(QPointF(x + fontMetrics().averageCharWidth() / 2.0, y - fontMetrics().ascent() / 2.0));
                                else
                                    p.drawText(QPointF(x + 2, y - 2), QString(QChar(0x2192)));
                            }
                        }
                    }
                }
            }
            block = block.next();
        }
    }
    if (m_rightMargin) {
        int x = fontMetrics().horizontalAdvance(' ') * m_rightMarginColumn + 4;
        p.setPen(QColor(108,112,134,120));
        p.drawLine(x, 0, x, viewport()->height());
    }
}

void Editor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extra;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(m_currentLineHighlight);
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        extra.append(sel);
    }
    if (m_bracketMatching && m_bracketMatchPos >= 0) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(255,255,0,60));
        sel.format.setForeground(QColor("#f9e2af"));
        QTextCursor c(document());
        c.setPosition(m_bracketMatchPos);
        c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        sel.cursor = c;
        extra.append(sel);
    }
    if (m_searchSelections.size() > 0) extra.append(m_searchSelections);
    setExtraSelections(extra);
}

void Editor::updateBracketMatch() {
    m_bracketMatchPos = -1;
    if (!m_bracketMatching) { highlightCurrentLine(); return; }
    QTextCursor c = textCursor();
    int pos = c.position();
    QString text = document()->toPlainText();
    // determine bracket at cursor or before
    QChar ch;
    int idx = -1;
    if (pos < text.size()) {
        ch = text[pos];
        if (QString("{}[]()").contains(ch)) idx = pos;
    }
    if (idx < 0 && pos > 0) {
        ch = text[pos-1];
        if (QString("{}[]()").contains(ch)) idx = pos-1;
    }
    if (idx < 0) { highlightCurrentLine(); return; }
    bool isOpen = QString("({[").contains(ch);
    QChar counterpart;
    if (isOpen) {
        if (ch == '(') counterpart = ')';
        else if (ch == '[') counterpart = ']';
        else counterpart = '}';
    } else {
        if (ch == ')') counterpart = '(';
        else if (ch == ']') counterpart = '[';
        else counterpart = '{';
    }
    int depth = 0;
    if (isOpen) {
        for (int i = idx; i < text.size(); i++) {
            if (text[i] == ch) depth++;
            else if (text[i] == counterpart) { depth--; if (depth == 0) { m_bracketMatchPos = i; break; } }
        }
    } else {
        for (int i = idx; i >= 0; i--) {
            if (text[i] == ch) depth++;
            else if (text[i] == counterpart) { depth--; if (depth == 0) { m_bracketMatchPos = i; break; } }
        }
    }
    highlightCurrentLine();
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0,0,0,0));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#6c7086"));
            painter.drawText(0, top, m_lineNumberArea->width() - 4, fontMetrics().height(),
                            Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        blockNumber++;
    }
}

void Editor::keyPressEvent(QKeyEvent *e) {
    // Auto bracket completion
    if (m_autoBracket && e->text().size() == 1 && !e->modifiers()) {
        QChar ch = e->text().at(0);
        QChar closing;
        if (ch == '(') closing = ')';
        else if (ch == '[') closing = ']';
        else if (ch == '{') closing = '}';
        else if (ch == '"') closing = '"';
        else if (ch == '\'') closing = '\'';
        if (!closing.isNull()) {
            // skip over if next char same
            QTextCursor c = textCursor();
            if (c.hasSelection()) {
                // wrap selection
                QString sel = c.selectedText();
                c.insertText(QString(ch) + sel + QString(closing));
                // move cursor before closing
                int pos = c.position() - sel.size() - 1;
                c.setPosition(pos);
                setTextCursor(c);
                return;
            }
            if (c.position() < document()->characterCount() - 1 && document()->characterAt(c.position()) == closing) {
                // just move
                c.movePosition(QTextCursor::NextCharacter);
                setTextCursor(c);
                return;
            }
            c.insertText(QString(ch) + QString(closing));
            c.movePosition(QTextCursor::PreviousCharacter);
            setTextCursor(c);
            return;
        }
        // skip over closing bracket
        if (QString(")]}\"'").contains(ch)) {
            QTextCursor c = textCursor();
            if (!c.hasSelection() && c.position() < document()->characterCount() - 1 && document()->characterAt(c.position()) == ch) {
                c.movePosition(QTextCursor::NextCharacter);
                setTextCursor(c);
                return;
            }
        }
    }
    // Tab
    if (e->key() == Qt::Key_Tab && !e->modifiers()) {
        if (m_insertSpaces)
            insertPlainText(QString(m_tabSize, ' '));
        else
            insertPlainText("\t");
        return;
    }
    // Smart Home/End
    if (m_smartHomeEnd != 0 && (e->key() == Qt::Key_Home || e->key() == Qt::Key_End)) {
        QTextCursor c = textCursor();
        if (e->key() == Qt::Key_Home) {
            QString line = c.block().text();
            int firstNonSpace = 0;
            while (firstNonSpace < line.size() && (line[firstNonSpace] == ' ' || line[firstNonSpace] == '\t')) firstNonSpace++;
            int col = c.columnNumber();
            bool atTextStart = (col == firstNonSpace);
            if (m_smartHomeEnd == 3 || (m_smartHomeEnd == 2 && !atTextStart) || (m_smartHomeEnd == 1 && atTextStart)) {
                c.movePosition(QTextCursor::StartOfBlock);
                for (int i = 0; i < firstNonSpace; i++) c.movePosition(QTextCursor::NextCharacter);
                setTextCursor(c);
                return;
            }
        } else {
            QString line = c.block().text();
            int lastNonSpace = line.size();
            while (lastNonSpace > 0 && (line[lastNonSpace-1] == ' ' || line[lastNonSpace-1] == '\t')) lastNonSpace--;
            int col = c.columnNumber();
            bool atTextEnd = (col == lastNonSpace);
            if (m_smartHomeEnd == 3 || (m_smartHomeEnd == 2 && !atTextEnd) || (m_smartHomeEnd == 1 && atTextEnd)) {
                c.movePosition(QTextCursor::StartOfBlock);
                for (int i = 0; i < lastNonSpace; i++) c.movePosition(QTextCursor::NextCharacter);
                setTextCursor(c);
                return;
            }
        }
    }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QTextCursor c = textCursor();
        QString line = c.block().text();
        int indent = 0;
        if (m_autoIndent) {
            while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t')) indent++;
            QString trimmed = line.trimmed();
            if (trimmed.endsWith('{') || trimmed.endsWith(':') || trimmed.endsWith('(')) indent += m_tabSize;
        }
        QPlainTextEdit::keyPressEvent(e);
        if (indent > 0) insertPlainText(QString(indent, ' '));
        return;
    }
    QPlainTextEdit::keyPressEvent(e);
}
