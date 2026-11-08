#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>
#include <QFont>
#include <QApplication>
#include <QCloseEvent>
#include <QShortcut>
#include <QKeySequence>
#include <QSettings>
#include <QInputDialog>
#include <QLocale>
#include <QFormLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QFileSystemWatcher>
#include <QDockWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QScrollBar>
#include <QPainter>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QSettings>
#include <QPrinter>
#include <QPrintDialog>
#include <QActionGroup>
#include <QTimer>

static const char *EXT_LANG[][2] = {
    {"c","c"},{"h","c"},{"cpp","cpp"},{"cc","cpp"},{"cxx","cpp"},{"hpp","cpp"},{"hh","cpp"},
    {"py","python"},{"pyw","python"},
    {"js","javascript"},{"jsx","javascript"},{"ts","typescript"},{"tsx","typescript"},
    {"rs","rust"},
    {"go","go"},
    {"java","java"},{"kt","kotlin"},{"kts","kotlin"},
    {"sh","sh"},{"bash","sh"},{"zsh","sh"},
    {"json","json"},
    {"html","html"},{"htm","html"},{"xml","xml"},{"svg","xml"},
    {"css","css"},
    {"md","markdown"},{"markdown","markdown"},
    {nullptr,nullptr}
};

static QString detectEncoding(const QByteArray &data) {
    if (data.startsWith("\xEF\xBB\xBF")) return "UTF-8";
    if (data.startsWith("\xFF\xFE")) return "UTF-16LE";
    if (data.startsWith("\xFE\xFF")) return "UTF-16BE";
    // try UTF-8
    QStringDecoder utf8(QStringDecoder::Utf8);
    (void)utf8(data);
    if (!utf8.hasError()) return "UTF-8";
    return "Windows-1252";
}

static QString decodeWithEncoding(const QByteArray &data, const QString &enc) {
    if (enc == "UTF-8") {
        QByteArray d = data;
        if (d.startsWith("\xEF\xBB\xBF")) d = d.mid(3);
        return QString::fromUtf8(d);
    }
    QStringDecoder dec(enc.toUtf8().constData());
    if (!dec.isValid()) dec = QStringDecoder(QStringDecoder::Latin1);
    return dec(data);
}

static QByteArray encodeWithEncoding(const QString &text, const QString &enc) {
    if (enc == "UTF-8") return text.toUtf8();
    QStringEncoder encr(enc.toUtf8().constData());
    if (!encr.isValid()) encr = QStringEncoder(QStringEncoder::Latin1);
    return encr(text);
}

MinimapWidget::MinimapWidget(QWidget *parent) : QWidget(parent) {
    setMinimumWidth(60);
    setMaximumWidth(120);
}

void MinimapWidget::setEditor(Editor *ed) {
    if (m_editor) {
        m_editor->document()->disconnect(this);
        m_editor->verticalScrollBar()->disconnect(this);
    }
    m_editor = ed;
    if (m_editor) {
        connect(m_editor->document(), &QTextDocument::contentsChanged, this, &MinimapWidget::refresh);
        connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, this, &MinimapWidget::refresh);
        connect(m_editor, &QPlainTextEdit::updateRequest, this, [this](const QRect&, int){ refresh(); });
    }
    refresh();
}

void MinimapWidget::refresh() { update(); }

void MinimapWidget::paintEvent(QPaintEvent *e) {
    QWidget::paintEvent(e);
    QPainter p(this);
    p.fillRect(rect(), palette().base());
    if (!m_editor) return;
    QTextDocument *doc = m_editor->document();
    int total = doc->blockCount();
    if (total <= 0) return;
    qreal h = height();
    qreal w = width();
    // draw lines
    p.setPen(QColor(108,112,134,100));
    QTextBlock block = doc->firstBlock();
    int i = 0;
    while (block.isValid()) {
        qreal y = (qreal)i / total * h;
        QString text = block.text();
        int len = qMin(text.size(), 100);
        qreal lw = (len / 100.0) * (w - 6);
        if (lw < 2) lw = 2;
        p.drawLine(QPointF(3, y), QPointF(3 + lw, y));
        block = block.next();
        i++;
    }
    // viewport indicator
    QTextBlock first = m_editor->firstVisibleBlockPublic();
    int firstNum = first.blockNumber();
    int visible = m_editor->viewport()->height() / qMax(1, m_editor->fontMetrics().height());
    qreal y0 = (qreal)firstNum / total * h;
    qreal y1 = (qreal)(firstNum + visible) / total * h;
    if (y1 < y0 + 4) y1 = y0 + 4;
    p.fillRect(QRectF(0, y0, w, y1 - y0), QColor(255,255,255,40));
    p.setPen(QColor(255,255,255,80));
    p.drawRect(QRectF(0, y0, w - 1, y1 - y0 - 1));
}

