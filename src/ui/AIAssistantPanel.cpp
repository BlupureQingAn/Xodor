#include "AIAssistantPanel.h"
#include "ChatBubbleDelegate.h"
#include "ChatHistoryDialog.h"
#include "../ai/OllamaClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStandardItem>

AIAssistantPanel::AIAssistantPanel(OllamaClient *aiClient, QWidget *parent)
    : QWidget(parent)
    , m_aiClient(aiClient)
    , m_hasQuestion(false)
    , m_isReceivingMessage(false)
    , m_currentAssistantItem(nullptr)
    , m_questionCount(0)
    , m_userLevel("beginner")
{
    setupUI();
    
    // 连接流式输出信号
    if (m_aiClient) {
        connect(m_aiClient, &OllamaClient::streamingChunk,
                this, &AIAssistantPanel::onStreamingChunk);
        connect(m_aiClient, &OllamaClient::streamingFinished,
                this, &AIAssistantPanel::onStreamingFinished);
        connect(m_aiClient, &OllamaClient::error,
                this, &AIAssistantPanel::onErrorOccurred);
    }
}

void AIAssistantPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // 标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("🤖 AI 导师", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    m_newChatButton = new QPushButton("🆕 新对话", this);
    m_newChatButton->setToolTip("开始新对话");
    m_newChatButton->setFixedHeight(30);
    
    m_historyButton = new QPushButton("📜", this);
    m_historyButton->setToolTip("查看历史记录");
    m_historyButton->setFixedSize(30, 30);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_newChatButton);
    titleLayout->addWidget(m_historyButton);
    
    mainLayout->addLayout(titleLayout);
    
    // 对话显示区域（使用QListView + 自定义Delegate）
    m_chatListView = new QListView(this);
    m_chatModel = new QStandardItemModel(this);
    m_bubbleDelegate = new ChatBubbleDelegate(this);
    
    m_chatListView->setModel(m_chatModel);
    m_chatListView->setItemDelegate(m_bubbleDelegate);
    m_chatListView->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatListView->setStyleSheet(R"(
        QListView {
            background-color: #1e1e1e;
            border: none;
        }
    )");
    
    mainLayout->addWidget(m_chatListView, 1);
    
    // 快捷按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(4);
    
    m_analyzeButton = new QPushButton("💡 分析代码", this);
    m_hintButton = new QPushButton("💭 思路", this);
    m_conceptButton = new QPushButton("📚 知识点", this);
    
    m_analyzeButton->setToolTip("请AI分析你的代码");
    m_hintButton->setToolTip("获取解题思路");
    m_conceptButton->setToolTip("讲解相关知识点");
    
    buttonLayout->addWidget(m_analyzeButton);
    buttonLayout->addWidget(m_hintButton);
    buttonLayout->addWidget(m_conceptButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 输入区域
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(4);
    
    m_inputField = new QLineEdit(this);
    m_inputField->setPlaceholderText("输入你的问题或想法...");
    m_inputField->setMinimumHeight(35);
    
    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setFixedWidth(60);
    m_sendButton->setMinimumHeight(35);
    
    inputLayout->addWidget(m_inputField);
    inputLayout->addWidget(m_sendButton);
    
    mainLayout->addLayout(inputLayout);
    
    // 连接信号
    connect(m_sendButton, &QPushButton::clicked, this, &AIAssistantPanel::onSendMessage);
    connect(m_inputField, &QLineEdit::returnPressed, this, &AIAssistantPanel::onSendMessage);
    connect(m_analyzeButton, &QPushButton::clicked, this, &AIAssistantPanel::onAnalyzeCode);
    connect(m_hintButton, &QPushButton::clicked, this, &AIAssistantPanel::onGetHint);
    connect(m_conceptButton, &QPushButton::clicked, this, &AIAssistantPanel::onExplainConcept);
    connect(m_newChatButton, &QPushButton::clicked, this, &AIAssistantPanel::onNewChat);
    connect(m_historyButton, &QPushButton::clicked, this, &AIAssistantPanel::onViewHistory);
}

void AIAssistantPanel::setQuestionContext(const Question &question)
{
    m_currentQuestion = question;
    m_hasQuestion = true;
    
    // 加载该题目的对话历史
    loadConversationHistory();
}

void AIAssistantPanel::clearHistory()
{
    m_messages.clear();
    m_chatModel->clear();
    m_questionCount = 0;
    m_currentAssistantItem = nullptr;
}

