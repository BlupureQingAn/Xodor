#include "FineTuneManager.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

FineTuneManager::FineTuneManager(QObject *parent)
    : QObject(parent)
{
}

void FineTuneManager::startFineTune(const QString &dataPath, const QString &modelName)
{
    // Ollama微调流程：
    // 1. 准备训练数据（JSONL格式）
    // 2. 创建Modelfile
    // 3. 使用ollama create命令创建模型
    
    emit fineTuneProgress(10);
    
    // 创建Modelfile
    QString modelfile = QString("FROM %1\n").arg(modelName);
    modelfile += "PARAMETER temperature 0.7\n";
    modelfile += "PARAMETER top_p 0.9\n";
    
    QFile file("data/Modelfile");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(modelfile.toUtf8());
        file.close();
    }
    
    emit fineTuneProgress(50);
    
    // 使用ollama create创建模型
    QProcess process;
    process.start("ollama", QStringList() << "create" << "custom-model" << "-f" << "data/Modelfile");
    process.waitForFinished(-1);
    
    if (process.exitCode() == 0) {
        emit fineTuneProgress(100);
        emit fineTuneCompleted("custom-model");
    } else {
        emit fineTuneError(process.readAllStandardError());
    }
}
