#ifndef AICONNECTIONCHECKER_H
#define AICONNECTIONCHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

struct AIConnectionStatus {
    bool ollamaAvailable = false;
    bool cloudApiAvailable = false;
    QString ollamaError;
    QString cloudApiError;
    QString ollamaModel;
    QString ollamaVersion;
    QStringList availableModels;  // 所有可用的模型列表
    bool needModelSelection = false;  // 是否需要用户选择模型
};

class AIConnectionChecker : public QObject
{
    Q_OBJECT
public:
    explicit AIConnectionChecker(QObject *parent = nullptr);
    
    // 检查连接
    void checkOllamaConnection(const QString &baseUrl, const QString &model);
    void checkCloudApiConnection(const QString &apiKey, const QString &apiUrl);
    
signals:
    void ollamaCheckCompleted(bool success, const QString &message);
    void cloudApiCheckCompleted(bool success, const QString &message);
    void allChecksCompleted(const AIConnectionStatus &status);
    
private slots:
    void handleOllamaReply(QNetworkReply *reply);
    void handleCloudApiReply(QNetworkReply *reply);
    
private:
    QNetworkAccessManager *m_networkManager;
    AIConnectionStatus m_status;
    int m_pendingChecks;
    QString m_checkingModel;
    
    void checkIfAllCompleted();
};

#endif // AICONNECTIONCHECKER_H
