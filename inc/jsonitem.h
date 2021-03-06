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

#ifndef JSONITEM_H
#define JSONITEM_H

#include <QStandardItem>
#include <QJsonObject>

class JsonItem : public QStandardItem
{
public:
    static const int JsonRole = Qt::UserRole +1;
    static const int IdRole = Qt::UserRole +2;
    explicit JsonItem(QJsonObject &object, bool overallVisible);
    QVariant data(int role) const;

signals:

public slots:


private:
    QJsonObject mObject;
    QString mDisplayName;
    bool mOverallVisible;
    bool mVisible;
    int mId;
};

#endif // JSONITEM_H
