#ifndef OPERATIONHISTORY_H
#define OPERATIONHISTORY_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QJsonObject>

// 操作类型
enum class OperationType {
    DeleteQuestion,     // 删除题目
    DeleteBank,         // 删除题库
    CreateQuestion,     // 创建题目
    EditQuestion        // 编辑题目
};

// 操作记录
struct Operation {
    OperationType type;
    QString description;        // 操作描述
    QDateTime timestamp;        // 操作时间
    QJsonObject data;          // 操作数据（用于恢复）
    
    // 删除操作的数据
    QString filePath;          // 被删除的文件/文件夹路径
    QString backupPath;        // 备份路径（回收站）
    QByteArray fileContent;    // 文件内容（用于恢复）
    bool isDirectory;          // 是否是目录
    
    QJsonObject toJson() const;
    static Operation fromJson(const QJsonObject &json);
};

// 操作历史管理器 - 单例模式
class OperationHistory : public QObject
{
    Q_OBJECT
    
public:
    static OperationHistory& instance();
    
    // 记录操作
    void recordDeleteQuestion(const QString &filePath, const QByteArray &content);
    void recordDeleteBank(const QString &bankPath);
    void recordCreateQuestion(const QString &filePath);
    void recordEditQuestion(const QString &filePath, const QByteArray &oldContent);
    
    // 撤销/重做
    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();
    
    // 清空历史
    void clear();
    
    // 获取历史记录
    QVector<Operation> getHistory() const { return m_history; }
    int getCurrentIndex() const { return m_currentIndex; }
    
    // 持久化
    void save();
    void load();
    
signals:
    void historyChanged();
    void operationUndone(const Operation &op);
    void operationRedone(const Operation &op);
    
private:
    OperationHistory(QObject *parent = nullptr);
    ~OperationHistory();
    OperationHistory(const OperationHistory&) = delete;
    OperationHistory& operator=(const OperationHistory&) = delete;
    
    void addOperation(const Operation &op);
    bool restoreFile(const QString &filePath, const QByteArray &content);
    bool restoreDirectory(const QString &dirPath, const QString &backupPath);
    bool deleteFileToRecycleBin(const QString &filePath, QString &backupPath);
    bool deleteDirectoryToRecycleBin(const QString &dirPath, QString &backupPath);
    bool copyDirectory(const QString &source, const QString &destination);
    QString getRecycleBinPath() const;
    QString getHistoryFilePath() const;
    
    QVector<Operation> m_history;
    int m_currentIndex;  // 当前位置（-1 表示没有操作）
    
    static const int MAX_HISTORY_SIZE = 50;  // 最大历史记录数
};

#endif // OPERATIONHISTORY_H
