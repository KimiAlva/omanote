#include "highlighter.h"
#include <QColor>

static QTextCharFormat makeFmt(const QColor &color, bool bold = false, bool italic = false) {
    QTextCharFormat f;
    f.setForeground(color);
    if (bold) f.setFontWeight(QFont::Bold);
    if (italic) f.setFontItalic(true);
    return f;
}

Highlighter::Highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    // Base formats; concrete colors set in buildRules via theme? We use fixed palette that reads OK on dark.
    m_keywordFmt = makeFmt(QColor("#cba6f7"), true);
    m_typeFmt = makeFmt(QColor("#89b4fa"));
    m_stringFmt = makeFmt(QColor("#a6e3a1"));
    m_commentFmt = makeFmt(QColor("#6c7086"), false, true);
    m_numberFmt = makeFmt(QColor("#fab387"));
    m_functionFmt = makeFmt(QColor("#89dceb"));
    m_preprocessorFmt = makeFmt(QColor("#f9e2af"));
    setLanguage("text");
}

void Highlighter::setLanguage(const QString &lang) {
    m_lang = lang.toLower();
    buildRules();
    rehighlight();
}

void Highlighter::setColors(const QColor &keyword, const QColor &type, const QColor &string,
                           const QColor &comment, const QColor &number, const QColor &function,
                           const QColor &preprocessor) {
    m_keywordFmt = makeFmt(keyword, true);
    m_typeFmt = makeFmt(type);
    m_stringFmt = makeFmt(string);
    m_commentFmt = makeFmt(comment, false, true);
    m_numberFmt = makeFmt(number);
    m_functionFmt = makeFmt(function);
    m_preprocessorFmt = makeFmt(preprocessor);
    if (m_lang == "md" || m_lang == "markdown") {
        // headings use accent-ish; keep fixed? update below not needed
    }
    buildRules();
    rehighlight();
}