void MinimapWidget::mousePressEvent(QMouseEvent *e) {
    if (!m_editor) return;
    int total = m_editor->document()->blockCount();
    if (total <= 0) return;
    qreal ratio = (qreal)e->pos().y() / height();
    int blockNum = qBound(0, (int)(ratio * total), total - 1);
    QTextCursor c(m_editor->document()->findBlockByNumber(blockNum));
    m_editor->setTextCursor(c);
    m_editor->centerCursor();
}

void MinimapWidget::mouseMoveEvent(QMouseEvent *e) {
    mousePressEvent(e);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_theme = new Theme(this);
    loadSettings();
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    setCentralWidget(m_tabs);

    // Side panel: file browser
    m_sideDock = new QDockWidget(tr("Files"), this);
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(QDir::homePath());
    m_fileTree = new QTreeView(m_sideDock);
    m_fileTree->setModel(m_fileModel);
    m_fileTree->setRootIndex(m_fileModel->index(QDir::homePath()));
    m_sideDock->setWidget(m_fileTree);
    addDockWidget(Qt::LeftDockWidgetArea, m_sideDock);
    m_sideDock->hide();
    connect(m_fileTree, &QTreeView::doubleClicked, this, [this](const QModelIndex &idx){
        QString path = m_fileModel->filePath(idx);
        QFileInfo fi(path);
        if (fi.isFile()) openFile(path);
    });

    // Bottom panel: document info
    m_bottomDock = new QDockWidget(tr("Document Info"), this);
    m_docInfo = new QLabel(tr("No document"), m_bottomDock);
    m_docInfo->setMargin(6);
    m_bottomDock->setWidget(m_docInfo);
    addDockWidget(Qt::BottomDockWidgetArea, m_bottomDock);
    m_bottomDock->hide();

    // Minimap dock
    m_minimapDock = new QDockWidget(tr("Minimap"), this);
    m_minimap = new MinimapWidget(m_minimapDock);
    m_minimapDock->setWidget(m_minimap);
    addDockWidget(Qt::RightDockWidgetArea, m_minimapDock);
    m_minimapDock->hide();

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::currentChanged);

    // Menu bar
    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&New"), this, &MainWindow::newFile, QKeySequence::New);
    auto *scClose = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(scClose, &QShortcut::activated, this, [this](){ if (m_tabs->count()>0) closeTab(m_tabs->currentIndex()); });
    auto *scGoto = new QShortcut(QKeySequence("Ctrl+G"), this);
    connect(scGoto, &QShortcut::activated, this, [this](){
        Editor *ed = currentEditor(); if (!ed) return;
        bool ok; int line = QInputDialog::getInt(this, tr("Go to line"), tr("Line:"), ed->textCursor().blockNumber()+1, 1, ed->blockCount(), 1, &ok);
        if (ok) {
            QTextCursor c(ed->document()->findBlockByNumber(line-1));
            ed->setTextCursor(c);
            ed->centerCursor();
        }
    });
    file->addAction(tr("&Open..."), this, &MainWindow::open, QKeySequence::Open);
    file->addSeparator();
    file->addAction(tr("&Save"), this, &MainWindow::save, QKeySequence::Save);
    file->addAction(tr("Save &As..."), this, &MainWindow::saveAs, QKeySequence::SaveAs);
    file->addAction(tr("&Print..."), this, &MainWindow::printDocument, QKeySequence::Print);
    QMenu *encMenu = file->addMenu(tr("&Encoding"));
    QStringList encs = {"UTF-8", "UTF-16LE", "UTF-16BE", "Windows-1252", "ISO-8859-1", "ISO-8859-15"};
    for (const QString &e : encs) {
        QAction *a = encMenu->addAction(e);
        connect(a, &QAction::triggered, this, [this, e](){
            Editor *ed = currentEditor();
            if (!ed) return;
            ed->setProperty("encoding", e);
            updateStatus();
        });
    }
    file->addSeparator();
    m_recentMenu = file->addMenu(tr("Recent &Files"));
    updateRecentFilesMenu();
    file->addSeparator();
    file->addAction(tr("&Quit"), this, &MainWindow::close, QKeySequence::Quit);

    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    edit->addAction(tr("&Undo"), this, [this](){ if (auto e=currentEditor()) e->undo(); }, QKeySequence::Undo);
    edit->addAction(tr("&Redo"), this, [this](){ if (auto e=currentEditor()) e->redo(); }, QKeySequence::Redo);
    edit->addSeparator();
    edit->addAction(tr("Cu&t"), this, [this](){ if (auto e=currentEditor()) e->cut(); }, QKeySequence::Cut);
    edit->addAction(tr("&Copy"), this, [this](){ if (auto e=currentEditor()) e->copy(); }, QKeySequence::Copy);
    edit->addAction(tr("&Paste"), this, [this](){ if (auto e=currentEditor()) e->paste(); }, QKeySequence::Paste);
    edit->addAction(tr("Select &All"), this, [this](){ if (auto e=currentEditor()) e->selectAll(); }, QKeySequence::SelectAll);
    edit->addSeparator();
    edit->addAction(tr("&Find/Replace..."), this, &MainWindow::showFindReplace, QKeySequence("Ctrl+F"));
    edit->addSeparator();
    edit->addAction(tr("&Preferences..."), this, &MainWindow::showPreferences);

    QMenu *view = menuBar()->addMenu(tr("&View"));
    view->addAction(tr("Zoom &In"), this, &MainWindow::zoomIn, QKeySequence::ZoomIn);
    view->addAction(tr("Zoom &Out"), this, &MainWindow::zoomOut, QKeySequence::ZoomOut);
    view->addSeparator();
    QAction *actLineNums = view->addAction(tr("Line &Numbers"));
    actLineNums->setCheckable(true);
    actLineNums->setChecked(m_lineNumbers);
    connect(actLineNums, &QAction::toggled, this, [this](bool on){ m_lineNumbers = on; saveSettings(); for (int i=0;i<m_tabs->count();i++) if (auto e=qobject_cast<Editor*>(m_tabs->widget(i))) e->setLineNumbersVisible(on); });
    QAction *actCurLine = view->addAction(tr("&Highlight Current Line"));
    actCurLine->setCheckable(true);
    actCurLine->setChecked(m_highlightCurrentLine);
    connect(actCurLine, &QAction::toggled, this, [this](bool on){ m_highlightCurrentLine = on; saveSettings(); QColor hl = m_theme->selection(); hl.setAlpha(on?90:0); for (int i=0;i<m_tabs->count();i++) if (auto e=qobject_cast<Editor*>(m_tabs->widget(i))) e->setCurrentLineHighlight(hl); });
    QAction *actBracket = view->addAction(tr("&Bracket Matching"));
    actBracket->setCheckable(true);
    actBracket->setChecked(m_bracketMatching);
    connect(actBracket, &QAction::toggled, this, [this](bool on){ m_bracketMatching = on; saveSettings(); for (int i=0;i<m_tabs->count();i++) if (auto e=qobject_cast<Editor*>(m_tabs->widget(i))) e->setBracketMatching(on); });
    QAction *actMargin = view->addAction(tr("&Right Margin"));
    actMargin->setCheckable(true);
    actMargin->setChecked(m_rightMargin);
    connect(actMargin, &QAction::toggled, this, [this](bool on){ m_rightMargin = on; saveSettings(); for (int i=0;i<m_tabs->count();i++) if (auto e=qobject_cast<Editor*>(m_tabs->widget(i))) e->setRightMargin(on, m_rightMarginColumn); });
    QAction *actWrap = view->addAction(tr("&Word Wrap"));
    actWrap->setCheckable(true);
    actWrap->setChecked(m_wrap);
    connect(actWrap, &QAction::toggled, this, [this](bool on){ m_wrap = on; saveSettings(); for (int i=0;i<m_tabs->count();i++) if (auto e=qobject_cast<Editor*>(m_tabs->widget(i))) e->setWordWrapMode(on ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap); });
    // word wrap handled above
    view->addSeparator();
    QAction *actSide = view->addAction(tr("Side &Panel"));
    actSide->setCheckable(true);
    connect(actSide, &QAction::toggled, this, [this](bool on){ if (m_sideDock) m_sideDock->setVisible(on); });
    QAction *actBottom = view->addAction(tr("&Bottom Panel"));
    actBottom->setCheckable(true);
    connect(actBottom, &QAction::toggled, this, [this](bool on){ if (m_bottomDock) m_bottomDock->setVisible(on); });
    QAction *actMini = view->addAction(tr("&Minimap"));
    actMini->setCheckable(true);
    connect(actMini, &QAction::toggled, this, [this](bool on){ if (m_minimapDock) m_minimapDock->setVisible(on); });

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About"), this, &MainWindow::about);

    m_status = new QLabel(this);
    statusBar()->addPermanentWidget(m_status);

    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path){
        Editor *ed = currentEditor();
        if (!ed) return;
        QString filePath = ed->property("filePath").toString();
        if (path != filePath) return;
        auto r = QMessageBox::question(this, tr("File changed"),
            tr("The file has been changed externally. Reload?"),
            QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::Yes) {
            // reload
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly)) return;
            QByteArray data = f.readAll();
            f.close();
            QString enc = ed->property("encoding").toString();
            if (enc.isEmpty()) enc = detectEncoding(data);
            ed->setPlainText(decodeWithEncoding(data, enc));
            ed->document()->setModified(false);
            ed->setProperty("encoding", enc);
            updateTitle();
            updateStatus();
        }
    });

    applyTheme();
    connect(m_theme, &Theme::changed, this, &MainWindow::applyTheme);

    // Autosave timer
    m_autosaveTimer = new QTimer(this);
    connect(m_autosaveTimer, &QTimer::timeout, this, [this](){
        if (!m_autosave) return;
        for (int i = 0; i < m_tabs->count(); i++) {
            if (auto ed = qobject_cast<Editor*>(m_tabs->widget(i))) {
                if (ed->document()->isModified()) {
                    QString path = ed->property("filePath").toString();
                    if (!path.isEmpty()) saveFile(ed, path);
                }
            }
        }
    });
    m_autosaveTimer->start(m_autosaveInterval * 60000);

    // Empty first tab
    newFile();

    QSettings gs("omarchy", "note");
    if (gs.contains("geometry"))
        restoreGeometry(gs.value("geometry").toByteArray());
    else
        resize(900, 650);
    if (gs.contains("windowState"))
        restoreState(gs.value("windowState").toByteArray());
    setWindowTitle("Note");
}

