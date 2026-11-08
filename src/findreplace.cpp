#include "findreplace.h"
#include "editor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

FindReplaceDialog::FindReplaceDialog(QPlainTextEdit *editor, QWidget *parent)
    : QDialog(parent), m_editor(editor) {
    setWindowTitle(tr("Find and Replace"));
    setModal(false);

    m_find = new QLineEdit(this);
    m_replace = new QLineEdit(this);
    m_case = new QCheckBox(tr("Match case"), this);
    m_whole = new QCheckBox(tr("Whole word"), this);

    auto *form = new QVBoxLayout(this);
    auto *r1 = new QHBoxLayout();
    r1->addWidget(new QLabel(tr("Find:"), this));
    r1->addWidget(m_find);
    form->addLayout(r1);
    auto *r2 = new QHBoxLayout();
    r2->addWidget(new QLabel(tr("Replace:"), this));
    r2->addWidget(m_replace);
    form->addLayout(r2);
    auto *r3 = new QHBoxLayout();
    r3->addWidget(m_case);
    r3->addWidget(m_whole);
    form->addLayout(r3);
    auto *r4 = new QHBoxLayout();
    auto *bFind = new QPushButton(tr("Find Next"), this);
    auto *bRep = new QPushButton(tr("Replace"), this);
    auto *bAll = new QPushButton(tr("Replace All"), this);
    auto *bClose = new QPushButton(tr("Close"), this);
    r4->addWidget(bFind); r4->addWidget(bRep); r4->addWidget(bAll); r4->addWidget(bClose);
    form->addLayout(r4);

    connect(bFind, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(bRep, &QPushButton::clicked, this, &FindReplaceDialog::replace);
    connect(bAll, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
    connect(bClose, &QPushButton::clicked, this, &QDialog::close);
    connect(m_find, &QLineEdit::returnPressed, this, &FindReplaceDialog::findNext);
}

static QTextDocument::FindFlags flags(bool cs, bool whole) {
    QTextDocument::FindFlags f;
    if (cs) f |= QTextDocument::FindCaseSensitively;
    if (whole) f |= QTextDocument::FindWholeWords;
    return f;
}

static void updateSearchHighlights(QPlainTextEdit *editor, const QString &needle, bool cs, bool whole) {
    Editor *ed = qobject_cast<Editor*>(editor);
    if (!ed) return;
    QList<QTextEdit::ExtraSelection> sels;
    if (!needle.isEmpty()) {
        QTextCharFormat fmt;
        fmt.setBackground(QColor(255,235,59,80));
        QTextCursor c(ed->document());
        while (true) {
            QTextCursor found = ed->document()->find(needle, c, flags(cs, whole));
            if (found.isNull()) break;
            QTextEdit::ExtraSelection sel;
            sel.format = fmt;
            sel.cursor = found;
            sels.append(sel);
            c = found;
            c.movePosition(QTextCursor::NextCharacter);
        }
    }
    ed->setSearchHighlights(sels);
}

void FindReplaceDialog::findNext() {
    QString needle = m_find->text();
    if (needle.isEmpty()) { updateSearchHighlights(m_editor, QString(), false, false); return; }
    QTextCursor c = m_editor->textCursor();
    QTextCursor found = m_editor->document()->find(needle, c, flags(m_case->isChecked(), m_whole->isChecked()));
    if (found.isNull()) {
        // wrap
        found = m_editor->document()->find(needle, QTextCursor(m_editor->document()), flags(m_case->isChecked(), m_whole->isChecked()));
    }
    if (!found.isNull()) {
        m_editor->setTextCursor(found);
        m_editor->ensureCursorVisible();
    }
    updateSearchHighlights(m_editor, needle, m_case->isChecked(), m_whole->isChecked());
}

void FindReplaceDialog::replace() {
    QTextCursor c = m_editor->textCursor();
    if (c.hasSelection() && c.selectedText() == m_find->text()) {
        c.insertText(m_replace->text());
    }
    findNext();
    updateSearchHighlights(m_editor, m_find->text(), m_case->isChecked(), m_whole->isChecked());
}

void FindReplaceDialog::replaceAll() {
    QString needle = m_find->text();
    if (needle.isEmpty()) return;
    QTextCursor c(m_editor->document());
    c.beginEditBlock();
    int guard = 0;
    while (guard++ < 1000000) {
        QTextCursor found = m_editor->document()->find(needle, c, flags(m_case->isChecked(), m_whole->isChecked()));
        if (found.isNull()) break;
        found.insertText(m_replace->text());
        // Advance past the inserted text to avoid re-matching it.
        c = found;
        if (c.position() == found.position() && m_replace->text().isEmpty() && needle.size() > 0) {
            // Ensure forward progress when deleting all matches.
            c.movePosition(QTextCursor::NextCharacter);
        }
    }
    c.endEditBlock();
    updateSearchHighlights(m_editor, m_find->text(), m_case->isChecked(), m_whole->isChecked());
}

void FindReplaceDialog::setEditor(QPlainTextEdit *editor) {
    m_editor = editor;
}
