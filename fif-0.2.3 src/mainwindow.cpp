#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifdef Q_OS_WIN
QString GetWinVer() {
    // http://web3.codeproject.com/Messages/4823136/Re-Another-Way.aspx
    NTSTATUS (WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
    OSVERSIONINFOEXW osInfo;

    *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");
    QString result;
    if (NULL != RtlGetVersion) {
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        RtlGetVersion(&osInfo);
        result = "Windows ";
        if(osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) result += "NT ";
        result += QString::number(osInfo.dwMajorVersion) + "." + QString::number(osInfo.dwMinorVersion);
    }
    return result;
}
#endif

QString GetOS() {
    QString OS;
#ifdef Q_OS_LINUX
    OS = "X11; Linux";
#endif
#ifdef Q_OS_FREEBSD
    OS = "X11; FreeBSD";
#endif
#ifdef Q_OS_WIN
    OS = GetWinVer();
#endif
#ifdef Q_OS_MAC
    OS = "Macintosh; Mac OSX";
#endif
    return OS;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    // Setup window
    ui->setupUi(this);
    this->setWindowTitle(APPNAME);
    // Setup app dir
    appDir = QDir::homePath() + "/.fif/";
    if(!appDir.exists()) appDir.mkdir(appDir.absolutePath());
    // Setup table
    ResetTable();
    // Load History
    LoadHistory();
    // Check for updates
    QTimer::singleShot(200, this, SLOT(checkUpdates()));
    ui->listResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listResults, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(listResultsContextMenu(QPoint)));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::createFileList(QString mask, QString path, bool recurse) {
    DIR *dir;
    struct dirent *ent;
    QString fp = path;
    QStringList masks;
    masks << mask.split(";");
    int i;
    if(!fp.endsWith(QDir::separator())) fp.append(QDir::separator());
    if((dir = opendir(fp.toStdString().c_str())) != NULL) {
      while ((ent = readdir (dir)) != NULL && stop == false) {
        QString filename = ent->d_name;
        QCoreApplication::processEvents();
        if(ent->d_type != DT_DIR) {
            for(i=0;i<masks.count();i++) {
                QFileInfo fi(filename);
                QFileInfo fi2(masks.at(i));
                if(fi.suffix() == fi2.suffix()) {
                    //fileList.append(fp + filename);
                    scanFile(fp + filename);
                }
            }
        }
        if(ent->d_type == DT_DIR && filename != "." && filename != ".." && recurse == true) {
            createFileList(mask,fp + filename,recurse);
        }
      }
      closedir(dir);
    }
}

void MainWindow::ResetTable() {
    QStringList headers;
    headers << "Filename" << "Path" << "Line Number" << "Line Content";
    ui->listResults->setColumnCount(4);
    ui->listResults->setRowCount(0);
    ui->listResults->setHorizontalHeaderLabels(headers);
    ui->listResults->horizontalHeader()->setVisible(true);
    ui->listResults->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::scanFile(QString filename) {
    fileCount++;
    int j,x,c;
    QString line;
    QStringList words;
    QStringList sWords;
    sWords << ui->textSearch->currentText().split(" ");
    QCoreApplication::processEvents();
    QFile f(filename);
    QString contents;
    if(f.open(QIODevice::ReadOnly)) {
        c = 0;
        QTextStream instream(&f);
        while(!f.atEnd()) {
            line = f.readLine();
            c++;
            if(ui->checkWholeWords->isChecked()) {
                /*words.clear();
                words << line.split(" ");
                for(j=0;j < words.count();j++) {
                    for(x=0;x < sWords.count();x++) {
                        if(ui->checkMatchCase->isChecked()) {
                            if(sWords.at(x) == words.at(j)) addFind(c,filename,line,sWords.at(x));
                        }
                        else {
                            if(sWords.at(x).toLower() == words.at(j).toLower()) addFind(c,filename,line,sWords.at(x));
                        }
                    }
                }*/
                if(line.contains(ui->textSearch->currentText())) {
                    addFind(c,filename,line,ui->textSearch->currentText());
                }
            }
            else {
                if(ui->checkMatchCase->isChecked()) {
                    for(x=0;x<sWords.count();x++) {
                        if(line.contains(sWords.at(x),Qt::CaseSensitive)) addFind(c,filename,line,sWords.at(x));
                    }
                }
                else {
                    for(x=0;x<sWords.count();x++) {
                        if(line.contains(sWords.at(x),Qt::CaseInsensitive)) addFind(c,filename,line,sWords.at(x));
                    }
                }
            }
        }
        f.close();
    }
}

void MainWindow::addFind(int line, QString filename, QString content, QString matchedword) {
    matchCount++;
    int row = 0;
    ui->listResults->insertRow(row);
    QTableWidgetItem* l = new QTableWidgetItem;
    QTableWidgetItem* f = new QTableWidgetItem;
    QTableWidgetItem* p = new QTableWidgetItem;
    QFileInfo fi(filename);
    l->setText(QString::number(line));
    f->setText(fi.fileName());
    p->setText(fi.absolutePath());
    QLabel* lbl = new QLabel;
    QString newcontent = content.replace("&","&amp;").replace(">","&gt;").replace("<","&lt;");
    if(ui->checkMatchCase) newcontent.replace(matchedword,"<b>"+matchedword+"</b>",Qt::CaseSensitive);
    else newcontent.replace(matchedword,"<b>"+matchedword+"</b>",Qt::CaseInsensitive);
    lbl->setText(newcontent);
    ui->listResults->setItem(row,0,f);
    ui->listResults->setItem(row,1,p);
    ui->listResults->setItem(row,2,l);
    ui->listResults->setCellWidget(row,3,lbl);
}

void MainWindow::LoadHistory() {
    int c;
    QFile f(appDir.absoluteFilePath("history.txt"));
    QString line;
    QStringList sText;
    QStringList ext;
    if(f.open(QIODevice::ReadOnly)) {
        c = 0;
        while(!f.atEnd()) {
            line = f.readLine();
            if(c == 0) {
                // Line 0 = file extensions
                ext.clear();
                ext << line.trimmed().split(":");
                if(!ext.isEmpty()) ui->textExt->addItems(ext);
            }
            else {
                sText << line.trimmed();
            }
            c++;
        }
        f.close();
        if(!sText.isEmpty()) ui->textSearch->addItems(sText);
    }
}

void MainWindow::SaveHistory() {
    QFile f(appDir.absoluteFilePath("history.txt"));
    if(f.open(QIODevice::WriteOnly)) {
        QStringList ext;
        int i;
        for(i = 0; i < ui->textExt->count();i++) {
            ext << ui->textExt->itemText(i);
        }
        QTextStream outstream(&f);
        outstream << ext.join(":") << "\n";
        for(i = 0; i < ui->textSearch->count();i++) {
            outstream << ui->textSearch->itemText(i) << "\n";
        }
        f.close();
    }
}

void MainWindow::checkUpdates() {
    QString ua;
    ua = "Mozilla/5.0 (compatible; "+GetOS()+"; "+APPNAME+" "+APPVER+" ("+QString::number(CURRVER)+"))";
    QNetworkAccessManager nam;
    QUrl url("http://www.matthewhipkin.co.uk/fif.txt");
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent",QByteArray(ua.toStdString().c_str()));
    QNetworkReply* reply = nam.get(req);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QByteArray bytes = reply->readAll();
    QString t = QString::fromUtf8(bytes);
    if(t.trimmed().toInt() > CURRVER) {
        labelUpdate = new QLabel(this);
        labelUpdate->setText("<p>A new version is available, <a href=\"http://www.matthewhipkin.co.uk\">click here</a> to get it!");
        labelUpdate->setVisible(true);
        labelUpdate->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 rgba(255, 204, 204, 255), stop:1 rgba(255, 255, 255, 255)); }");
        labelUpdate->setTextInteractionFlags(Qt::TextBrowserInteraction);
        labelUpdate->setOpenExternalLinks(true);
        ui->centralWidget->layout()->addWidget(labelUpdate);
    }
}