Editor *MainWindow::currentEditor() const {
    if (m_tabs->count() == 0) return nullptr;
    return qobject_cast<Editor*>(m_tabs->currentWidget());
}

bool MainWindow::maybeSave(Editor *ed) {
    if (!ed || !ed->document()->isModified()) return true;
    auto r = QMessageBox::warning(this, tr("Unsaved changes"),
        tr("This document has unsaved changes. Discard?"),
        QMessageBox::Discard | QMessageBox::Cancel);
    return r == QMessageBox::Discard;
}

bool MainWindow::saveFile(Editor *ed, const QString &path) {
    // ensure trailing newline
    if (m_ensureTrailingNewline) ensureTrailingNewline(ed);
    // trim trailing spaces
    if (m_trimTrailingSpaces) {
        QTextCursor c(ed->document());
        c.beginEditBlock();
        QTextBlock b = ed->document()->firstBlock();
        while (b.isValid()) {
            QString t = b.text();
            int i = t.size();
            while (i > 0 && (t[i-1] == ' ' || t[i-1] == '\t')) i--;
            if (i < t.size()) {
                QTextCursor bc(ed->document());
                bc.setPosition(b.position() + i);
                bc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                bc.removeSelectedText();
            }
            b = b.next();
        }
        c.endEditBlock();
    }
    // backup copy
    if (m_createBackup && QFile::exists(path)) {
        QFile::copy(path, path + "~");
    }
    QString enc = ed->property("encoding").toString();
    if (enc.isEmpty()) enc = "UTF-8";
    QByteArray bytes = encodeWithEncoding(ed->toPlainText(), enc);
    QFile f(path);
    // atomic: write to temp then rename
    QString tmp = path + ".tmp.note";
    QFile t(tmp);
    if (!t.open(QIODevice::WriteOnly)) return false;
    t.write(bytes);
    t.close();
    if (!QFile::rename(tmp, path)) {
        // fallback
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QTextStream o(&f); o << ed->toPlainText(); f.close();
        QFile::remove(tmp);
    }
    ed->document()->setModified(false);
    int idx = m_tabs->indexOf(ed);
    if (idx >= 0) m_tabs->setTabText(idx, QFileInfo(path).fileName());
    ed->setProperty("filePath", path);
    if (m_restoreCursorPosition) {
        QSettings s("omarchy", "note");
        s.setValue("cursor/" + path, ed->cursorPosition());
    }
    addToRecentFiles(path);
    return true;
}

