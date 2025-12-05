#ifndef QUESTIONBANKMANAGER_H
#define QUESTIONBANKMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

// 题库类型
enum class QuestionBankType {
    Original,    // 原始题库（只读备份）
    Processed,   // 基础题库（解析后）
    Mock         // 模拟题库（AI生成）
};

// 题库信息
struct QuestionBankInfo {
    QString id;              // 唯一ID
    QString name;            // 题库名称
    QString path;            // 内部存储路径
    QString originalPath;    // 原始导入路径
    int questionCount;       // 题目数量
    QDateTime importTime;    // 导入时间
    QDateTime lastAccessTime;// 最后访问时间
    bool isAIParsed;         // 是否AI解析
    QuestionBankType type;   // 题库类型
    
    QString toJson() const;
    static QuestionBankInfo fromJson(const QString &json);
};

// 题库管理器 - 单例模式
class QuestionBankManager : public QObject
{
    Q_OBJECT
public:
    static QuestionBankManager& instance();
    
    // 题库操作
    QString importQuestionBank(const QString &sourcePath, const QString &name, bool isAIParsed = false);
    bool deleteQuestionBank(const QString &bankId);
    bool renameQuestionBank(const QString &bankId, const QString &newName);
    bool updateQuestionCount(const QString &bankId, int count);
    
    // 题库查询
    QVector<QuestionBankInfo> getAllBanks() const;
    QVector<QuestionBankInfo> getBanksByType(QuestionBankType type) const;
    QuestionBankInfo getBankInfo(const QString &bankId) const;
    QString getCurrentBankId() const { return m_currentBankId; }
    
    // 题库切换
    bool switchToBank(const QString &bankId);
    
    // 路径管理
    QString getBankStoragePath(const QString &bankId) const;
    QString getBankPath(const QString &bankId, QuestionBankType type) const;
    QString getInternalStorageRoot() const;
    QString getOriginalBanksRoot() const;
    QString getProcessedBanksRoot() const;
    QString getMockBanksRoot() const;
    
    // 持久化
    void save();
    void load();
    
    // 路径验证和修复
    bool validateAndFixBankPaths();
    
signals:
    void bankListChanged();
    void currentBankChanged(const QString &bankId);
    
private:
    QuestionBankManager(QObject *parent = nullptr);
    ~QuestionBankManager();
    QuestionBankManager(const QuestionBankManager&) = delete;
    QuestionBankManager& operator=(const QuestionBankManager&) = delete;
    
    QString generateBankId() const;
    QString getConfigFilePath() const;
    bool copyDirectory(const QString &source, const QString &destination);
    
    QVector<QuestionBankInfo> m_banks;
    QString m_currentBankId;
};

#endif // QUESTIONBANKMANAGER_H
