/*
 * Copyright (c) 2013-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILTER_H
#define FILTER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QRectF>
#include <MltFilter.h>
#include "qmlmetadata.h"
#include <QVector>

class AbstractJob;
class AbstractTask;

struct key_frame_item
{
   double keyFrame;
   QMap<QString, QString> paraMap;
};

class QmlFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isNew READ isNew)
    Q_PROPERTY(QString path READ path)
    Q_PROPERTY(QStringList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(int producerIn READ producerIn)
    Q_PROPERTY(int producerOut READ producerOut)
    Q_PROPERTY(double producerAspect READ producerAspect)

public:
    explicit QmlFilter(Mlt::Filter* mltFilter, /*const*/ QmlMetadata* metadata, QObject *parent = 0);
    ~QmlFilter();

    bool isNew() const { return m_isNew; }
    void setIsNew(bool isNew) { m_isNew = isNew; };

    Q_INVOKABLE QString get(QString name);
    Q_INVOKABLE double getDouble(QString name);
    Q_INVOKABLE QRectF getRect(QString name);
    Q_INVOKABLE void set(QString name, QString value);
    Q_INVOKABLE void set(QString name, double value);
    Q_INVOKABLE void set(QString name, int value);
    Q_INVOKABLE void set(QString name, double x, double y, double width, double height, double opacity = 1.0);
    QString path() const { return m_path; }
    Q_INVOKABLE void loadPresets();
    QStringList presets() const { return m_presets; }
    /// returns the index of the new preset
    Q_INVOKABLE int  savePreset(const QStringList& propertyNames, const QString& name = QString());
    Q_INVOKABLE void deletePreset(const QString& name);
    Q_INVOKABLE void analyze(bool isAudio = false);
    Q_INVOKABLE static int framesFromTime(const QString& time);
    Q_INVOKABLE static QString timeFromFrames(int frames);
    Q_INVOKABLE void getHash();
    int producerIn() const;
    int producerOut() const;
    double producerAspect() const;
    Q_INVOKABLE void anim_set(QString name, QString value);

    //function to set or get key frame para value
#ifdef MOVIEMATOR_PRO
    Q_INVOKABLE void setKeyFrameParaValue(double frame, QString key, QString value);
    Q_INVOKABLE void removeKeyFrameParaValue(double frame);
    Q_INVOKABLE QString getKeyFrameParaValue(double frame, QString key);
    Q_INVOKABLE double getKeyFrameParaDoubleValue(double frame, QString key);
    Q_INVOKABLE double getPreKeyFrameNum(double currentKeyFrame);
    Q_INVOKABLE double getNextKeyFrameNum(double currentKeyFrame);
    Q_INVOKABLE void combineAllKeyFramePara();
    Q_INVOKABLE bool bKeyFrame(double frame);
    Q_INVOKABLE bool bHasPreKeyFrame(double frame);
    Q_INVOKABLE bool bHasNextKeyFrame(double frame);
    Q_INVOKABLE int  getKeyFrameNumber();
    Q_INVOKABLE int  getKeyFrame(int index);
    Q_INVOKABLE int getKeyFrameCountOnProject(QString name);
    Q_INVOKABLE QString getStringKeyValueOnProjectOnIndex(int index, QString name);
    Q_INVOKABLE int getKeyFrameOnProjectOnIndex(int index, QString name);
    Q_INVOKABLE float getKeyValueOnProjectOnIndex(int index, QString name);

#endif



public slots:
    void preset(const QString& name);


signals:
    void presetsChanged();
    void analyzeFinished(bool isSuccess);
    void changed(); /// Use to let UI and VUI QML signal updates to each other.

#ifdef MOVIEMATOR_PRO
    void addKeyFrame();
    void removeKeyFrame();
    void keyFrameChanged();
#endif

private:
    QmlMetadata* m_metadata;
    Mlt::Filter* m_filter;
    QString m_path;
    bool m_isNew;
    QStringList m_presets;
    
    QString objectNameOrService();


 //   QMap<QString, QString> m_keyFrameParaMap;

//     struct key_frame_item
//    {
//        double keyFrame;
//        QMap<QString, QString> paraMap;
//    };

    QVector<key_frame_item> m_keyFrameList;
};

class AnalyzeDelegate : public QObject
{
    Q_OBJECT
public:
    explicit AnalyzeDelegate(Mlt::Filter *filter);

public slots:
    void onAnalyzeFinished(AbstractTask *task, bool isSuccess);

private:
    Mlt::Filter m_filter;
};

#endif // FILTER_H