void AIAssistantPanel::refreshChat()
{
    // 保存当前对话到历史
    if (m_hasQuestion && !m_messages.isEmpty()) {
        saveConversationHistory();
    }
    
    // 清空当前对话
    clearHistory();
}

void AIAssistantPanel::viewHistory()
{
    ChatHistoryDialog dialog(this);
    
    connect(&dialog, &ChatHistoryDialog::conversationSelected,
            this, [this](const QString &questionId) {
        // 加载选中的对话
        loadConversationById(questionId);
    });
    
    dialog.exec();
}

void AIAssistantPanel::setCurrentCode(const QString &code)
{
    m_currentCode = code;
}

void AIAssistantPanel::offerHelp(const QString &message)
{
    // AI主动提供帮助
    startAssistantMessage();
    appendToAssistantMessage(message);
    finishAssistantMessage();
}

void AIAssistantPanel::onSendMessage()
{
    QString message = m_inputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }
    
    // 清空输入框
    m_inputField->clear();
    
    // 显示用户消息
    appendUserMessage(message);
    
    // 发送到AI
    sendChatMessage(message);
}

void AIAssistantPanel::onAnalyzeCode()
{
    if (!m_hasQuestion) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    if (m_currentCode.isEmpty()) {
        QMessageBox::warning(this, "提示", "代码编辑器为空，请先编写代码");
        return;
    }
    
    // 模拟用户点击"分析代码"
    QString message = "请帮我分析一下代码";
    appendUserMessage(message);
    
    // 构建包含代码的消息
    QString fullMessage = QString("我的代码如下：\n```cpp\n%1\n```\n\n请帮我分析一下").arg(m_currentCode);
    sendChatMessage(fullMessage);
}

void AIAssistantPanel::onGetHint()
{
    if (!m_hasQuestion) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    QString message = "我不知道怎么做，能给我一些思路吗？";
    appendUserMessage(message);
    sendChatMessage(message);
}

void AIAssistantPanel::onExplainConcept()
{
    if (!m_hasQuestion) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    QString message = "这道题涉及哪些知识点？能讲解一下吗？";
    appendUserMessage(message);
    sendChatMessage(message);
}

void AIAssistantPanel::onStreamingChunk(const QString &chunk)
{
    if (!m_isReceivingMessage) {
        startAssistantMessage();
    }
    
    appendToAssistantMessage(chunk);
}

void AIAssistantPanel::onStreamingFinished()
{
    if (m_isReceivingMessage) {
        finishAssistantMessage();
    }
}

void AIAssistantPanel::onErrorOccurred(const QString &error)
{
    // 创建错误消息item
    QStandardItem *item = new QStandardItem();
    item->setData("system", Qt::UserRole);
    item->setData(QString("❌ 错误: %1").arg(error), Qt::DisplayRole);
    item->setData(QDateTime::currentDateTime().toString("hh:mm"), Qt::UserRole + 1);
    item->setEditable(false);
    
    m_chatModel->appendRow(item);
    m_chatListView->scrollToBottom();
    
    if (m_isReceivingMessage) {
        m_isReceivingMessage = false;
        m_currentAssistantItem = nullptr;
    }
}

void AIAssistantPanel::onNewChat()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("新对话");
    msgBox.setText("确定要开始新对话吗？");
    msgBox.setInformativeText("当前对话将被保存到历史记录。");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    
    if (msgBox.exec() == QMessageBox::Yes) {
        refreshChat();
    }
}

void AIAssistantPanel::onViewHistory()
{
    viewHistory();
}

void AIAssistantPanel::appendUserMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
    
    // 创建用户消息item
    QStandardItem *item = new QStandardItem();
    item->setData("user", Qt::UserRole);  // 角色
    item->setData(message, Qt::DisplayRole);  // 内容
    item->setData(timestamp, Qt::UserRole + 1);  // 时间戳
    item->setEditable(false);
    
    m_chatModel->appendRow(item);
    
    // 滚动到底部
    m_chatListView->scrollToBottom();
    
    // 保存到历史
    ChatMessage msg;
    msg.role = "user";
    msg.content = message;
    msg.timestamp = QDateTime::currentDateTime();
    m_messages.append(msg);
}

void AIAssistantPanel::startAssistantMessage()
{
    m_isReceivingMessage = true;
    m_currentAssistantMessage.clear();
    
    // 创建新的AI消息item
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
    m_currentAssistantItem = new QStandardItem();
    m_currentAssistantItem->setData("assistant", Qt::UserRole);
    m_currentAssistantItem->setData("", Qt::DisplayRole);
    m_currentAssistantItem->setData(timestamp, Qt::UserRole + 1);
    m_currentAssistantItem->setEditable(false);
    
    m_chatModel->appendRow(m_currentAssistantItem);
}

