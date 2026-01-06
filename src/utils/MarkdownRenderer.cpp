#include "MarkdownRenderer.h"
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

QString MarkdownRenderer::toHtml(const QString &markdown, bool darkMode)
{
    if (markdown.isEmpty()) {
        return QString();
    }
    
    QString result = markdown;
    
    // 1. 先处理代码块（避免被其他规则影响）
    QVector<QPair<int, int>> codeBlockPositions;
    QStringList codeBlockReplacements;
    
    QRegularExpression codeBlockRegex("```([^\\n]*)\\n([\\s\\S]*?)```");
    QRegularExpressionMatchIterator it = codeBlockRegex.globalMatch(result);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString lang = match.captured(1).trimmed();
        QString code = match.captured(2);
        
        // HTML转义
        code.replace("&", "&amp;");
        code.replace("<", "&lt;");
        code.replace(">", "&gt;");
        
        // 应用语法高亮
        if (lang.isEmpty() || lang == "cpp" || lang == "c++" || lang == "c") {
            code = applyCppSyntaxHighlight(code);
        }
        
        QString langLabel = lang.isEmpty() ? "代码" : lang;
        QString bgColor = darkMode ? "#1e1e1e" : "#f5f5f5";
        QString borderColor = darkMode ? "#007acc" : "#0066cc";
        QString headerBg = darkMode ? "#252525" : "#e8e8e8";
        QString textColor = darkMode ? "#d4d4d4" : "#333333";
        
        QString codeHtml = QString(
            "<div style='margin: 12px 0; background-color: %1; "
            "border-left: 3px solid %2; border-radius: 6px; overflow: hidden;'>"
            "<div style='padding: 6px 12px; background-color: %3; "
            "border-bottom: 1px solid %2; font-weight: bold; color: %2;'>%4</div>"
            "<pre style='margin: 0; padding: 12px; background-color: %1; "
            "font-family: Consolas, Monaco, \"Courier New\", monospace; "
            "font-size: 10pt; line-height: 1.5; "
            "white-space: pre-wrap; word-wrap: break-word; color: %5; overflow-x: auto;'>%6</pre>"
            "</div>"
        ).arg(bgColor, borderColor, headerBg, langLabel, textColor, code);
        
        codeBlockPositions.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        codeBlockReplacements.append(codeHtml);
    }
    
    // 从后往前替换代码块 - 使用特殊标记避免被Markdown规则影响
    for (int i = codeBlockPositions.size() - 1; i >= 0; --i) {
        result.replace(codeBlockPositions[i].first, 
                      codeBlockPositions[i].second - codeBlockPositions[i].first, 
                      QString("\x01CODEBLOCK\x02%1\x03").arg(i));
    }
    
    // 2. 处理数学公式
    QVector<QPair<int, int>> mathBlockPositions;
    QStringList mathBlockReplacements;
    
    // 块级数学公式 $$...$$
    QRegularExpression blockMathRegex("\\$\\$([^$]+)\\$\\$");
    QRegularExpressionMatchIterator mathIt = blockMathRegex.globalMatch(result);
    while (mathIt.hasNext()) {
        QRegularExpressionMatch match = mathIt.next();
        QString formula = match.captured(1).trimmed();
        
        // 先转换LaTeX符号（此时反斜杠还是 \ ），再HTML转义
        formula = convertLatexSymbols(formula);
        formula.replace("&", "&amp;");
        formula.replace("<", "&lt;");
        formula.replace(">", "&gt;");
        
        QString mathColor = darkMode ? "#ffd700" : "#d4a017";
        QString mathBg = darkMode ? "#2a2a2a" : "#fffacd";
        
        QString mathHtml = QString(
            "<div style='margin: 12px 0; padding: 12px; background: %1; "
            "border-left: 3px solid %2; border-radius: 6px; text-align: center;'>"
            "<span style='font-family: \"Consolas\", \"Courier New\", \"Cambria Math\", monospace; "
            "font-size: 12pt; color: %2;'>%3</span>"
            "</div>"
        ).arg(mathBg, mathColor, formula);
        
        mathBlockPositions.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        mathBlockReplacements.append(mathHtml);
    }
    
    for (int i = mathBlockPositions.size() - 1; i >= 0; --i) {
        result.replace(mathBlockPositions[i].first, 
                      mathBlockPositions[i].second - mathBlockPositions[i].first, 
                      QString("\x01MATHBLOCK\x02%1\x03").arg(i));
    }
    
    // 行内数学公式 $...$
    QVector<QPair<int, int>> inlineMathPositions;
    QStringList inlineMathReplacements;
    
    QRegularExpression inlineMathRegex("(?<!\\$)\\$([^$\\n]+)\\$(?!\\$)");
    QRegularExpressionMatchIterator inlineMathIt = inlineMathRegex.globalMatch(result);
    while (inlineMathIt.hasNext()) {
        QRegularExpressionMatch match = inlineMathIt.next();
        QString formula = match.captured(1).trimmed();
        
        // 先转换LaTeX符号（此时反斜杠还是 \ ），再HTML转义
        formula = convertLatexSymbols(formula);
        formula.replace("&", "&amp;");
        formula.replace("<", "&lt;");
        formula.replace(">", "&gt;");
        
        QString mathColor = darkMode ? "#ffd700" : "#d4a017";
        QString mathBg = darkMode ? "#2a2a2a" : "#fffacd";
        
        QString mathHtml = QString(
            "<span style='font-family: \"Consolas\", \"Courier New\", \"Cambria Math\", monospace; "
            "color: %1; background: %2; "
            "padding: 2px 6px; border-radius: 3px;'>%3</span>"
        ).arg(mathColor, mathBg, formula);
        
        inlineMathPositions.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        inlineMathReplacements.append(mathHtml);
    }
    
    for (int i = inlineMathPositions.size() - 1; i >= 0; --i) {
        result.replace(inlineMathPositions[i].first, 
                      inlineMathPositions[i].second - inlineMathPositions[i].first, 
                      QString("\x01MATHINLINE\x02%1\x03").arg(i));
    }
    
    // 3. HTML转义（但要保护占位符）
    // 先临时替换占位符为更安全的形式
    // 注意：使用原始字符串字面量来正确表示控制字符
    QString placeholderPattern = QString("%1([A-Z]+)%2(\\d+)%3")
        .arg(QChar(0x01))  // SOH
        .arg(QChar(0x02))  // STX
        .arg(QChar(0x03)); // ETX
    QRegularExpression placeholderRegex(placeholderPattern);
    QVector<QString> placeholders;
    QRegularExpressionMatchIterator phIt = placeholderRegex.globalMatch(result);
    while (phIt.hasNext()) {
        QRegularExpressionMatch match = phIt.next();
        placeholders.append(match.captured(0));
    }
    
    // 用临时标记替换占位符
    for (int i = placeholders.size() - 1; i >= 0; --i) {
        result.replace(placeholders[i], QString("§§PLACEHOLDER%1§§").arg(i));
    }
    
    // 现在可以安全地HTML转义
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    
    // 恢复占位符
    for (int i = 0; i < placeholders.size(); ++i) {
        result.replace(QString("§§PLACEHOLDER%1§§").arg(i), placeholders[i]);
    }
    
    // 4. 处理表格
    result = processTables(result);
    
    // 5. 处理标题
    result = processHeaders(result);
    
    // 6. 处理水平分隔线
    result = processHorizontalRules(result);
    
    // 7. 处理引用块
    result = processBlockquotes(result);
    
    // 8. 处理列表
    result = processLists(result);
    
    // 9. 处理粗体和斜体（占位符已经被保护，不会被误处理）
    result = processBoldItalic(result);
    
    // 10. 处理删除线
    QRegularExpression strikeRegex("~~([^~]+)~~");
    QString strikeColor = darkMode ? "#888888" : "#999999";
    result.replace(strikeRegex, QString("<s style='color: %1;'>\\1</s>").arg(strikeColor));
    
    // 11. 处理行内代码
    QRegularExpression inlineCodeRegex("`([^`]+)`");
    QString codeBg = darkMode ? "#2d2d2d" : "#f0f0f0";
    QString codeColor = darkMode ? "#ff6b6b" : "#c7254e";
    result.replace(inlineCodeRegex, 
                  QString("<code style='background-color: %1; color: %2; "
                         "padding: 2px 6px; border-radius: 3px; "
                         "font-family: Consolas, Monaco, monospace; font-size: 9.5pt;'>\\1</code>")
                  .arg(codeBg, codeColor));
    
    // 12. 处理链接和图片
    result = processLinks(result);
    
    // 13. 处理换行
    result.replace("\n\n", "</p><p style='margin: 8px 0;'>");
    result.replace("\n", "<br>");
    
    // 14. 恢复数学公式 - 使用特殊标记
    for (int i = 0; i < mathBlockReplacements.size(); ++i) {
        result.replace(QString("\x01MATHBLOCK\x02%1\x03").arg(i), mathBlockReplacements[i]);
    }
    for (int i = 0; i < inlineMathReplacements.size(); ++i) {
        result.replace(QString("\x01MATHINLINE\x02%1\x03").arg(i), inlineMathReplacements[i]);
    }
    
    // 15. 恢复代码块 - 使用特殊标记
    for (int i = 0; i < codeBlockReplacements.size(); ++i) {
        result.replace(QString("\x01CODEBLOCK\x02%1\x03").arg(i), codeBlockReplacements[i]);
    }
    
    // 16. 包装在段落中
    if (!result.startsWith("<div") && !result.startsWith("<h") && 
        !result.startsWith("<ul") && !result.startsWith("<ol") &&
        !result.startsWith("<table") && !result.startsWith("<blockquote")) {
        result = "<p style='margin: 8px 0;'>" + result + "</p>";
    }
    
    return result;
}