void MainWindow::on_btnGo_clicked(){
    SetControlsEnabled(false);
    if(ui->btnGo->text() == "Stop") {
        stop = true;
        ui->btnGo->setText("Go");
    }
    else {
        ui->btnGo->setText("Stop");
        stop = false;
        ResetTable();
        ui->statusBar->showMessage("Working...");
        fileCount = 0;
        matchCount = 0;
        createFileList(ui->textExt->currentText(),ui->textDir->text(),ui->checkRecurse->isChecked());
        QString MatchText = QString::number(matchCount) + " match";
        QString FileText = QString::number(fileCount) + " file";
        if(matchCount != 1) MatchText.append("es");
        if(fileCount != 1) FileText.append("s");
        ui->statusBar->showMessage("Done (" + MatchText + " in " + FileText + ")");
        bool addNew = true;
        int i;
        for(i = 0; i < ui->textExt->count(); i++) {
            if(ui->textExt->itemText(i) == ui->textExt->currentText()) addNew = false;
        }
        if(addNew) {
            ui->textExt->addItem(ui->textExt->currentText());
        }
        addNew = true;
        for(i = 0; i < ui->textSearch->count(); i++) {
            if(ui->textSearch->itemText(i) == ui->textSearch->currentText()) addNew = false;
        }
        if(addNew) {
            ui->textSearch->addItem(ui->textSearch->currentText());
        }
        SaveHistory();
        ui->btnGo->setText("Go");
    }
    SetControlsEnabled(true);
}

void MainWindow::on_btnDir_clicked() {
    QString dirname = QFileDialog::getExistingDirectory(this, "Select a Directory", QDir::currentPath());
    if(!dirname.isEmpty()) {
        ui->textDir->setText(dirname);
    }
}