void Highlighter::buildRules() {
    m_rules.clear();
    m_hasMultilineComment = false;
    m_commentStart = QRegularExpression();
    m_commentEnd = QRegularExpression();

    if (m_lang == "none") return;

    QStringList keywords, types;
    QString lineComment, blockStart, blockEnd;
    bool hasPreprocessor = false;
    bool stringsDouble = true, stringsSingle = true;

    if (m_lang == "c" || m_lang == "cpp" || m_lang == "c++" || m_lang == "h" || m_lang == "hpp") {
        keywords = {"break","case","catch","class","const","continue","default","delete","do","else","enum",
            "explicit","extern","for","friend","goto","if","inline","namespace","new","operator","private",
            "protected","public","return","sizeof","static","struct","switch","template","this","throw",
            "try","typedef","typename","union","using","virtual","while","constexpr","noexcept","override",
            "final","auto","nullptr","true","false","NULL"};
        types = {"int","char","float","double","void","bool","long","short","unsigned","signed","size_t",
            "string","vector","map","set","list","array","unique_ptr","shared_ptr","uint8_t","uint32_t",
            "uint64_t","int32_t","int64_t","std"};
        lineComment = "//"; blockStart = "/\\*"; blockEnd = "\\*/";
        hasPreprocessor = true;
    } else if (m_lang == "python" || m_lang == "py") {
        keywords = {"and","as","assert","async","await","break","class","continue","def","del","elif","else",
            "except","finally","for","from","global","if","import","in","is","lambda","nonlocal","not","or",
            "pass","raise","return","try","while","with","yield","True","False","None"};
        types = {"int","str","float","bool","list","dict","tuple","set","bytes","object"};
        lineComment = "#"; stringsSingle = true; stringsDouble = true;
        // triple-quoted strings handled as comments-ish? keep simple
    } else if (m_lang == "javascript" || m_lang == "js" || m_lang == "ts" || m_lang == "typescript" || m_lang == "jsx" || m_lang == "tsx") {
        keywords = {"break","case","catch","class","const","continue","debugger","default","delete","do",
            "else","export","extends","finally","for","function","if","import","in","instanceof","let","new",
            "return","super","switch","this","throw","try","typeof","var","void","while","with","yield",
            "async","await","true","false","null","undefined"};
        types = {"Array","Boolean","Date","Error","Function","JSON","Map","Math","Number","Object","Promise",
            "RegExp","Set","String","Symbol"};
        lineComment = "//"; blockStart = "/\\*"; blockEnd = "\\*/";
    } else if (m_lang == "rust" || m_lang == "rs") {
        keywords = {"as","async","await","break","const","continue","crate","dyn","else","enum","extern",
            "false","fn","for","if","impl","in","let","loop","match","mod","move","mut","pub","ref",
            "return","self","Self","static","struct","super","trait","true","type","unsafe","use","where",
            "while"};
        types = {"i8","i16","i32","i64","i128","u8","u16","u32","u64","u128","f32","f64","usize",
            "isize","bool","char","str","String","Vec","Option","Result","Box","Rc","Arc"};
        lineComment = "//"; blockStart = "/\\*"; blockEnd = "\\*/";
        hasPreprocessor = false;
    } else if (m_lang == "go") {
        keywords = {"break","case","chan","const","continue","default","defer","else","fallthrough","for",
            "func","go","goto","if","import","interface","map","package","range","return","select","struct",
            "switch","type","var","true","false","nil","iota"};
        types = {"bool","byte","complex64","complex128","error","float32","float64","int","int8","int16",
            "int32","int64","rune","string","uint","uint8","uint16","uint32","uint64","uintptr"};
        lineComment = "//"; blockStart = "/\\*"; blockEnd = "\\*/";
    } else if (m_lang == "java" || m_lang == "kt" || m_lang == "kotlin") {
        keywords = {"abstract","assert","break","case","catch","class","const","continue","default","do",
            "else","enum","extends","final","finally","for","if","implements","import","instanceof",
            "interface","native","new","package","private","protected","public","return","static","strictfp",
            "super","switch","synchronized","this","throw","throws","transient","try","volatile","while",
            "true","false","null"};
        types = {"boolean","byte","char","double","float","int","long","short","void","String","Object",
            "List","Map","Set","ArrayList","HashMap"};
        lineComment = "//"; blockStart = "/\\*"; blockEnd = "\\*/";
    } else if (m_lang == "sh" || m_lang == "bash" || m_lang == "zsh") {
        keywords = {"if","then","else","elif","fi","case","esac","for","while","do","done","in","function",
            "return","exit","export","local","readonly","unset","source","alias","unalias","trap","break",
            "continue","eval","exec","shift","set","unset"};
        lineComment = "#";
    } else if (m_lang == "json") {
        // only strings/numbers/keywords true/false/null
        keywords = {"true","false","null"};
        lineComment = "";
    } else if (m_lang == "html" || m_lang == "xml" || m_lang == "svg") {
        lineComment = ""; // tags handled separately minimal
    } else if (m_lang == "css") {
        // minimal
    } else if (m_lang == "md" || m_lang == "markdown") {
        // headings handled below
    }

    // Common keyword rules
    auto addKeywordRules = [&](const QStringList &kws, const QTextCharFormat &fmt) {
        for (const QString &kw : kws) {
            Rule r;
            r.pattern = QRegularExpression("\\b" + QRegularExpression::escape(kw) + "\\b");
            r.format = fmt;
            m_rules.append(r);
        }
    };
    addKeywordRules(keywords, m_keywordFmt);
    addKeywordRules(types, m_typeFmt);

    // Numbers
    {
        Rule r;
        r.pattern = QRegularExpression("\\b[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?\\b");
        r.format = m_numberFmt;
        m_rules.append(r);
    }

    // Preprocessor
    if (hasPreprocessor) {
        Rule r;
        r.pattern = QRegularExpression("^\\s*#\\s*[a-zA-Z_]+");
        r.format = m_preprocessorFmt;
        m_rules.append(r);
    }

    // Strings (double and single)
    if (stringsDouble) {
        Rule r;
        r.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"");
        r.format = m_stringFmt;
        m_rules.append(r);
    }
    if (stringsSingle && m_lang != "python") {
        Rule r;
        r.pattern = QRegularExpression("'([^'\\\\]|\\\\.)*'");
        r.format = m_stringFmt;
        m_rules.append(r);
    }
    if (m_lang == "python") {
        // single-quoted strings too
        Rule r;
        r.pattern = QRegularExpression("'([^'\\\\]|\\\\.)*'");
        r.format = m_stringFmt;
        m_rules.append(r);
    }

    // Comments
    if (!lineComment.isEmpty()) {
        Rule r;
        r.pattern = QRegularExpression(QRegularExpression::escape(lineComment) + "[^\\n]*");
        r.format = m_commentFmt;
        m_rules.append(r);
    }
    if (!blockStart.isEmpty()) {
        m_hasMultilineComment = true;
        m_commentStart = QRegularExpression(blockStart);
        m_commentEnd = QRegularExpression(blockEnd);
    }

    // Markdown headings
    if (m_lang == "md" || m_lang == "markdown") {
        Rule r;
        r.pattern = QRegularExpression("^#{1,6}\\s.*$");
        r.format = makeFmt(QColor("#89b4fa"), true);
        m_rules.append(r);
        Rule r2;
        r2.pattern = QRegularExpression("\\*\\*[^*]+\\*\\*");
        r2.format = makeFmt(QColor("#cdd6f4"), true);
        m_rules.append(r2);
        Rule r3;
        r3.pattern = QRegularExpression("`[^`]+`");
        r3.format = m_stringFmt;
        m_rules.append(r3);
    }
}

void Highlighter::highlightBlock(const QString &text) {
    for (const Rule &r : m_rules) {
        QRegularExpressionMatchIterator it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.format);
        }
    }

    if (m_hasMultilineComment) {
        setCurrentBlockState(0);
        int startIndex = 0;
        if (previousBlockState() != 1) {
            QRegularExpressionMatch m = m_commentStart.match(text);
            startIndex = m.hasMatch() ? m.capturedStart() : -1;
        }
        while (startIndex >= 0) {
            QRegularExpressionMatch endMatch = m_commentEnd.match(text, startIndex);
            int endIndex = endMatch.hasMatch() ? endMatch.capturedStart() + endMatch.capturedLength() : -1;
            int length;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                length = text.length() - startIndex;
            } else {
                length = endIndex - startIndex;
            }
            setFormat(startIndex, length, m_commentFmt);
            if (endIndex == -1) break;
            QRegularExpressionMatch nextStart = m_commentStart.match(text, endIndex);
            startIndex = nextStart.hasMatch() ? nextStart.capturedStart() : -1;
        }
    }
}

void Highlighter::applyRules(const QString &text, const QList<Rule> &rules) {
    for (const Rule &r : rules) {
        QRegularExpressionMatchIterator it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.format);
        }
    }
}
