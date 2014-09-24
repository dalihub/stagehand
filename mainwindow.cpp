/*
 * Copyright (c) 2014 Gamoeba Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jsonitem.h"
#include "glwidget.h"
#include "nodeobject.h"
#include "sceneobject.h"
#include "socketclient.h"
#include "treemodel.h"

#include <QStandardItemModel>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTableWidget>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QDesktopWidget>

Settings MainWindow::settings;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QDesktopWidget desktop;
    QRect screenSize = desktop.availableGeometry(this);
    QSize sz(screenSize.width() * 0.8f, screenSize.height() * 0.8f);

    ui->setupUi(this);

    mGLWidget = new GLWidget();

    mGLWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->glWidget->addWidget(mGLWidget);

    mTableModel = new TableModel(ui->tableView);

    ui->treeView->setAlternatingRowColors(false);
    ui->updatePanel->setHidden(true);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->horizontalHeader()->setVisible(false);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->treeView->header()->setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
    ui->treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeView->header()->setStretchLastSection(false);
    ui->treeView->setHeaderHidden(true);

    ui->tableViewUpdate->setModel(new QStandardItemModel());
    ui->tableViewUpdate->setAlternatingRowColors(true);
    ui->tableViewUpdate->horizontalHeader()->setVisible(false);
    ui->tableViewUpdate->verticalHeader()->setVisible(false);

    TableDelegate* delegate = new TableDelegate(1);
    ui->tableView->setItemDelegateForColumn(1, delegate);
    TableDelegate* delegateUpdate = new TableDelegate(2);
    ui->tableViewUpdate->setItemDelegateForColumn(2 ,delegateUpdate);

    QObject::connect(mTableModel->model(), SIGNAL(itemChanged(QStandardItem*)), this, SLOT(tableItemChanged(QStandardItem*)));
    QObject::connect(mGLWidget, SIGNAL(selectedId(int)), this, SLOT(selectedId(int)));
    QObject::connect(mGLWidget, SIGNAL(mousePosition(int,int)), this, SLOT(mousePositionChanged(int,int)));
    QObject::connect(&mRecordThread, SIGNAL(frameAvailable()), this, SLOT(newRecordedFrame()));

    mCurrentFrameIndex = 0;

    mSBLabel = new QLabel();
    ui->statusBar->addWidget(mSBLabel);
    resize(sz);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete mGLWidget;
    //delete mTreeModel;
    delete mTableModel;
}

void MainWindow::setAppNameAndDirectory(QString appName, QDir directory)
{
    setWindowTitle(appName);
    QString settingsFile = directory.filePath(appName+".ini");
    settings.loadSettings(settingsFile);

    QFont font;
    int pointSize = settings.mFontPointSize.toInt();
    font.setPointSize(pointSize);
    ui->tableView->setFont(font);
    ui->tableViewUpdate->setFont(font);
    ui->treeView->setFont(font);

}

void MainWindow::setHostName(QString hostName)
{
    settings.mHostName = hostName;
}

void MainWindow::setPortNumber(QString portNumber)
{
    settings.mPortNumber = portNumber;
}

QString appendPath(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

void MainWindow::loadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                     "",
                                                     tr("Files (*.zip)"),NULL,
                                                     QFileDialog::DontUseNativeDialog);

    QProcess process;
    if (!fileName.isEmpty()) {
        process.execute("unzip", QStringList() << "-o" << "-d" << QDir::tempPath() << fileName);
        process.waitForFinished();

        QString jsonFile = appendPath(QDir::tempPath(), QString("actors.txt"));
        QString screenShotFile = appendPath(QDir::tempPath(), QString("screenshot.png"));
        inputFiles(jsonFile, screenShotFile);
    }
}

void MainWindow::saveFile()
{

}

void MainWindow::zoomIn()
{
    mGLWidget->zoomIn();
}

void MainWindow::zoomOut()
{
    mGLWidget->zoomOut();
}

void MainWindow::updateScene()
{
    adbForward();
    SocketClient client;
    QString json = client.sendCommandSizedReturn(settings.mHostName, settings.mPortNumber.toUInt(), settings.mCmdGetScene + "\n");

    if (json.length() > 0 ) {
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        Frame* frame = new Frame(doc);
        mFrames.push_back(frame);

        //QProcess process;
        //QString screenShotFile = appendPath(QDir::tempPath(), QString("screenshot.png"));

        //process.execute("bash", QStringList() << "takescreenshot" << screenShotFile);

        mCurrentFrameIndex = mFrames.size()-1;
        updateFrame();
        //resetSearch();

    } else {
        QMessageBox msgBox;
        msgBox.setText("Error");
        msgBox.setInformativeText("Connection to device failed");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void MainWindow::recordFrame()
{
    adbForward();
    mRecordThread.startRecord();
}

void MainWindow::stopRecord()
{
    mRecordThread.stopRecord();
}

void MainWindow::updateFrame()
{
    Frame* frame = mFrames[mCurrentFrameIndex];
    QStandardItemModel* model = frame->getTreeModel().model();
    ui->treeView->setModel(model);
    ui->treeView->expandAll();
    mGLWidget->setFrame(frame);
    mGLWidget->repaint();
}

void MainWindow::MessageReceived(std::string recv)
{
    Frame* newFrame = NULL;
    if (mFrames.size()>0)
    {
        Frame* currentFrame = mFrames[mFrames.size()-1];
        newFrame = new Frame(*currentFrame);

        newFrame->updateProperties(recv);
        mGLWidget->setFrame(newFrame);
        mFrames.push_back(newFrame);
        mCurrentFrameIndex = mFrames.size()-1;
    }
}

void MainWindow::nextFrame()
{
    mCurrentFrameIndex++;
    int size = mFrames.size();
    if (mCurrentFrameIndex >= size)
    {
        mCurrentFrameIndex = size-1;
    }
    updateFrame();
}

void MainWindow::prevFrame()
{
    mCurrentFrameIndex--;
    if (mCurrentFrameIndex < 0)
    {
        mCurrentFrameIndex = 0;
    }
    updateFrame();
}

void MainWindow::inputFiles(QString jsonFile, QString screenShotFile) {
    Frame* frame = NULL;
    if (jsonFile.length()>0 && QFile::exists(jsonFile)) {
        frame = new Frame(readJson(jsonFile));
        mFrames.push_back(frame);
        QFile::remove(jsonFile);       
    }
/*    if (QFile::exists(screenShotFile))
    {
        QImage img(screenShotFile);
        QFile::remove(screenShotFile);
        mGLWidget->setScreenShot(img);
    }
*/
    frame = mFrames[mCurrentFrameIndex];
    if (frame )
    {
        QStandardItemModel* model = frame->getTreeModel().model();
        ui->treeView->setModel(model);
        ui->treeView->expandAll();
        mFrames.push_back(frame);
        mGLWidget->repaint();
        //resetSearch();
    }
}

