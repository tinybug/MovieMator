/*
 * Copyright (c) 2012-2016 Meltytech, LLC
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

#ifndef ABSTRACTJOB_H
#define ABSTRACTJOB_H

#include <QProcess>
#include <QModelIndex>
#include <QList>

class QAction;

class AbstractJob : public QProcess
{
    Q_OBJECT
public:
    explicit AbstractJob(const QString& name);

    void setModelIndex(const QModelIndex& index);
    QModelIndex modelIndex() const;
    bool ran() const;
    bool stopped() const;
    bool finishedNormally() const;
    void appendToLog(const QString&);
    QString log() const;
    QString label() const { return m_label; }
    void setLabel(const QString& label);
    QList<QAction*> standardActions() const { return m_standardActions; }
    QList<QAction*> successActions() const { return m_successActions; }

public slots:
    virtual void start();
    virtual void stop();

signals:
    void progressUpdated(QModelIndex index, uint percent);
    void finished(AbstractJob* job, bool isSuccess);

protected:
    QList<QAction*> m_standardActions;
    QList<QAction*> m_successActions;
    QModelIndex m_index;

protected slots:
    virtual void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void onReadyRead();

private:
    bool m_ran;
    bool m_killed;
    bool m_jobFinishedNormally;
    QString m_log;
    QString m_label;
};

#endif // ABSTRACTJOB_H