void AIAssistantPanel::appendToAssistantMessage(const QString &chunk)
{
    m_currentAssistantMessage += chunk;
    
    // 更新当前AI消息item的内容
    if (m_currentAssistantItem) {
        m_currentAssistantItem->setData(m_currentAssistantMessage, Qt::DisplayRole);
        
        // 触发重绘
        QModelIndex index = m_chatModel->indexFromItem(m_currentAssistantItem);
        m_chatModel->dataChanged(index, index);
        
        // 滚动到底部
        m_chatListView->scrollToBottom();
    }
}

void AIAssistantPanel::finishAssistantMessage()
{
    m_isReceivingMessage = false;
    
    // 保存到历史
    ChatMessage msg;
    msg.role = "assistant";
    msg.content = m_currentAssistantMessage;
    msg.timestamp = QDateTime::currentDateTime();
    m_messages.append(msg);
    
    // 保存对话历史
    if (m_hasQuestion) {
        saveConversationHistory();
    }
    
    m_currentAssistantMessage.clear();
    m_currentAssistantItem = nullptr;
}

void AIAssistantPanel::sendChatMessage(const QString &message)
{
    if (!m_aiClient) {
        onErrorOccurred("AI客户端未初始化");
        return;
    }
    
    if (!m_hasQuestion) {
        onErrorOccurred("请先选择一道题目");
        return;
    }
    
    // 构建系统提示词
    QString systemPrompt = buildSystemPrompt();
    
    // 构建完整消息（包含题目上下文）
    QString fullMessage = QString("【当前题目】\n%1\n\n【题目描述】\n%2\n\n【学生的问题】\n%3")
        .arg(m_currentQuestion.title())
        .arg(m_currentQuestion.description())
        .arg(message);
    
    // 发送消息
    m_aiClient->sendChatMessage(fullMessage, systemPrompt);
}

void AIAssistantPanel::loadConversationHistory()
{
    if (!m_hasQuestion) {
        return;
    }
    
    QString filePath = QString("data/conversations/%1.json").arg(m_currentQuestion.id());
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        // 文件不存在，清空历史
        clearHistory();
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        clearHistory();
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonArray messagesArray = obj["messages"].toArray();
    
    m_messages.clear();
    m_chatModel->clear();
    
    for (const QJsonValue &val : messagesArray) {
        QJsonObject msgObj = val.toObject();
        ChatMessage msg;
        msg.role = msgObj["role"].toString();
        msg.content = msgObj["content"].toString();
        msg.timestamp = QDateTime::fromString(msgObj["timestamp"].toString(), Qt::ISODate);
        
        m_messages.append(msg);
        
        // 添加到model
        QString timestamp = msg.timestamp.toString("hh:mm");
        QStandardItem *item = new QStandardItem();
        item->setData(msg.role, Qt::UserRole);
        item->setData(msg.content, Qt::DisplayRole);
        item->setData(timestamp, Qt::UserRole + 1);
        item->setEditable(false);
        
        m_chatModel->appendRow(item);
    }
    
    m_questionCount = obj["questionCount"].toInt(0);
    m_userLevel = obj["userLevel"].toString("beginner");
}

