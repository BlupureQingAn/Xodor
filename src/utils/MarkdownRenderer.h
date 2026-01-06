#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QString>

/**
 * @brief Markdown渲染工具类
 * 
 * 提供统一的Markdown到HTML转换功能，支持：
 * - 代码块（带语法高亮）
 * - 行内代码
 * - 标题（h1-h6）
 * - 粗体、斜体、删除线
 * - 有序列表、无序列表
 * - 引用块
 * - 链接、图片
 * - 表格
 * - 数学公式（LaTeX）
 * - 水平分隔线
 */
class MarkdownRenderer
{
public:
    /**
     * @brief 将Markdown转换为HTML
     * @param markdown Markdown文本
     * @param darkMode 是否使用暗色主题（默认true）
     * @return HTML文本
     */
    static QString toHtml(const QString &markdown, bool darkMode = true);
    
    /**
     * @brief 应用C++语法高亮
     * @param code 代码文本
     * @return 带HTML标签的高亮代码
     */
    static QString applyCppSyntaxHighlight(const QString &code);
    
    /**
     * @brief 转换LaTeX数学符号为Unicode
     * @param latex LaTeX文本
     * @return 转换后的文本
     */
    static QString convertLatexSymbols(const QString &latex);
    
private:
    // 处理各种Markdown元素
    static QString processCodeBlocks(QString &text);
    static QString processInlineCode(const QString &text);
    static QString processHeaders(const QString &text);
    static QString processBoldItalic(const QString &text);
    static QString processLists(const QString &text);
    static QString processBlockquotes(const QString &text);
    static QString processLinks(const QString &text);
    static QString processTables(const QString &text);
    static QString processMathFormulas(const QString &text);
    static QString processHorizontalRules(const QString &text);
};

#endif // MARKDOWNRENDERER_H
