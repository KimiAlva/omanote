#pragma once
#include <QPlainTextEdit>
#include <QWidget>
#include <KSyntaxHighlighting/repository.h>
#include <KSyntaxHighlighting/syntaxhighlighter.h>
#include <KSyntaxHighlighting/theme.h>

class LineNumberArea;

class Editor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);

    void setLanguage(const QString &lang);
    void setHighlightDefinition(const QString &fileName);
    QString highlightLanguageName() const;
    QString language() const { return m_lang; }
    void setFontFamily(const QString &family, int size);
    void setCurrentLineHighlight(const QColor &c);

    // Xed-like options
    void setTabSize(int spaces);
    void setInsertSpaces(bool on);
    void setAutoIndent(bool on);
    void setBracketMatching(bool on);
    void setRightMargin(bool on, int column = 120);
    void setDrawWhitespace(bool on);
    void setAutoBracketCompletion(bool on);
    void setSyntaxHighlighting(bool on);
    void setHighlightColors(const QColor &keyword, const QColor &type, const QColor &string,
                            const QColor &comment, const QColor &number, const QColor &function,
                            const QColor &preprocessor);
    void refreshHighlightTheme();
    void setSmartHomeEnd(int mode); // 0 disabled,1 before,2 after,3 always
    void setSearchHighlights(const QList<QTextEdit::ExtraSelection> &sel);
    bool lineNumbersVisible() const { return m_lineNumbersVisible; }
    void setLineNumbersVisible(bool on);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    // cursor position persistence helper
    int cursorPosition() const { return textCursor().position(); }
    void setCursorPosition(int pos);
    QTextBlock firstVisibleBlockPublic() const { return firstVisibleBlock(); }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateBracketMatch();

private:
    QWidget *m_lineNumberArea;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
    KSyntaxHighlighting::Repository m_repo;
    QString m_lang = "text";
    QString m_highlightPath;
    QColor m_currentLineHighlight;

    int m_tabSize = 4;
    bool m_insertSpaces = true;
    bool m_autoIndent = false;
    bool m_bracketMatching = false;
    bool m_rightMargin = false;
    int m_rightMarginColumn = 120;
    bool m_drawWhitespace = false;
    bool m_autoBracket = true;
    bool m_syntaxHighlighting = true;
    bool m_lineNumbersVisible = true;
    int m_smartHomeEnd = 2; // after
    int m_bracketMatchPos = -1;
    QList<QTextEdit::ExtraSelection> m_searchSelections;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(Editor *editor) : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent *event) override { m_editor->lineNumberAreaPaintEvent(event); }
private:
    Editor *m_editor;
};
