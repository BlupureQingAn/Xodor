#ifndef IMPORTRULEMANAGER_H
#define IMPORTRULEMANAGER_H

#include <QString>
#include <QJsonObject>

/**
 * @brief 导入规则管理器
 * 
 * 统一管理题库导入规则文件，规则文件存储在 data/config/ 目录
 * 文件命名格式：{bankName}_parse_rule.json
 */
class ImportRuleManager
{
public:
    /**
     * @brief 保存导入规则到config目录
     * @param bankName 题库名称
     * @param rule 规则JSON对象
     * @return 是否保存成功
     */
    static bool saveImportRule(const QString &bankName, const QJsonObject &rule);
    
    /**
     * @brief 从config目录读取导入规则
     * @param bankName 题库名称
     * @return 规则JSON对象，如果不存在返回空对象
     */
    static QJsonObject loadImportRule(const QString &bankName);
    
    /**
     * @brief 检查规则文件是否存在
     * @param bankName 题库名称
     * @return 规则文件是否存在
     */
    static bool hasImportRule(const QString &bankName);
    
    /**
     * @brief 删除规则文件
     * @param bankName 题库名称
     * @return 是否删除成功
     */
    static bool deleteImportRule(const QString &bankName);
    
    /**
     * @brief 获取规则文件路径
     * @param bankName 题库名称
     * @return 规则文件的完整路径
     */
    static QString getRulePath(const QString &bankName);
    
private:
    static const QString CONFIG_DIR;  // "data/config"
    
    // 私有构造函数，防止实例化
    ImportRuleManager() = delete;
};

#endif // IMPORTRULEMANAGER_H