void MainWindow::newFile() {
    Editor *ed = new Editor(this);
    addTab(ed, "Untitled");
    ed->setProperty("encoding", "UTF-8");
}

void MainWindow::applySettingsToEditor(Editor *ed) {
    if (!ed) return;
    ed->setFontFamily("Monospace", m_fontSize);
    ed->setTabSize(m_tabSize);
    ed->setInsertSpaces(m_insertSpaces);
    ed->setAutoIndent(m_autoIndent);
    ed->setLineNumbersVisible(m_lineNumbers);
    ed->setCurrentLineHighlight(m_highlightCurrentLine ? [this](){ QColor hl = m_theme->selection(); hl.setAlpha(90); return hl; }() : QColor(0,0,0,0));
    ed->setBracketMatching(m_bracketMatching);
    ed->setRightMargin(m_rightMargin, m_rightMarginColumn);
    ed->setDrawWhitespace(m_drawWhitespace);
    ed->setAutoBracketCompletion(m_autoBracketCompletion);
    ed->setSyntaxHighlighting(m_syntaxHighlighting);
    ed->setSmartHomeEnd(m_smartHomeEnd);
    ed->setWordWrapMode(m_wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

void MainWindow::addTab(Editor *ed, const QString &title) {
    applySettingsToEditor(ed);
    m_tabs->addTab(ed, title);
    m_tabs->setCurrentWidget(ed);
    connect(ed, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatus);
    connect(ed->document(), &QTextDocument::modificationChanged, this, [this](bool){ updateTitle(); });
    connect(ed->document(), &QTextDocument::contentsChanged, this, [this, ed](){
        if (currentEditor() == ed) {
            if (m_docInfo) {
                int words = ed->toPlainText().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
                int chars = ed->toPlainText().size();
                int lines = ed->blockCount();
                m_docInfo->setText(QString("Lines: %1   Words: %2   Characters: %3").arg(lines).arg(words).arg(chars));
            }
        }
    });
    setEditorLanguage(ed, ed->property("filePath").toString());
    applyTheme();
}

void MainWindow::open() {
    QString path = QFileDialog::getOpenFileName(this, "Open file");
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::openFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file."));
        return;
    }
    QByteArray data = f.readAll();
    f.close();
    QString enc = detectEncoding(data);
    QString content = decodeWithEncoding(data, enc);

    // reuse empty untitled tab?
    Editor *ed = currentEditor();
    if (ed && !ed->document()->isModified() && ed->toPlainText().isEmpty() && ed->property("filePath").toString().isEmpty()) {
        applySettingsToEditor(ed);
        // reuse
    } else {
        ed = new Editor(this);
        addTab(ed, QFileInfo(path).fileName());
    }
    ed->setPlainText(content);
    ed->document()->setModified(false);
    ed->setProperty("filePath", path);
    ed->setProperty("encoding", enc);
    int idx = m_tabs->indexOf(ed);
    if (idx >= 0) m_tabs->setTabText(idx, QFileInfo(path).fileName());
    setEditorLanguage(ed, path);
    if (m_restoreCursorPosition) {
        QSettings s("omarchy", "note");
        int pos = s.value("cursor/" + path, -1).toInt();
        if (pos >= 0) ed->setCursorPosition(pos);
    }
    addToRecentFiles(path);
    updateTitle();
}

void MainWindow::save() {
    Editor *ed = currentEditor();
    if (!ed) return;
    QString path = ed->property("filePath").toString();
    if (path.isEmpty()) { saveAs(); return; }
    saveFile(ed, path);
    updateTitle();
}

void MainWindow::saveAs() {
    Editor *ed = currentEditor();
    if (!ed) return;
    QString path = QFileDialog::getSaveFileName(this, "Save file");
    if (path.isEmpty()) return;
    saveFile(ed, path);
    updateTitle();
}

void MainWindow::closeTab(int index) {
    Editor *ed = qobject_cast<Editor*>(m_tabs->widget(index));
    if (!maybeSave(ed)) return;
    if (m_findDlg && m_findDlg->editor() == ed) {
        delete m_findDlg;
        m_findDlg = nullptr;
    }
    m_tabs->removeTab(index);
    ed->deleteLater();
    if (m_tabs->count() == 0) {
        if (m_closeLastTab) close();
        else newFile();
    }
    updateTitle();
}

void MainWindow::currentChanged(int /*index*/) {
    Editor *ed = currentEditor();
    // file watcher update
    if (!m_watchedFile.isEmpty()) {
        m_fileWatcher->removePath(m_watchedFile);
        m_watchedFile.clear();
    }
    if (ed) {
        QString p = ed->property("filePath").toString();
        if (!p.isEmpty() && QFile::exists(p)) {
            m_fileWatcher->addPath(p);
            m_watchedFile = p;
        }
    }
    if (m_minimap) m_minimap->setEditor(ed);
    if (m_docInfo) {
        if (ed) {
            int words = ed->toPlainText().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
            int chars = ed->toPlainText().size();
            int lines = ed->blockCount();
            m_docInfo->setText(QString("Lines: %1   Words: %2   Characters: %3").arg(lines).arg(words).arg(chars));
        } else {
            m_docInfo->setText("No document");
        }
    }
    updateTitle();
    updateStatus();
}

void MainWindow::loadSettings() {
    QSettings s("omarchy", "note");
    m_fontSize = s.value("fontSize", 12).toInt();
    m_wrap = s.value("wrap", false).toBool();
    m_lineNumbers = s.value("lineNumbers", true).toBool();
    m_highlightCurrentLine = s.value("highlightCurrentLine", true).toBool();
    m_bracketMatching = s.value("bracketMatching", false).toBool();
    m_rightMargin = s.value("rightMargin", false).toBool();
    m_rightMarginColumn = s.value("rightMarginColumn", 120).toInt();
    m_drawWhitespace = s.value("drawWhitespace", false).toBool();
    m_autoBracketCompletion = s.value("autoBracketCompletion", true).toBool();
    m_trimTrailingSpaces = s.value("trimTrailingSpaces", false).toBool();
    m_syntaxHighlighting = s.value("syntaxHighlighting", true).toBool();
    m_autoIndent = s.value("autoIndent", false).toBool();
    m_insertSpaces = s.value("insertSpaces", true).toBool();
    m_tabSize = s.value("tabSize", 4).toInt();
    m_restoreCursorPosition = s.value("restoreCursorPosition", true).toBool();
    m_ensureTrailingNewline = s.value("ensureTrailingNewline", true).toBool();
    m_autosave = s.value("autosave", false).toBool();
    m_autosaveInterval = s.value("autosaveInterval", 10).toInt();
    m_createBackup = s.value("createBackup", false).toBool();
    m_smartHomeEnd = s.value("smartHomeEnd", 2).toInt();
    m_closeLastTab = s.value("closeLastTab", false).toBool();
    m_searchHighlighting = s.value("searchHighlighting", true).toBool();
    m_language = s.value("language", QLocale::system().name()).toString();
    m_recentFiles = s.value("recentFiles").toStringList();
}

void MainWindow::saveSettings() {
    QSettings s("omarchy", "note");
    s.setValue("fontSize", m_fontSize);
    s.setValue("wrap", m_wrap);
    s.setValue("lineNumbers", m_lineNumbers);
    s.setValue("highlightCurrentLine", m_highlightCurrentLine);
    s.setValue("bracketMatching", m_bracketMatching);
    s.setValue("rightMargin", m_rightMargin);
    s.setValue("rightMarginColumn", m_rightMarginColumn);
    s.setValue("drawWhitespace", m_drawWhitespace);
    s.setValue("autoBracketCompletion", m_autoBracketCompletion);
    s.setValue("trimTrailingSpaces", m_trimTrailingSpaces);
    s.setValue("syntaxHighlighting", m_syntaxHighlighting);
    s.setValue("autoIndent", m_autoIndent);
    s.setValue("insertSpaces", m_insertSpaces);
    s.setValue("tabSize", m_tabSize);
    s.setValue("restoreCursorPosition", m_restoreCursorPosition);
    s.setValue("ensureTrailingNewline", m_ensureTrailingNewline);
    s.setValue("autosave", m_autosave);
    s.setValue("autosaveInterval", m_autosaveInterval);
    s.setValue("createBackup", m_createBackup);
    s.setValue("smartHomeEnd", m_smartHomeEnd);
    s.setValue("closeLastTab", m_closeLastTab);
    s.setValue("searchHighlighting", m_searchHighlighting);
    s.setValue("language", m_language);
    s.setValue("recentFiles", m_recentFiles);
}

void MainWindow::applyTheme() {
    QColor bg = m_theme->background();
    QColor fg = m_theme->foreground();
    QColor acc = m_theme->accent();
    int r = m_theme->radius();

    QString ss = QString(
        "QMainWindow { background: %1; color: %2; }"
        "QTabWidget::pane { border: 1px solid %3; border-radius: %4px; }"
        "QTabBar::tab { background: %5; color: %2; padding: 6px 12px; border-radius: %4px; }"
        "QTabBar::tab:selected { background: %1; border-bottom: 2px solid %6; }"
        "QMenuBar { background: %1; color: %2; }"
        "QMenuBar::item:selected { background: %3; }"
        "QMenu { background: %1; color: %2; border: 1px solid %3; }"
        "QMenu::item:selected { background: %6; color: %1; }"
        "QStatusBar { background: %1; color: %2; }"
        "QPlainTextEdit { background: %1; color: %2; border: none; selection-background-color: %3; }"
    ).arg(bg.name(), fg.name(), m_theme->muted().name()).arg(r)
     .arg(m_theme->muted().darker(120).name()).arg(acc.name());
    // fix: muted darker variable
    setStyleSheet(ss);

    // editor font color handled by palette
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Window, bg);
    pal.setColor(QPalette::WindowText, fg);
    pal.setColor(QPalette::Base, bg);
    pal.setColor(QPalette::Text, fg);
    pal.setColor(QPalette::Button, bg);
    pal.setColor(QPalette::ButtonText, fg);
    pal.setColor(QPalette::Highlight, m_theme->selection());
    pal.setColor(QPalette::HighlightedText, fg);
    qApp->setPalette(pal);

    QColor hl = m_theme->selection();
    hl.setAlpha(m_highlightCurrentLine ? 90 : 0);
    for (int i = 0; i < m_tabs->count(); i++) {
        if (auto e = qobject_cast<Editor*>(m_tabs->widget(i))) {
            e->setCurrentLineHighlight(hl);
            e->refreshHighlightTheme();
        }
    }
}

