#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <sstream>

#include "frame.h"
#include "treemodel.h"
#include "mainwindow.h"
#include "nodeobject.h"

Frame::Frame()
{
    mTreeModel = new TreeModel;
    mProjectionMatrixSet = false;
    mViewMatrixSet = false;
    mAspectRatioSet = false;
}

Frame::Frame(QJsonDocument doc)
{
    mDoc = doc;
    mTreeModel = new TreeModel;
    mTreeModel->setTreeData(mDoc);
    addObjects();
}

Frame::Frame(const Frame &f)
   : //mDoc(f.mDoc),
     mObjects(f.mObjects),
     mProjectionMatrix(f.mProjectionMatrix),
     mViewMatrix(f.mViewMatrix),
     mAspectRatio(f.mAspectRatio),
     mProjectionMatrixSet(f.mProjectionMatrixSet),
     mViewMatrixSet(f.mViewMatrixSet),
     mAspectRatioSet(f.mAspectRatioSet)
{
   // mTreeModel = new TreeModel;
    //mTreeModel->setTreeData(mDoc);
//    addObjects();
/*    std::map<int, SceneObject>::const_iterator iter;
    for (iter=f.mObjects.begin();iter!=f.mObjects.end();iter++)
    {
        std::pair<int, SceneObject> obj = *iter;
        mObjects[obj.first] = obj.second;
    }
    */
}


void Frame::updateProperties(std::string proplist)
{
    QTextStream iss(proplist.c_str());
    while (!iss.atEnd())
    {
        QString line = iss.readLine();
        std::string propName, propValue;
        QStringList strList = line.split("|");
        if (strList[0].compare("p")==0 && strList.length()==5)
        {
            int actorId = strList[1].toInt();
            int propId = strList[2].toInt();
            QString propName = strList[3].trimmed();
            SceneObject& so = mObjects[actorId];

            if (!mProjectionMatrixSet && actorId == 2) {
                if (propName.compare("projection-matrix") ==0)
                {
                    DataObject wm( strList[4] );
                    mProjectionMatrix = wm.get4x4Matrix();
                    mProjectionMatrixSet = true;
                }
            }
            if (!mViewMatrixSet && actorId == 2) {

                if (propName.compare("view-matrix") ==0)
                {
                    DataObject wm( strList[4] );
                    mViewMatrix = wm.get4x4Matrix();
                    mViewMatrixSet = true;
                }

            }
            if (!mAspectRatioSet && actorId == 2) {

                if (propName.compare("aspect-ratio") ==0)
                {
                    mAspectRatio = strList[4].toDouble();
                    mAspectRatioSet = true;
                }

            }
            if (propName.compare("world-matrix") ==0)
            {
                DataObject wm( strList[4] );
                so.mWorldMatrix = wm.get4x4Matrix();
            }
            else if (propName.compare("size")==0)
            {
                DataObject sz( strList[4] );
                std::vector<double> size = sz.getVector();
                QVector3D qsize(size[0], size[1], size[2]);
                so.mSize = qsize;
            }
            else if (propName.compare("resourceID") == 0)
            {
                int resourceID = strList[4].toInt();
                so.mResourceID = resourceID;
            }
            else if (propId.compare("name") == 0)
            {
                so.mName = strList[4];
            }
            else if (propName.compare("children")) {
                DataObject children( strList[4] );
                std::vector<double> childlist = children.getVector();
                std::vector<double>::iterator it;
                for (it=childlist.begin();it!=childlist.end();++it)
                {
                    so.mChildren.push_back((int)*it);
                }
            }
        } else if (strList[0].compare("d")==0 && strList.length()==2){
            int deleteActor = strList[1].toInt();
            std::map<int, SceneObject>::iterator finditer = mObjects.find(deleteActor);
            if (finditer != mObjects.end()) {
                mObjects.erase(finditer);
            }
        }
    }
}

TreeModel &Frame::getTreeModel()
{
    return *mTreeModel;
}

std::map<int, SceneObject>& Frame::getSceneObjects()
{
    return mObjects;
}

void Frame::addObjects() {
    QJsonObject obj = mDoc.object();
    addNodeObject(obj);
    QJsonValue children = obj.value(MainWindow::settings.mNodeChildrenName);
    if (children.isArray()) {
        QJsonArray array = children.toArray();
        addObjects2(array);
    }
}

bool Frame::addNodeObject(QJsonObject obj) {
    QJsonValue value = obj.value(MainWindow::settings.mNodeName);
    QJsonValue id = obj.value(MainWindow::settings.mNodeID);
    int idVal = (int)id.toInt();

    NodeObject node(obj, MainWindow::settings.mNodePropertiesName);
    bool vis = node.getProperty(MainWindow::settings.mNodeVisible).toInt() == 1;

    if (value == MainWindow::settings.mCameraNodeName) {

        DataObject pm = node.getProperty(MainWindow::settings.mPropProjectionMatrixName);
        mProjectionMatrix = pm.get4x4Matrix();
        DataObject vm = node.getProperty(MainWindow::settings.mPropViewMatrixName);
        mViewMatrix = vm.get4x4Matrix();

        double aspectRatio = node.getProperty(MainWindow::settings.mPropAspectRatioName).toDouble();
        mAspectRatio = aspectRatio;
    } else {

        QMatrix4x4 wm = node.getProperty(MainWindow::settings.mPropNodeWorldMatrixName).get4x4Matrix();
        std::vector<double> size = node.getProperty(MainWindow::settings.mPropNodeSizeName).getVector();
        QVector3D qsize(size[0], size[1], size[2]);
        if (true) {
            mObjects[idVal] = SceneObject(wm, qsize);
        }
    }
    return vis;
}

void Frame::addObjects2(QJsonArray array) {
    QJsonArray::iterator iter;
    QMatrix4x4 projectionMatrix, viewMatrix;

    for (iter = array.begin(); iter !=array.end();iter++) {
        QJsonValue val = *iter;
        if (val.isObject()) {
            QJsonObject obj = val.toObject();
            bool vis = addNodeObject(obj);
            QJsonValue children = obj.value(MainWindow::settings.mNodeChildrenName);
            if (vis && children.isArray()) {
                addObjects2(children.toArray());
            }
        }
    }
}
