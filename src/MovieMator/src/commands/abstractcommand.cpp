#include "abstractcommand.h"

#include <Mlt.h>
#include <Logger.h>
#include <mltcontroller.h>
#include <QDir>
#include <QDateTime>
#include "util.h"
#include <QDomDocument>
#include "mainwindow.h"
#include "docks/timelinedock.h"

bool g_isInUndoRedoProcess = false;

void saveXmlFile(QString original,QString currrent,QString commandName)
{   
    QDomDocument doc;
    if(doc.setContent(original)){
        original = doc.toString();
    }
    if(doc.setContent(currrent)){
        currrent = doc.toString();
    }

    QString folderName = commandName;

    QDateTime now = QDateTime::currentDateTime();
    QString strNow =now.toString("MMdd-hhmmss");
    folderName = strNow + folderName;

    if(original != currrent){
        folderName = folderName + "-Error";
    }

    QDir dir = Util::logFolderPath();
    dir.mkdir(folderName);
    dir.cd(folderName);

    QString originalPath = dir.absoluteFilePath("original.mmp");
    QFile originalFile(originalPath);
    originalFile.open(QIODevice::WriteOnly | QIODevice::Text);
    originalFile.write(original.toUtf8());
    originalFile.close();

    QString currentPath = dir.absoluteFilePath("current.mmp");
    QFile currentFile(currentPath);
    currentFile.open(QIODevice::WriteOnly | QIODevice::Text);
    currentFile.write(currrent.toUtf8());
    currentFile.close();

    qDebug()<<"original:";
    qDebug()<<original;
    qDebug()<<"currrent:";
    qDebug()<<currrent;
}

AbstractCommand::AbstractCommand(QUndoCommand * parent) : QUndoCommand (parent)
{
    //Q_ASSERT(g_isInUndoRedoProcess == false); //UpdateClipCommand 创建后并不会立即push，此处添加assert创建UpdateClipCommand时会出错。在pushcommand的时候添加。
}

void AbstractCommand::redo()
{
    g_isInUndoRedoProcess = true;
    m_originalXml = MLT.XML(MAIN.timelineDock()->model()->tractor());

    MLT.consumer()->set_cancelled(1);
    w_enter_critical();
    MLT.consumer()->set_cancelled(0);

    this->redo_impl();
    w_leave_critical();

    g_isInUndoRedoProcess = false;
}

void AbstractCommand::undo()
{
    g_isInUndoRedoProcess = true;
    MLT.consumer()->set_cancelled(1);
    w_enter_critical();
    MLT.consumer()->set_cancelled(0);
    this->undo_impl();
    w_leave_critical();

    m_currentXml = MLT.XML(MAIN.timelineDock()->model()->tractor());
#ifndef NDEBUG
    saveXmlFile(m_originalXml,m_currentXml,text());
//    Q_ASSERT(m_currentXml == m_originalXml);
#endif
    g_isInUndoRedoProcess = false;
}


