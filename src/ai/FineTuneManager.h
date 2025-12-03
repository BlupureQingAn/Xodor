#ifndef FINETUNEMANAGER_H
#define FINETUNEMANAGER_H

#include <QObject>
#include <QString>

class FineTuneManager : public QObject
{
    Q_OBJECT
public:
    explicit FineTuneManager(QObject *parent = nullptr);
    
    void startFineTune(const QString &dataPath, const QString &modelName);
    
signals:
    void fineTuneProgress(int percentage);
    void fineTuneCompleted(const QString &modelPath);
    void fineTuneError(const QString &error);
    
private:
    // Ollama微调相关
};

#endif // FINETUNEMANAGER_H
