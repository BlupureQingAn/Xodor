#ifndef CLOUDAICLIENT_H
#define CLOUDAICLIENT_H

#include "AIService.h"
#include <QNetworkAccessManager>

class CloudAIClient : public AIService
{
    Q_OBJECT
public:
    explicit CloudAIClient(QObject *parent = nullptr);
    
    void setApiKey(const QString &key);
    void setApiUrl(const QString &url);
    
    void analyzeCode(const QString &questionDesc, const QString &code) override;
    void generateQuestions(const QJsonObject &params) override;
    void parseQuestionBank(const QStringList &mdFiles) override;
    
private:
    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_apiUrl;
};

#endif // CLOUDAICLIENT_H