void MainWindow::selectedId(int id)
{
//   QModelIndex index = mTreeModel->getIndex(id);
//   ui->treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect) ;
//   updateTableView(index);
}

void MainWindow::mousePositionChanged(int x, int y)
{
    QString lb = "Cursor Pos: ";
    if (x == -1 && y == -1) {
        lb += "Offscreen";
    } else {
        lb += "( %1 , %2 )";
        lb = lb.arg(QString::number(x),QString::number(y));
    }
    mSBLabel->setText(lb);
}

void MainWindow::showScreenShot(bool show)
{
    mGLWidget->showScreenShot(show);
}



QJsonDocument MainWindow::readJson(QString& fileName)
   {
      QString val;
      QFile file;
      file.setFileName(fileName);
      file.open(QIODevice::ReadOnly | QIODevice::Text);
      val = file.readAll();
      file.close();

      QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
      return d;

   }

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    updateTableView(index);
    updateGLView(index);

}

void MainWindow::updateGLView(const QModelIndex &index)
{
    QVariant var = ui->treeView->model()->data(index, JsonItem::JsonRole);
    QJsonObject obj = var.toJsonObject();
    int id = obj.value(settings.mNodeID).toInt();
    mGLWidget->setSelection(id);
}

void MainWindow::updateTableView(const QModelIndex &index)
{
    QVariant var = ui->treeView->model()->data(index, JsonItem::JsonRole);
    QJsonObject obj = var.toJsonObject();

    //delete mTableModel;
    //mTableModel = new TableModel(ui->tableView);
    mTableModel->setTableData(obj);
    ui->tableView->setModel(mTableModel->model());
    ui->tableView->resizeColumnsToContents();
    ui->tableView->resizeRowsToContents();

}