void MainWindow::showFindReplace() {
    Editor *ed = currentEditor();
    if (!ed) return;
    if (!m_findDlg) m_findDlg = new FindReplaceDialog(ed, this);
    else m_findDlg->setEditor(ed);
    m_findDlg->show();
    m_findDlg->raise();
    m_findDlg->activateWindow();
}

void MainWindow::zoomIn() {
    m_fontSize = qMin(48, m_fontSize + 1);
    for (int i = 0; i < m_tabs->count(); i++)
        if (auto e = qobject_cast<Editor*>(m_tabs->widget(i)))
            e->setFontFamily("Monospace", m_fontSize);
    saveSettings();
}

void MainWindow::zoomOut() {
    m_fontSize = qMax(6, m_fontSize - 1);
    for (int i = 0; i < m_tabs->count(); i++)
        if (auto e = qobject_cast<Editor*>(m_tabs->widget(i)))
            e->setFontFamily("Monospace", m_fontSize);
    saveSettings();
}

void MainWindow::toggleWrap() {
    m_wrap = !m_wrap;
    for (int i = 0; i < m_tabs->count(); i++) {
        if (auto e = qobject_cast<Editor*>(m_tabs->widget(i)))
            e->setWordWrapMode(m_wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    }
    saveSettings();
}

void MainWindow::addToRecentFiles(const QString &path) {
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > 10) m_recentFiles.removeLast();
    saveSettings();
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu() {
    if (!m_recentMenu) return;
    m_recentMenu->clear();
    if (m_recentFiles.isEmpty()) {
        QAction *a = m_recentMenu->addAction("(none)");
        a->setEnabled(false);
        return;
    }
    for (const QString &p : m_recentFiles) {
        QAction *a = m_recentMenu->addAction(QFileInfo(p).fileName());
        connect(a, &QAction::triggered, this, [this, p](){ openFile(p); });
    }
    m_recentMenu->addSeparator();
    QAction *clear = m_recentMenu->addAction("Clear Recent Files");
    connect(clear, &QAction::triggered, this, [this](){ m_recentFiles.clear(); saveSettings(); updateRecentFilesMenu(); });
}

void MainWindow::printDocument() {
    Editor *ed = currentEditor();
    if (!ed) return;
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() != QDialog::Accepted) return;
    ed->document()->print(&printer);
}

