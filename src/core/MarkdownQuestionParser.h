#ifndef MARKDOWNQUESTIONPARSER_H
#define MARKDOWNQUESTIONPARSER_H

#include "Question.h"
#include <QString>
#include <QJsonObject>
#include <QVector>

/**
 * @brief Markdown题目解析器
 * 
 * 负责解析带Front Matter的Markdown文件，提取题目信息
 * 支持YAML格式的Front Matter和标准Markdown格式的题目内容
 */
class MarkdownQuestionParser
{
public:
    /**
     * @brief 从MD文件解析Question对象
     * @param filePath MD文件路径
     * @return Question对象
     */
    static Question parseFromFile(const QString &filePath);
    
    /**
     * @brief 从MD内容解析Question对象
     * @param content MD文件内容
     * @return Question对象
     */
    static Question parseFromContent(const QString &content);
    
    /**
     * @brief 提取Front Matter元数据
     * @param content MD文件内容
     * @return JSON格式的元数据
     */
    static QJsonObject extractFrontMatter(const QString &content);
    
    /**
     * @brief 解析测试用例
     * @param content MD文件内容（不含Front Matter）
     * @return 测试用例列表
     */
    static QVector<TestCase> parseTestCases(const QString &content);
    
    /**
     * @brief 生成带Front Matter的MD内容
     * @param question Question对象
     * @return MD格式的完整内容
     */
    static QString generateMarkdown(const Question &question);
    
    /**
     * @brief 移除Front Matter，返回纯MD内容
     * @param content 完整MD内容
     * @return 不含Front Matter的MD内容
     */
    static QString removeFrontMatter(const QString &content);
    
private:
    /**
     * @brief 解析YAML格式的Front Matter
     * @param yamlContent YAML内容
     * @return JSON对象
     */
    static QJsonObject parseYamlFrontMatter(const QString &yamlContent);
    
    /**
     * @brief 生成YAML格式的Front Matter
     * @param metadata 元数据JSON对象
     * @return YAML字符串
     */
    static QString generateYamlFrontMatter(const QJsonObject &metadata);
};

#endif // MARKDOWNQUESTIONPARSER_H
