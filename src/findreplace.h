#pragma once
#include <QDialog>
#include <QPlainTextEdit>

class FindReplaceDialog : public QDialog {
    Q_OBJECT
public:
    explicit FindReplaceDialog(QPlainTextEdit *editor, QWidget *parent = nullptr);
    void setEditor(QPlainTextEdit *editor);
    QPlainTextEdit *editor() const { return m_editor; }

private slots:
    void findNext();
    void replace();
    void replaceAll();

private:
    QPlainTextEdit *m_editor;
    class QLineEdit *m_find;
    class QLineEdit *m_replace;
    class QCheckBox *m_case;
    class QCheckBox *m_whole;
};