void MainWindow::ensureTrailingNewline(Editor *ed) {
    if (!ed) return;
    QString t = ed->toPlainText();
    if (!t.isEmpty() && !t.endsWith('\n')) {
        QTextCursor c(ed->document());
        c.movePosition(QTextCursor::End);
        c.insertText("\n");
    }
}

void MainWindow::showPreferences() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Preferences"));
    QFormLayout *form = new QFormLayout(&dlg);
    // Simple: use QSettings-backed controls via checkboxes/spinboxes
    QCheckBox *cbLineNums = new QCheckBox(&dlg); cbLineNums->setChecked(m_lineNumbers);
    QCheckBox *cbCurLine = new QCheckBox(&dlg); cbCurLine->setChecked(m_highlightCurrentLine);
    QCheckBox *cbBracket = new QCheckBox(&dlg); cbBracket->setChecked(m_bracketMatching);
    QCheckBox *cbMargin = new QCheckBox(&dlg); cbMargin->setChecked(m_rightMargin);
    QSpinBox *spMargin = new QSpinBox(&dlg); spMargin->setRange(1, 500); spMargin->setValue(m_rightMarginColumn);
    QCheckBox *cbWhitespace = new QCheckBox(&dlg); cbWhitespace->setChecked(m_drawWhitespace);
    QCheckBox *cbAutoBracket = new QCheckBox(&dlg); cbAutoBracket->setChecked(m_autoBracketCompletion);
    QCheckBox *cbTrim = new QCheckBox(&dlg); cbTrim->setChecked(m_trimTrailingSpaces);
    QCheckBox *cbSyntax = new QCheckBox(&dlg); cbSyntax->setChecked(m_syntaxHighlighting);
    QCheckBox *cbAutoIndent = new QCheckBox(&dlg); cbAutoIndent->setChecked(m_autoIndent);
    QCheckBox *cbInsertSpaces = new QCheckBox(&dlg); cbInsertSpaces->setChecked(m_insertSpaces);
    QSpinBox *spTabSize = new QSpinBox(&dlg); spTabSize->setRange(1, 16); spTabSize->setValue(m_tabSize);
    QCheckBox *cbRestore = new QCheckBox(&dlg); cbRestore->setChecked(m_restoreCursorPosition);
    QCheckBox *cbTrailing = new QCheckBox(&dlg); cbTrailing->setChecked(m_ensureTrailingNewline);
    QCheckBox *cbAutosave = new QCheckBox(&dlg); cbAutosave->setChecked(m_autosave);
    QSpinBox *spAutosave = new QSpinBox(&dlg); spAutosave->setRange(1, 120); spAutosave->setValue(m_autosaveInterval);
    QCheckBox *cbBackup = new QCheckBox(&dlg); cbBackup->setChecked(m_createBackup);
    QCheckBox *cbCloseLast = new QCheckBox(&dlg); cbCloseLast->setChecked(m_closeLastTab);
    QComboBox *cbSmart = new QComboBox(&dlg);
    cbSmart->addItems({"Disabled", "Before", "After", "Always"});
    cbSmart->setCurrentIndex(m_smartHomeEnd);
    QComboBox *cbLang = new QComboBox(&dlg);
    cbLang->addItems({"System", "English", "Deutsch"});
    if (m_language.startsWith("de")) cbLang->setCurrentIndex(2);
    else if (m_language.startsWith("en")) cbLang->setCurrentIndex(1);
    else cbLang->setCurrentIndex(0);

    form->addRow(tr("Line numbers"), cbLineNums);
    form->addRow(tr("Highlight current line"), cbCurLine);
    form->addRow(tr("Bracket matching"), cbBracket);
    form->addRow(tr("Right margin"), cbMargin);
    form->addRow(tr("Right margin column"), spMargin);
    form->addRow(tr("Draw whitespace"), cbWhitespace);
    form->addRow(tr("Auto bracket completion"), cbAutoBracket);
    form->addRow(tr("Trim trailing spaces on save"), cbTrim);
    form->addRow(tr("Syntax highlighting"), cbSyntax);
    form->addRow(tr("Auto indent"), cbAutoIndent);
    form->addRow(tr("Insert spaces"), cbInsertSpaces);
    form->addRow(tr("Tab size"), spTabSize);
    form->addRow(tr("Restore cursor position"), cbRestore);
    form->addRow(tr("Ensure trailing newline"), cbTrailing);
    form->addRow(tr("Autosave"), cbAutosave);
    form->addRow(tr("Autosave interval (min)"), spAutosave);
    form->addRow(tr("Create backup copy"), cbBackup);
    form->addRow(tr("Close window on last tab"), cbCloseLast);
    form->addRow(tr("Smart Home/End"), cbSmart);
    form->addRow(tr("Language"), cbLang);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        m_lineNumbers = cbLineNums->isChecked();
        m_highlightCurrentLine = cbCurLine->isChecked();
        m_bracketMatching = cbBracket->isChecked();
        m_rightMargin = cbMargin->isChecked();
        m_rightMarginColumn = spMargin->value();
        m_drawWhitespace = cbWhitespace->isChecked();
        m_autoBracketCompletion = cbAutoBracket->isChecked();
        m_trimTrailingSpaces = cbTrim->isChecked();
        m_syntaxHighlighting = cbSyntax->isChecked();
        m_autoIndent = cbAutoIndent->isChecked();
        m_insertSpaces = cbInsertSpaces->isChecked();
        m_tabSize = spTabSize->value();
        m_restoreCursorPosition = cbRestore->isChecked();
        m_ensureTrailingNewline = cbTrailing->isChecked();
        m_autosave = cbAutosave->isChecked();
        m_autosaveInterval = spAutosave->value();
        m_createBackup = cbBackup->isChecked();
        m_closeLastTab = cbCloseLast->isChecked();
        m_smartHomeEnd = cbSmart->currentIndex();
        if (cbLang->currentIndex() == 1) m_language = "en_US";
        else if (cbLang->currentIndex() == 2) m_language = "de_DE";
        else m_language = QLocale::system().name();
        saveSettings();
        m_autosaveTimer->setInterval(m_autosaveInterval * 60000);
        for (int i = 0; i < m_tabs->count(); i++)
            if (auto e = qobject_cast<Editor*>(m_tabs->widget(i))) applySettingsToEditor(e);
        applyTheme();
    }
}