QString MarkdownRenderer::applyCppSyntaxHighlight(const QString &code)
{
    QString result = code;
    
    // C++关键字
    QStringList keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
        "class", "compl", "concept", "const", "consteval", "constexpr", "constinit",
        "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
        "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit",
        "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline",
        "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
        "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
        "register", "reinterpret_cast", "requires", "return", "short", "signed",
        "sizeof", "static", "static_assert", "static_cast", "struct", "switch",
        "template", "this", "thread_local", "throw", "true", "try", "typedef",
        "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "wchar_t", "while", "xor", "xor_eq",
        "include", "define", "ifdef", "ifndef", "endif", "pragma"
    };
    
    // 高亮关键字
    for (const QString &keyword : keywords) {
        QRegularExpression keywordRegex(QString("\\b(%1)\\b").arg(keyword));
        result.replace(keywordRegex, "<span style='color: #569cd6; font-weight: bold;'>\\1</span>");
    }
    
    // 高亮字符串
    QRegularExpression stringRegex("\"([^\"]*)\"");
    result.replace(stringRegex, "<span style='color: #ce9178;'>\"\\1\"</span>");
    
    // 高亮数字
    QRegularExpression numberRegex("\\b(\\d+\\.?\\d*)\\b");
    result.replace(numberRegex, "<span style='color: #b5cea8;'>\\1</span>");
    
    // 高亮注释
    QRegularExpression commentRegex("//(.*)$");
    result.replace(commentRegex, "<span style='color: #6a9955; font-style: italic;'>//\\1</span>");
    
    return result;
}

