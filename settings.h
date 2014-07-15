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


#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings
{
public:
    Settings();
    void loadSettings(QString fileName);
    QString updateSetting(QSettings &settings, QString settingName, QString defaultValue);

    QString mNodeName;
    QString mNodeID;
    QString mNodeVisible;
    QString mNodeChildrenName;
    QString mNodePropertiesName;

    QString mCameraNodeName;
    QString mPropProjectionMatrixName;
    QString mPropViewMatrixName;
    QString mPropAspectRatioName;

    QString mPropNodeWorldMatrixName;
    QString mPropNodeSizeName;
    QString mHostName;
    QString mPortNumber;
    QString mCmdGetScene;
    QString mCmdSetProperties;
    QString mAdbForwardPort;
};

#endif // SETTINGS_H