void MainWindow::about() {
    QMessageBox::about(this, tr("About Note"), tr("Note - a lightweight text editor for Omarchy."));
}

void MainWindow::updateTitle() {
    Editor *ed = currentEditor();
    QString name = "Untitled";
    if (ed) {
        QString p = ed->property("filePath").toString();
        name = p.isEmpty() ? "Untitled" : QFileInfo(p).fileName();
        if (ed->document()->isModified()) name += " *";
    }
    setWindowTitle(name + " - Note");
}

void MainWindow::updateStatus() {
    Editor *ed = currentEditor();
    if (!ed) { m_status->setText(""); return; }
    QTextCursor c = ed->textCursor();
    int line = c.blockNumber() + 1;
    int col = c.columnNumber() + 1;
    QString lang = ed->language();
    QString enc = ed->property("encoding").toString();
    if (enc.isEmpty()) enc = "UTF-8";
    m_status->setText(QString("Ln %1, Col %2   |   %3   |   %4").arg(line).arg(col).arg(lang).arg(enc));
}

QString MainWindow::detectLanguage(const QString &path) const {
    QString ext = QFileInfo(path).suffix().toLower();
    for (int i = 0; EXT_LANG[i][0]; i++) {
        if (ext == EXT_LANG[i][0]) return EXT_LANG[i][1];
    }
    return "text";
}

void MainWindow::setEditorLanguage(Editor *ed, const QString &path) {
    if (!ed) return;
    if (!path.isEmpty()) {
        ed->setHighlightDefinition(path);
        QString name = ed->highlightLanguageName();
        ed->setLanguage(name);
    } else {
        ed->setLanguage("text");
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    for (int i = 0; i < m_tabs->count(); i++) {
        Editor *ed = qobject_cast<Editor*>(m_tabs->widget(i));
        if (!maybeSave(ed)) { event->ignore(); return; }
    }
    QSettings gs("omarchy", "note");
    gs.setValue("geometry", saveGeometry());
    gs.setValue("windowState", saveState());
    event->accept();
}