void AIAssistantPanel::saveConversationHistory()
{
    if (!m_hasQuestion || m_messages.isEmpty()) {
        return;
    }
    
    QDir dir("data/conversations");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonObject obj;
    obj["questionId"] = m_currentQuestion.id();
    obj["questionTitle"] = m_currentQuestion.title();  // 保存题目标题
    obj["questionCount"] = m_questionCount;
    obj["userLevel"] = m_userLevel;
    
    QJsonArray messagesArray;
    for (const ChatMessage &msg : m_messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;
        msgObj["timestamp"] = msg.timestamp.toString(Qt::ISODate);
        messagesArray.append(msgObj);
    }
    obj["messages"] = messagesArray;
    
    QString filePath = QString("data/conversations/%1.json").arg(m_currentQuestion.id());
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

void AIAssistantPanel::loadConversationById(const QString &questionId)
{
    QString filePath = QString("data/conversations/%1.json").arg(questionId);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "加载失败", "无法打开对话记录文件");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        QMessageBox::warning(this, "加载失败", "对话记录文件格式错误");
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonArray messagesArray = obj["messages"].toArray();
    
    // 清空当前对话
    m_messages.clear();
    m_chatModel->clear();
    
    // 加载消息
    for (const QJsonValue &val : messagesArray) {
        QJsonObject msgObj = val.toObject();
        ChatMessage msg;
        msg.role = msgObj["role"].toString();
        msg.content = msgObj["content"].toString();
        msg.timestamp = QDateTime::fromString(msgObj["timestamp"].toString(), Qt::ISODate);
        
        m_messages.append(msg);
        
        // 添加到model
        QString timestamp = msg.timestamp.toString("hh:mm");
        QStandardItem *item = new QStandardItem();
        item->setData(msg.role, Qt::UserRole);
        item->setData(msg.content, Qt::DisplayRole);
        item->setData(timestamp, Qt::UserRole + 1);
        item->setEditable(false);
        
        m_chatModel->appendRow(item);
    }
    
    m_questionCount = obj["questionCount"].toInt(0);
    m_userLevel = obj["userLevel"].toString("beginner");
    
    // 滚动到底部
    m_chatListView->scrollToBottom();
    
    QMessageBox::information(this, "加载成功", 
                            QString("已加载 %1 条历史消息").arg(messagesArray.size()));
}

QString AIAssistantPanel::buildSystemPrompt()
{
    return R"(你是一位经验丰富的编程导师，采用费曼学习法教学。

核心原则：
1. 永远不要直接给出答案或完整代码
2. 通过提问引导学生思考
3. 让学生用自己的话解释概念和思路
4. 根据学生的回答质量调整引导程度

教学策略：
- 学生回答正确：鼓励并深入提问，探讨更复杂的情况
- 学生回答模糊：引导其更清晰地表达，问"你能具体说说吗？"
- 学生回答错误：不直接指出，而是反问让其发现问题，如"你确定吗？我们来验证一下"
- 学生完全卡住：给出小提示，但不超过30%的信息，如"想想这个问题的输入输出是什么"

对话风格：
- 友好、耐心、鼓励
- 使用苏格拉底式提问
- 适当使用emoji增加亲和力（但不要过度）
- 语言简洁明了，避免长篇大论

引导示例：
❌ 错误："这道题应该用动态规划，状态转移方程是..."
✅ 正确："你觉得这道题的关键是什么？有没有发现什么规律？"

记住：你的目标是让学生独立思考和解决问题，而不是替他们解决问题。)";
}

QString AIAssistantPanel::formatMessageContent(const QString &content)
{
    QString result = content;
    
    // 处理代码块 ```language\ncode\n```
    QRegularExpression codeBlockRegex("```([^\\n]*)\\n([\\s\\S]*?)```");
    QRegularExpressionMatchIterator it = codeBlockRegex.globalMatch(result);
    
    QVector<QPair<int, int>> replacements;
    QStringList replacementTexts;
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString language = match.captured(1).trimmed();
        QString code = match.captured(2);
        
        // 创建代码块HTML
        QString codeHtml = QString(
            "<div style='margin: 8px 0; background: #1a1a1a; border-radius: 8px; "
            "border: 1px solid #333; overflow: hidden;'>"
            "<div style='background: #2a2a2a; padding: 4px 12px; font-size: 8pt; color: #888;'>%1</div>"
            "<pre style='margin: 0; padding: 12px; color: #e0e0e0; font-family: \"Consolas\", \"Courier New\", monospace; "
            "font-size: 9pt; line-height: 1.4; overflow-x: auto; white-space: pre-wrap; word-wrap: break-word;'>%2</pre>"
            "</div>"
        ).arg(language.isEmpty() ? "代码" : language, code.toHtmlEscaped());
        
        replacements.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        replacementTexts.append(codeHtml);
    }
    
    // 从后往前替换，避免位置偏移
    for (int i = replacements.size() - 1; i >= 0; --i) {
        result.replace(replacements[i].first, 
                      replacements[i].second - replacements[i].first, 
                      replacementTexts[i]);
    }
    
    // 处理行内代码 `code`
    result.replace(QRegularExpression("`([^`]+)`"), 
                  "<code style='background: #2a2a2a; padding: 2px 6px; border-radius: 4px; "
                  "font-family: \"Consolas\", \"Courier New\", monospace; font-size: 9pt; color: #e0e0e0;'>\\1</code>");
    
    // 处理换行
    result.replace("\n", "<br>");
    
    return result;
}
