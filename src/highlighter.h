#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class Highlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *parent = nullptr);
    void setLanguage(const QString &lang);
    void setColors(const QColor &keyword, const QColor &type, const QColor &string,
                   const QColor &comment, const QColor &number, const QColor &function,
                   const QColor &preprocessor);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    void buildRules();
    void applyRules(const QString &text, const QList<Rule> &rules);

    QString m_lang;
    QList<Rule> m_rules;
    QRegularExpression m_commentStart;
    QRegularExpression m_commentEnd;
    bool m_hasMultilineComment = false;

    QTextCharFormat m_keywordFmt;
    QTextCharFormat m_typeFmt;
    QTextCharFormat m_stringFmt;
    QTextCharFormat m_commentFmt;
    QTextCharFormat m_numberFmt;
    QTextCharFormat m_functionFmt;
    QTextCharFormat m_preprocessorFmt;
};