void MainWindow::on_btnAbout_clicked() {
    QString html;
    html = "<p><b style=\"font-size: 14pt\">"+QString(APPNAME)+"</b> "+QString(APPVER)+"<br>\n";
    html.append("&copy;2016 <a href=\"https://www.matthewhipkin.co.uk\" style=\"color: #FF0000\">Matthew Hipkin</a>\n");
    html.append("<p>A simple tool to search multiple files over multiple directories.</p>");
    html.append("<p><a href=\"https://twitter.com/hippy2094\"><img src=\":/images/logo_twitter_25px.png\"></a> <a href=\"https://sourceforge.net/projects/fif/\"><img src=\":/images/sourceforge_logo_small.png\"></a></p>");
    QMessageBox::about(this,"About "+QString(APPNAME),html);
}

void MainWindow::on_btnSave_clicked() {
    QString filename = QFileDialog::getSaveFileName(this, "Save file", "", "*.txt");
    if(filename.isEmpty()) return;
    int row;
    QTableWidgetItem* item;
    if(!filename.endsWith(".txt")) filename.append(".txt");
    QFile f(filename);
    if(f.open(QIODevice::WriteOnly)) {
        QTextStream outstream(&f);
        for(row = 0; row < ui->listResults->rowCount()-1; row++) {
            item = ui->listResults->item(row,0);
            if(item && !item->text().isEmpty()) outstream << "file = " << item->text() << "\n";
            item = ui->listResults->item(row,1);
            if(item && !item->text().isEmpty()) outstream << "path = " << item->text() << "\n";
            item = ui->listResults->item(row,2);
            if(item && !item->text().isEmpty()) outstream << "line = " << item->text() << "\n";
            QLabel *lbl = qobject_cast<QLabel*>(ui->listResults->cellWidget(row, 3));
            QTextDocument doc;
            doc.setHtml(lbl->text());
            QString pt = doc.toPlainText();
            outstream << pt << "\n";
        }
        f.close();
        ui->statusBar->showMessage("Saved " + filename);
    }
}

void MainWindow::on_listResults_cellDoubleClicked(int row, int column) {
    QTableWidgetItem* item;
    item = ui->listResults->item(row,0);
    QString filename;
    filename = item->text();
    item = ui->listResults->item(row,1);
    QString path;
    path = item->text();
    QString fullname;
    fullname = path + "/" + filename;
    int line;
    item = ui->listResults->item(row,2);
    line = item->text().toInt();
    QDialog *frmEditor = new QDialog(this);
    frmEditor->setWindowFlags(Qt::Window);
    frmEditor->resize(400,400);
    frmEditor->show();   
    frmEditor->setWindowTitle("fif ["+fullname+"]");
    QPushButton *btnSaveContents = new QPushButton(frmEditor);
    QPushButton *btnRevertContents = new QPushButton(frmEditor);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    QToolBar *dToolbar = new QToolBar(frmEditor);
    dToolbar->addWidget(btnSaveContents);
    dToolbar->addWidget(btnRevertContents);
    QVBoxLayout *dialogLayout = new QVBoxLayout(frmEditor);
    frmEditor->setLayout(dialogLayout);
    CodeEditor *textEdit = new CodeEditor(this);
    textEdit->setFont(font);
    dialogLayout->addWidget(dToolbar);
    dialogLayout->addWidget(textEdit);
    QFile f(fullname);
    QString content;
    if(f.open(QIODevice::ReadOnly)) {
        content = f.readAll();
        textEdit->document()->setPlainText(content);
        f.close();
        QTextCursor cursor(textEdit->document()->findBlockByLineNumber(line-1));
        textEdit->setTextCursor(cursor);
    }
    frmEditor->exec();
}

void MainWindow::listResultsContextMenu(const QPoint& pos) {
    if(ui->listResults->selectedItems().count() == 0) return;
    QPoint globalPos = ui->listResults->mapToGlobal(pos);
    QMenu actionMenu;
    actionMenu.addAction("Open File");
    actionMenu.addAction("Open Containing Directory");
    QAction* selectedItem = actionMenu.exec(globalPos);
    if(!selectedItem) return;
    if (selectedItem->text() == "Open File") {
        QTableWidgetItem* item;
        item = ui->listResults->selectedItems().at(1);
        QString path;
        path = item->text();
        QString filename;
        item = ui->listResults->selectedItems().at(0);
        filename = item->text();
        QString fullname;
        fullname = path + "/" + filename;
        QDesktopServices::openUrl(QUrl::fromLocalFile(fullname));
    }
    if (selectedItem->text() == "Open Containing Directory") {
        QTableWidgetItem* item;
        // Index 1 is the path
        item = ui->listResults->selectedItems().at(1);
        QString path = QDir::toNativeSeparators(item->text());
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::SetControlsEnabled(bool b) {
    ui->btnAbout->setEnabled(b);
    ui->btnDir->setEnabled(b);
    ui->btnSave->setEnabled(b);
    ui->checkMatchCase->setEnabled(b);
    ui->checkRecurse->setEnabled(b);
    ui->checkWholeWords->setEnabled(b);
    ui->textDir->setEnabled(b);
    ui->textExt->setEnabled(b);
    ui->textSearch->setEnabled(b);
    ui->listResults->setEnabled(b);
}
