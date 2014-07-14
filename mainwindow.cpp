/****************************************************************************
Copyright (C) 2014 Gamoeba Ltd
All rights reserved

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or other
materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jsonitem.h"
#include "glwidget.h"
#include "nodeobject.h"
#include "sceneobject.h"
#include "socketclient.h"

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

Settings MainWindow::settings;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    settings.loadSettings();

    mGLWidget = new GLWidget();

    mGLWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->glWidget->addWidget(mGLWidget);

    mTreeModel = new TreeModel;
    mTableModel = new TableModel(ui->tableView);

    ui->treeView->setHeaderHidden(true);
    ui->treeView->setAlternatingRowColors(true);
    ui->updatePanel->setHidden(true);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->horizontalHeader()->setVisible(false);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->treeView->setAutoScroll(true);

    ui->tableViewUpdate->setModel(new QStandardItemModel());
    ui->tableViewUpdate->setAlternatingRowColors(true);
    ui->tableViewUpdate->horizontalHeader()->setVisible(false);
    ui->tableViewUpdate->verticalHeader()->setVisible(false);

    TableDelegate* delegate = new TableDelegate();
    ui->tableView->setItemDelegateForColumn(1, delegate);
    TableDelegate* delegateUpdate = new TableDelegate();
    ui->tableViewUpdate->setItemDelegateForColumn(2, delegateUpdate);

    QObject::connect(mTableModel->model(), SIGNAL(itemChanged(QStandardItem*)), this, SLOT(tableItemChanged(QStandardItem*)));
    QObject::connect(mGLWidget, SIGNAL(selectedId(int)), this, SLOT(selectedId(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
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
    qDebug() << QDir::tempPath();
    process.execute("unzip", QStringList() << "-o" << "-d" << QDir::tempPath() << fileName);
    process.waitForFinished();

    QString jsonFile = appendPath(QDir::tempPath(), QString("actors.txt"));
    QString screenShotFile = appendPath(QDir::tempPath(), QString("screenshot.png"));
    inputFiles(jsonFile, screenShotFile);
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
    SocketClient client;
    QString json = client.sendCommand(settings.mHostName, settings.mPortNumber.toUInt(), "dump_scene\n");
    mDoc = QJsonDocument::fromJson(json.toUtf8());

    QProcess process;
    QString screenShotFile = appendPath(QDir::tempPath(), QString("screenshot.png"));

    process.execute("bash", QStringList() << "takescreenshot" << screenShotFile);

    inputFiles(QString(""), screenShotFile);
}

void MainWindow::inputFiles(QString jsonFile, QString screenShotFile) {
    if (jsonFile.length()>0 && QFile::exists(jsonFile)) {
        mDoc = readJson(jsonFile);
        QFile::remove(jsonFile);       
    }
    if (QFile::exists(screenShotFile))
    {
        QImage img(screenShotFile);
        QFile::remove(screenShotFile);
        mGLWidget->setScreenShot(img);
    }

    mTreeModel->setTreeData(mDoc);
    ui->treeView->setModel(mTreeModel->model());

    addObjects();


    mGLWidget->repaint();
}

void MainWindow::selectedId(int id)
{
   QModelIndex index = mTreeModel->getIndex(id);
   ui->treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect) ;
   updateTableView(index);
}

void MainWindow::showScreenShot(bool show)
{
    mGLWidget->showScreenShot(show);
}

void MainWindow::addObjects() {
    QJsonObject obj = mDoc.object();
    addNodeObject(obj);
    QJsonValue children = obj.value(QString("children"));
    if (children.isArray()) {
        QJsonArray array = children.toArray();
        addObjects2(array);
    }
}

bool MainWindow::addNodeObject(QJsonObject obj) {
    QJsonValue value = obj.value(settings.mNodeName);
    QJsonValue id = obj.value(settings.mNodeID);
    QJsonValue isVis = obj.value(settings.mNodeVisible);
    bool vis = isVis.toInt() == 1;
    int idVal = (int)id.toInt();

    NodeObject node(obj, settings.mNodePropertiesName);
    if (value == settings.mCameraNodeName) {
        DataObject pm = node.getProperty(settings.mPropProjectionMatrixName);
        mGLWidget->setProjectionMatrix(pm.get4x4Matrix());
        DataObject vm = node.getProperty(settings.mPropViewMatrixName);
        mGLWidget->setViewMatrix(vm.get4x4Matrix());

        double aspectRatio = node.getProperty(settings.mPropAspectRatioName).toDouble();
        mGLWidget->setAspectRatio(aspectRatio);
    } else {

        QMatrix4x4 wm = node.getProperty(settings.mPropNodeWorldMatrixName).get4x4Matrix();
        std::vector<double> size = node.getProperty(settings.mPropNodeSizeName).getVector();
        QVector3D qsize(size[0], size[1], size[2]);
        if (vis) {
            mGLWidget->addObject(idVal, SceneObject(wm, qsize));
        }
    }
    return vis;
}

void MainWindow::addObjects2(QJsonArray array) {
    QJsonArray::iterator iter;
    QMatrix4x4 projectionMatrix, viewMatrix;


    for (iter = array.begin(); iter !=array.end();iter++) {
        QJsonValue val = *iter;
        if (val.isObject()) {
            QJsonObject obj = val.toObject();
            bool vis = addNodeObject(obj);
            QJsonValue children = obj.value(settings.mNodeChildrenName);
            if (vis && children.isArray()) {
                addObjects2(children.toArray());
            }
        }
    }
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
    qDebug() << idval;
    QStandardItem* id = new QStandardItem(QString::number(idval));
    QStandardItem* name = new QStandardItem(nameStr);
    QStandardItem* value = new QStandardItem(valueStr);
    QList<QStandardItem*> items;
    items.append(id);
    items.append(name);
    items.append(value);
    model->appendRow(items);
    ui->updatePanel->setHidden(false);
}

void MainWindow::on_clearButton_clicked()
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->tableViewUpdate->model());
    model->clear();
    ui->updatePanel->setHidden(true);
}