void MainWindow::tableItemChanged(QStandardItem * item)
{
    QString valueStr = item->data(Qt::DisplayRole).toString();
    QModelIndex ind = item->index();
    QString nameStr =  mTableModel->model()->itemFromIndex(ind.sibling(ind.row(),0))->data(Qt::DisplayRole).toString();
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->tableViewUpdate->model());
    int idval = mTableModel->getObjectId();
    int rows = model->rowCount();
    bool found = false;
    for (int i=0;i<rows &&!found;i++) {

        int currid = model->data(model->index(i,0), Qt::DisplayRole).toInt();
        QString currname = model->data(model->index(i,1), Qt::DisplayRole).toString();
        if (currid==idval && currname == nameStr) {
            found = true;
            //model->setData(model->index(i,2), QVariant(valueStr),Qt::DisplayRole);
            // SetItem force repaint but for some reason setData doesn't?
            QStandardItem* value = new QStandardItem(valueStr);
            model->setItem(i,2, value);
        }
    }
    if (!found) {
        QStandardItem* id = new QStandardItem(QString::number(idval));
        id->setFlags(id->flags() ^ Qt::ItemIsEditable);
        QStandardItem* name = new QStandardItem(nameStr);
        name->setFlags(name->flags() ^ Qt::ItemIsEditable);
        QStandardItem* value = new QStandardItem(valueStr);
        QList<QStandardItem*> items;
        items.append(id);
        items.append(name);
        items.append(value);
        model->appendRow(items);
    }
    ui->updatePanel->setHidden(false);
    ui->tableViewUpdate->resizeColumnsToContents();
    ui->tableViewUpdate->resizeRowsToContents();
}

void MainWindow::on_clearButton_clicked()
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->tableViewUpdate->model());
    model->clear();
    ui->updatePanel->setHidden(true);
}

void MainWindow::on_sendButton_clicked()
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->tableViewUpdate->model());
    int rows = model->rowCount();
    QString command = " |";
    for (int i=0;i<rows;i++) {
        int currid = model->data(model->index(i,0), Qt::DisplayRole).toInt();
        QString currname = model->data(model->index(i,1), Qt::DisplayRole).toString();
        QString currvalue = model->data(model->index(i,2), Qt::DisplayRole).toString();
        command += QString::number(currid) + ";" + currname + ";" + currvalue;
        if (i!= rows -1) {
            command += "|";
        }
    }
    adbForward();
    SocketClient client;
    client.sendCommand(settings.mHostName, settings.mPortNumber.toUInt(), settings.mCmdSetProperties + command);

    // after sending, clear the data
    on_clearButton_clicked();
}

void MainWindow::adbForward() {
    if (settings.mAdbForwardPort.length()>0) {
        QProcess process;
        process.execute("adb", QStringList() << "forward" << "tcp:"+settings.mPortNumber << "tcp:"+settings.mAdbForwardPort);
        process.waitForFinished();
    }
}

void MainWindow::nextSelection() {
    if (mCurrentTreeSearchResults.size()>0) {
        const QModelIndex& index = mCurrentTreeSearchResults[mCurrentTreeSearchIndex];
        ui->treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
        updateTableView(index);
        updateGLView(index);

        mCurrentTreeSearchIndex++;
        mCurrentTreeSearchIndex %= mCurrentTreeSearchResults.size();
    }

}

void MainWindow::on_treeSearch_textEdited(const QString &strSearch)
{
//    if (strSearch.size()>0) {
//        if (mCurrentTreeSearch != strSearch) {
//            mCurrentTreeSearchResults = mTreeModel->search(strSearch);
//            mCurrentTreeSearchIndex = 0;
//            mCurrentTreeSearch = strSearch;
//        }
//        nextSelection();
//    }
}

void MainWindow::resetSearch()
{
//    if (mCurrentTreeSearch.length()>0) {
//        mCurrentTreeSearchResults = mTreeModel->search(mCurrentTreeSearch);
//        mCurrentTreeSearchIndex = 0;
//        nextSelection();
//    } else {
//        mCurrentTreeSearchResults.clear();
//        mCurrentTreeSearchIndex = 0;
//    }
}

void MainWindow::on_treeSearch_editingFinished()
{
    //qDebug() << "edit finished";
    //ui->treeView->selectionModel()->select(;

}

void MainWindow::on_treeSearch_returnPressed()
{
    qDebug() << "return Pressed";
    nextSelection();
}

void MainWindow::newRecordedFrame()
{
    while (mRecordThread.hasMoreFrames())
    {
        std::string frame = mRecordThread.getFrame();
        MessageReceived(frame);
    }
    mGLWidget->repaint();

}