QString MarkdownRenderer::convertLatexSymbols(const QString &latex)
{
    QString result = latex;
    
    // 1. 先替换LaTeX符号（按长度从长到短排序，避免部分匹配）
    // 使用QVector保证顺序
    QVector<QPair<QString, QString>> symbols = {
        // 长符号优先
        {"\\Leftrightarrow", "⇔"},
        {"\\Rightarrow", "⇒"},
        {"\\Leftarrow", "⇐"},
        {"\\rightarrow", "→"},
        {"\\leftarrow", "←"},
        {"\\subseteq", "⊆"},
        {"\\supseteq", "⊇"},
        {"\\emptyset", "∅"},
        {"\\epsilon", "ε"},
        {"\\partial", "∂"},
        {"\\approx", "≈"},
        {"\\lambda", "λ"},
        {"\\Lambda", "Λ"},
        {"\\forall", "∀"},
        {"\\exists", "∃"},
        {"\\notin", "∉"},
        {"\\subset", "⊂"},
        {"\\supset", "⊃"},
        {"\\equiv", "≡"},
        {"\\propto", "∝"},
        {"\\infty", "∞"},
        {"\\alpha", "α"},
        {"\\Gamma", "Γ"},
        {"\\gamma", "γ"},
        {"\\Delta", "Δ"},
        {"\\delta", "δ"},
        {"\\Theta", "Θ"},
        {"\\theta", "θ"},
        {"\\kappa", "κ"},
        {"\\Sigma", "Σ"},
        {"\\sigma", "σ"},
        {"\\Omega", "Ω"},
        {"\\omega", "ω"},
        {"\\cdots", "⋯"},  // 必须在 \cdot 之前
        {"\\ldots", "..."},
        {"\\times", "×"},
        {"\\cdot", "·"},
        {"\\dots", "..."},
        {"\\beta", "β"},
        {"\\zeta", "ζ"},
        {"\\iota", "ι"},
        {"\\oint", "∮"},
        {"\\sqrt", "√"},
        {"\\prod", "Π"},
        {"\\frac", "frac"},
        {"\\leq", "≤"},
        {"\\geq", "≥"},
        {"\\neq", "≠"},
        {"\\land", "∧"},
        {"\\eta", "η"},
        {"\\chi", "χ"},
        {"\\psi", "ψ"},
        {"\\Psi", "Ψ"},
        {"\\Phi", "Φ"},
        {"\\phi", "φ"},
        {"\\tau", "τ"},
        {"\\rho", "ρ"},
        {"\\lor", "∨"},
        {"\\cap", "∩"},
        {"\\cup", "∪"},
        {"\\int", "∫"},
        {"\\sum", "Σ"},
        {"\\neg", "¬"},
        {"\\div", "÷"},
        {"\\le", "≤"},
        {"\\ge", "≥"},
        {"\\ne", "≠"},
        {"\\pm", "±"},
        {"\\mp", "∓"},
        {"\\in", "∈"},
        {"\\mu", "μ"},
        {"\\nu", "ν"},
        {"\\xi", "ξ"},
        {"\\Xi", "Ξ"},
        {"\\pi", "π"},
        {"\\Pi", "Π"}
    };
    
    // 按顺序替换
    for (const auto &pair : symbols) {
        result.replace(pair.first, pair.second);
    }
    
    // 2. 然后处理下标和上标
    // Unicode下标字符映射（使用Unicode码点）
    QMap<QChar, QChar> subscriptMap = {
        {'0', QChar(0x2080)}, {'1', QChar(0x2081)}, {'2', QChar(0x2082)}, {'3', QChar(0x2083)}, {'4', QChar(0x2084)},
        {'5', QChar(0x2085)}, {'6', QChar(0x2086)}, {'7', QChar(0x2087)}, {'8', QChar(0x2088)}, {'9', QChar(0x2089)},
        {'a', QChar(0x2090)}, {'e', QChar(0x2091)}, {'i', QChar(0x1D62)}, {'j', QChar(0x2C7C)}, {'o', QChar(0x2092)},
        {'r', QChar(0x1D63)}, {'u', QChar(0x1D64)}, {'v', QChar(0x1D65)}, {'x', QChar(0x2093)},
        {'n', QChar(0x2099)}, {'m', QChar(0x2098)}, {'k', QChar(0x2096)}, {'l', QChar(0x2097)}, {'p', QChar(0x209A)},
        {'s', QChar(0x209B)}, {'t', QChar(0x209C)},
        {'+', QChar(0x208A)}, {'-', QChar(0x208B)}, {'=', QChar(0x208C)}, {'(', QChar(0x208D)}, {')', QChar(0x208E)}
    };
    
    // Unicode上标字符映射（使用Unicode码点）
    QMap<QChar, QChar> superscriptMap = {
        {'0', QChar(0x2070)}, {'1', QChar(0x00B9)}, {'2', QChar(0x00B2)}, {'3', QChar(0x00B3)}, {'4', QChar(0x2074)},
        {'5', QChar(0x2075)}, {'6', QChar(0x2076)}, {'7', QChar(0x2077)}, {'8', QChar(0x2078)}, {'9', QChar(0x2079)},
        {'n', QChar(0x207F)}, {'i', QChar(0x2071)},
        {'+', QChar(0x207A)}, {'-', QChar(0x207B)}, {'=', QChar(0x207C)}, {'(', QChar(0x207D)}, {')', QChar(0x207E)}
    };
    
    // 处理下标 x_{ij} 或 x_i
    QRegularExpression subscriptRegex("([a-zA-Z0-9])_\\{([^}]+)\\}");
    QRegularExpressionMatchIterator subIt = subscriptRegex.globalMatch(result);
    QVector<QPair<int, QPair<QString, QString>>> subscriptReplacements;
    while (subIt.hasNext()) {
        QRegularExpressionMatch match = subIt.next();
        QString base = match.captured(1);
        QString sub = match.captured(2);
        QString converted = base;
        for (QChar c : sub) {
            converted += subscriptMap.value(c, c);
        }
        subscriptReplacements.append(qMakePair(match.capturedStart(), 
                                               qMakePair(match.captured(0), converted)));
    }
    for (int i = subscriptReplacements.size() - 1; i >= 0; --i) {
        result.replace(subscriptReplacements[i].first, 
                      subscriptReplacements[i].second.first.length(),
                      subscriptReplacements[i].second.second);
    }
    
    QRegularExpression subscriptSimpleRegex("([a-zA-Z0-9])_([a-zA-Z0-9])");
    subIt = subscriptSimpleRegex.globalMatch(result);
    subscriptReplacements.clear();
    while (subIt.hasNext()) {
        QRegularExpressionMatch match = subIt.next();
        QString base = match.captured(1);
        QChar sub = match.captured(2)[0];
        QString converted = base + subscriptMap.value(sub, sub);
        subscriptReplacements.append(qMakePair(match.capturedStart(), 
                                               qMakePair(match.captured(0), converted)));
    }
    for (int i = subscriptReplacements.size() - 1; i >= 0; --i) {
        result.replace(subscriptReplacements[i].first, 
                      subscriptReplacements[i].second.first.length(),
                      subscriptReplacements[i].second.second);
    }
    
    // 处理上标 x^{n+1} 或 x^2
    QRegularExpression superscriptRegex("([a-zA-Z0-9])\\^\\{([^}]+)\\}");
    QRegularExpressionMatchIterator supIt = superscriptRegex.globalMatch(result);
    QVector<QPair<int, QPair<QString, QString>>> superscriptReplacements;
    while (supIt.hasNext()) {
        QRegularExpressionMatch match = supIt.next();
        QString base = match.captured(1);
        QString sup = match.captured(2);
        QString converted = base;
        for (QChar c : sup) {
            converted += superscriptMap.value(c, c);
        }
        superscriptReplacements.append(qMakePair(match.capturedStart(), 
                                                qMakePair(match.captured(0), converted)));
    }
    for (int i = superscriptReplacements.size() - 1; i >= 0; --i) {
        result.replace(superscriptReplacements[i].first, 
                      superscriptReplacements[i].second.first.length(),
                      superscriptReplacements[i].second.second);
    }
    
    QRegularExpression superscriptSimpleRegex("([a-zA-Z0-9])\\^([a-zA-Z0-9])");
    supIt = superscriptSimpleRegex.globalMatch(result);
    superscriptReplacements.clear();
    while (supIt.hasNext()) {
        QRegularExpressionMatch match = supIt.next();
        QString base = match.captured(1);
        QChar sup = match.captured(2)[0];
        QString converted = base + superscriptMap.value(sup, sup);
        superscriptReplacements.append(qMakePair(match.capturedStart(), 
                                                qMakePair(match.captured(0), converted)));
    }
    for (int i = superscriptReplacements.size() - 1; i >= 0; --i) {
        result.replace(superscriptReplacements[i].first, 
                      superscriptReplacements[i].second.first.length(),
                      superscriptReplacements[i].second.second);
    }

    
    return result;
}

QString MarkdownRenderer::processHeaders(const QString &text)
{
    QString result = text;
    
    // H1-H6
    QRegularExpression h6Regex("^######\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(h6Regex, "<h6 style='color: #e8e8e8; margin: 8px 0 4px 0;'>\\1</h6>");
    
    QRegularExpression h5Regex("^#####\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(h5Regex, "<h5 style='color: #e8e8e8; margin: 10px 0 5px 0;'>\\1</h5>");
    
    QRegularExpression h4Regex("^####\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(h4Regex, "<h4 style='color: #e8e8e8; margin: 12px 0 6px 0;'>\\1</h4>");
    
    QRegularExpression h3Regex("^###\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(h3Regex, "<h3 style='color: #e8e8e8; margin: 14px 0 7px 0; font-size: 12pt;'>\\1</h3>");
    
    QRegularExpression h2Regex("^##\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(h2Regex, "<h2 style='color: #e8e8e8; margin: 16px 0 8px 0; font-size: 13pt; border-bottom: 2px solid #660000; padding-bottom: 6px;'>\\1</h2>");
    
    QRegularExpression h1Regex("^#\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(h1Regex, "<h1 style='color: #e8e8e8; margin: 18px 0 9px 0; font-size: 14pt; border-bottom: 3px solid #660000; padding-bottom: 8px;'>\\1</h1>");
    
    return result;
}

QString MarkdownRenderer::processBoldItalic(const QString &text)
{
    QString result = text;
    
    // 粗体 **text** 或 __text__
    QRegularExpression boldRegex1("\\*\\*([^\\*]+)\\*\\*");
    result.replace(boldRegex1, "<b>\\1</b>");
    
    QRegularExpression boldRegex2("__([^_]+)__");
    result.replace(boldRegex2, "<b>\\1</b>");
    
    // 斜体 *text* (但不匹配粗体，不匹配占位符)
    // 只处理 *text*，不处理 _text_ 以避免与占位符冲突
    QRegularExpression italicRegex("(?<!\\*)\\*([^\\*\\n]+)\\*(?!\\*)");
    result.replace(italicRegex, "<i>\\1</i>");
    
    // 不再处理 _text_ 形式的斜体，因为会与占位符冲突
    // 如果需要斜体，请使用 *text* 格式
    
    return result;
}

QString MarkdownRenderer::processLists(const QString &text)
{
    QString result = text;
    
    // 有序列表
    QRegularExpression orderedListRegex("^(\\d+)\\.\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(orderedListRegex, 
                  "<div style='margin: 3px 0 3px 16px;'>"
                  "<span style='color: #ffd700; font-weight: bold;'>\\1.</span> \\2"
                  "</div>");
    
    // 无序列表
    QRegularExpression unorderedListRegex("^[-*+]\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(unorderedListRegex, 
                  "<div style='margin: 3px 0 3px 16px;'>"
                  "<span style='color: #ffd700;'>•</span> \\1"
                  "</div>");
    
    return result;
}

QString MarkdownRenderer::processBlockquotes(const QString &text)
{
    QString result = text;
    
    // 引用块
    QRegularExpression blockquoteRegex("^&gt;\\s+(.+)$", QRegularExpression::MultilineOption);
    result.replace(blockquoteRegex, 
                  "<div style='margin: 8px 0; padding: 8px 12px; "
                  "background-color: #2d2d2d; border-left: 4px solid #ffd700; "
                  "border-radius: 4px; color: #b0b0b0; font-style: italic;'>\\1</div>");
    
    return result;
}

QString MarkdownRenderer::processLinks(const QString &text)
{
    QString result = text;
    
    // 图片 ![alt](url)
    QRegularExpression imageRegex("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)");
    result.replace(imageRegex, 
                  "<img src='\\2' alt='\\1' style='max-width: 100%; border-radius: 6px; margin: 8px 0;' />");
    
    // 链接 [text](url)
    QRegularExpression linkRegex("\\[([^\\]]+)\\]\\(([^\\)]+)\\)");
    result.replace(linkRegex, 
                  "<a href='\\2' style='color: #4a9eff; text-decoration: none;'>\\1</a>");
    
    return result;
}

QString MarkdownRenderer::processTables(const QString &text)
{
    // 简单的表格支持（Markdown表格格式）
    // 暂时返回原文本，完整实现较复杂
    return text;
}

QString MarkdownRenderer::processHorizontalRules(const QString &text)
{
    QString result = text;
    
    // 水平分隔线 --- 或 *** 或 ___
    QRegularExpression hrRegex("^(---|\\*\\*\\*|___)$", QRegularExpression::MultilineOption);
    result.replace(hrRegex, 
                  "<hr style='border: none; border-top: 2px solid #3a3a3a; margin: 16px 0;' />");
    
    return result;
}
