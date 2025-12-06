#include "AIAssistantPanel.h"
#include "ChatBubbleWidget.h"
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
#include <QScrollArea>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QTimer>

AIAssistantPanel::AIAssistantPanel(OllamaClient *aiClient, QWidget *parent)
    : QWidget(parent)
    , m_aiClient(aiClient)
    , m_hasQuestion(false)
    , m_isReceivingMessage(false)
    , m_currentAssistantBubble(nullptr)
    , m_questionCount(0)
    , m_userLevel("beginner")
    , m_fontScale(1.0)
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
    
    // 顶部按钮栏（新对话和历史记录）
    QHBoxLayout *topButtonLayout = new QHBoxLayout();
    
    m_newChatButton = new QPushButton("🆕 新对话", this);
    m_newChatButton->setToolTip("开始新对话");
    m_newChatButton->setFixedHeight(30);
    
    m_historyButton = new QPushButton("📜 历史", this);
    m_historyButton->setToolTip("查看历史记录");
    m_historyButton->setFixedHeight(30);
    
    topButtonLayout->addWidget(m_newChatButton);
    topButtonLayout->addWidget(m_historyButton);
    
    mainLayout->addLayout(topButtonLayout);
    
    // 对话显示区域（使用QScrollArea + ChatBubbleWidget）
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(R"(
        QScrollArea {
            background-color: #1e1e1e;
            border: none;
        }
    )");
    
    // 创建容器widget
    m_chatContainer = new QWidget();
    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setSpacing(0);
    m_chatLayout->setContentsMargins(0, 0, 0, 0);
    m_chatLayout->addStretch();  // 底部弹性空间
    
    m_scrollArea->setWidget(m_chatContainer);
    m_scrollArea->viewport()->installEventFilter(this);  // 用于Ctrl+滚轮缩放
    
    mainLayout->addWidget(m_scrollArea, 1);
    
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
    
    m_inputField = new QTextEdit(this);
    m_inputField->setPlaceholderText("输入你的问题或想法... (Enter发送, Shift+Enter换行)");
    m_inputField->setMinimumHeight(35);
    m_inputField->setMaximumHeight(120);  // 限制最大高度
    m_inputField->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_inputField->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_inputField->installEventFilter(this);  // 安装事件过滤器
    
    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setFixedWidth(60);
    m_sendButton->setMinimumHeight(35);
    
    // 终止按钮（初始隐藏）
    m_stopButton = new QPushButton("⏹ 终止", this);
    m_stopButton->setFixedWidth(60);
    m_stopButton->setMinimumHeight(35);
    m_stopButton->setVisible(false);  // 初始隐藏
    m_stopButton->setStyleSheet(R"(
        QPushButton {
            background-color: #cc0000;
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #ff0000;
        }
        QPushButton:pressed {
            background-color: #990000;
        }
    )");
    
    inputLayout->addWidget(m_inputField);
    inputLayout->addWidget(m_stopButton);
    inputLayout->addWidget(m_sendButton);
    
    mainLayout->addLayout(inputLayout);
    
    // 连接信号
    connect(m_sendButton, &QPushButton::clicked, this, &AIAssistantPanel::onSendMessage);
    connect(m_stopButton, &QPushButton::clicked, this, &AIAssistantPanel::onStopGeneration);
    connect(m_analyzeButton, &QPushButton::clicked, this, &AIAssistantPanel::onAnalyzeCode);
    connect(m_hintButton, &QPushButton::clicked, this, &AIAssistantPanel::onGetHint);
    connect(m_conceptButton, &QPushButton::clicked, this, &AIAssistantPanel::onExplainConcept);
    connect(m_newChatButton, &QPushButton::clicked, this, &AIAssistantPanel::onNewChat);
    connect(m_historyButton, &QPushButton::clicked, this, &AIAssistantPanel::onViewHistory);
}

void AIAssistantPanel::setQuestionContext(const Question &question)
{
    qDebug() << "[AIAssistantPanel] setQuestionContext called for:" << question.id() << question.title();
    
    // 如果是同一个题目，不需要切换
    if (m_hasQuestion && m_currentQuestion.id() == question.id()) {
        qDebug() << "[AIAssistantPanel] Same question, skipping switch";
        return;
    }
    
    // 先保存当前题目的对话历史（如果有的话）
    if (m_hasQuestion && !m_messages.isEmpty()) {
        qDebug() << "[AIAssistantPanel] Saving conversation for old question:" << m_currentQuestion.id() 
                 << "messages:" << m_messages.size();
        saveConversationHistory();
    }
    
    // 切换到新题目
    QString oldQuestionId = m_hasQuestion ? m_currentQuestion.id() : "none";
    m_currentQuestion = question;
    m_hasQuestion = true;
    
    qDebug() << "[AIAssistantPanel] Switched from" << oldQuestionId << "to" << question.id();
    
    // 加载新题目的对话历史
    loadConversationHistory();
}

void AIAssistantPanel::clearHistory()
{
    qDebug() << "[AIAssistantPanel] clearHistory called, clearing" << m_messages.size() << "messages";
    
    // 清除所有消息widget
    int widgetCount = 0;
    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            // 使用 delete 而不是 deleteLater，确保立即删除
            delete item->widget();
            widgetCount++;
        }
        delete item;
    }
    m_chatLayout->addStretch();  // 重新添加弹性空间
    
    m_messages.clear();
    m_questionCount = 0;
    m_currentAssistantBubble = nullptr;
    
    qDebug() << "[AIAssistantPanel] Cleared" << widgetCount << "widgets from layout";
    
    // 强制处理待删除的事件，确保布局完全清空
    QApplication::processEvents();
}

void AIAssistantPanel::refreshChat()
{
    qDebug() << "[AIAssistantPanel] refreshChat called for question:" 
             << (m_hasQuestion ? m_currentQuestion.id() : "none");
    
    // 保存当前对话到历史
    if (m_hasQuestion && !m_messages.isEmpty()) {
        qDebug() << "[AIAssistantPanel] Saving conversation before clearing, messages:" << m_messages.size();
        saveConversationHistory();
    }
    
    // 清空当前对话（但保留题目上下文）
    clearHistory();
    
    qDebug() << "[AIAssistantPanel] Chat cleared, current question still:" 
             << (m_hasQuestion ? m_currentQuestion.id() : "none");
}

void AIAssistantPanel::viewHistory()
{
    // 检查是否有当前题目
    if (!m_hasQuestion) {
        QMessageBox::information(this, "提示", "请先选择一道题目");
        return;
    }
    
    ChatHistoryDialog dialog(this);
    
    // 设置当前题目ID，只显示当前题目的对话历史
    dialog.setCurrentQuestionId(m_currentQuestion.id());
    
    connect(&dialog, &ChatHistoryDialog::conversationSelected,
            this, [this](const QString &questionId) {
        // 加载选中的对话
        loadConversationById(questionId);
    });
    
    // 连接删除信号
    connect(&dialog, &ChatHistoryDialog::conversationDeleted,
            this, [this](const QString &questionId) {
        // 如果删除的是当前显示的对话，清空显示
        if (m_hasQuestion && m_currentQuestion.id() == questionId) {
            clearHistory();
        }
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
    
    // 确保滚动到底部（延迟更长时间，确保气泡完全渲染）
    QTimer::singleShot(100, this, [this]() {
        QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    });
}

void AIAssistantPanel::onSendMessage()
{
    QString message = m_inputField->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
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
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
    }
    
    // 发出信号请求更新当前代码
    emit requestCurrentCode();
    
    // 稍微延迟一下，确保代码已更新
    QTimer::singleShot(50, this, [this]() {
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
    });
}

void AIAssistantPanel::onGetHint()
{
    if (!m_hasQuestion) {
        QMessageBox::warning(this, "提示", "请先选择一道题目");
        return;
    }
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
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
    
    // 如果正在接收AI消息，先终止
    if (m_isReceivingMessage) {
        onStopGeneration();
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

void AIAssistantPanel::onStopGeneration()
{
    if (!m_aiClient) {
        return;
    }
    
    qDebug() << "[AIAssistantPanel] 用户请求终止输出";
    
    // 终止AI客户端的当前请求
    m_aiClient->abortCurrentRequest();
    
    // 如果正在接收消息，添加终止标记并完成消息
    if (m_isReceivingMessage && m_currentAssistantBubble) {
        m_currentAssistantMessage += "\n\n⏹ **输出已终止**";
        m_currentAssistantBubble->setContent(m_currentAssistantMessage);
        finishAssistantMessage();
        // finishAssistantMessage() 会恢复按钮状态，所以这里不需要再次设置
    } else {
        // 如果没有正在接收的消息，手动恢复按钮状态
        m_stopButton->setVisible(false);
        m_sendButton->setVisible(true);
    }
}

void AIAssistantPanel::onErrorOccurred(const QString &error)
{
    qWarning() << "[AIAssistantPanel] Error occurred:" << error;
    
    // 检查UI组件是否已初始化
    if (!m_chatContainer || !m_chatLayout) {
        qCritical() << "[AIAssistantPanel] Chat container or layout not initialized!";
        QMessageBox::critical(this, "AI连接错误", 
            QString("AI连接失败：%1\n\n请检查AI配置或网络连接。").arg(error));
        return;
    }
    
    // 恢复按钮状态
    m_stopButton->setVisible(false);
    m_sendButton->setVisible(true);
    
    // 如果正在接收消息，先结束当前消息
    if (m_isReceivingMessage) {
        m_isReceivingMessage = false;
        if (m_currentAssistantBubble) {
            // 更新当前气泡显示错误
            m_currentAssistantBubble->setContent(QString("❌ 错误: %1").arg(error));
            m_currentAssistantBubble = nullptr;
        } else {
            // 创建新的错误气泡
            QString errorMsg = QString("❌ 错误: %1").arg(error);
            ChatBubbleWidget *bubble = new ChatBubbleWidget(errorMsg, false, m_chatContainer);
            bubble->setFontScale(m_fontScale);
            m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);
        }
    } else {
        // 创建错误消息气泡
        QString errorMsg = QString("❌ 错误: %1").arg(error);
        ChatBubbleWidget *bubble = new ChatBubbleWidget(errorMsg, false, m_chatContainer);
        bubble->setFontScale(m_fontScale);
        
        // 插入到布局中
        m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);
    }
    
    scrollToBottom();
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
    // 创建用户消息气泡
    ChatBubbleWidget *bubble = new ChatBubbleWidget(message, true, m_chatContainer);
    bubble->setFontScale(m_fontScale);
    
    // 插入到布局中（在stretch之前）
    m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);
    
    // 滚动到底部
    scrollToBottom();
    
    // 保存到历史
    ChatMessage msg;
    msg.role = "user";
    msg.content = message;
    msg.timestamp = QDateTime::currentDateTime();
    m_messages.append(msg);
    
    // 立即保存用户消息（避免切换题目时丢失）
    if (m_hasQuestion) {
        saveConversationHistory();
    }
}

void AIAssistantPanel::startAssistantMessage()
{
    m_isReceivingMessage = true;
    m_currentAssistantMessage.clear();
    
    // 显示终止按钮，隐藏发送按钮
    m_sendButton->setVisible(false);
    m_stopButton->setVisible(true);
    
    // 创建新的AI消息气泡
    m_currentAssistantBubble = new ChatBubbleWidget("", false, m_chatContainer);
    m_currentAssistantBubble->setFontScale(m_fontScale);
    
    // 插入到布局中（在stretch之前）
    m_chatLayout->insertWidget(m_chatLayout->count() - 1, m_currentAssistantBubble);
}

void AIAssistantPanel::appendToAssistantMessage(const QString &chunk)
{
    m_currentAssistantMessage += chunk;
    
    // 更新当前AI消息气泡的内容
    if (m_currentAssistantBubble) {
        m_currentAssistantBubble->setContent(m_currentAssistantMessage);
        
        // 滚动到底部
        scrollToBottom();
    }
}

void AIAssistantPanel::finishAssistantMessage()
{
    m_isReceivingMessage = false;
    
    // 恢复发送按钮，隐藏终止按钮
    m_stopButton->setVisible(false);
    m_sendButton->setVisible(true);
    
    // 强制更新气泡布局，确保气泡大小正确匹配内容
    if (m_currentAssistantBubble) {
        // 强制Qt重新计算布局
        m_currentAssistantBubble->updateGeometry();
        m_currentAssistantBubble->adjustSize();
        
        // 保存气泡指针到局部变量，避免lambda中访问已清空的成员变量
        ChatBubbleWidget *bubble = m_currentAssistantBubble;
        
        // 延迟一帧再次更新，确保布局完全计算完成
        QTimer::singleShot(0, this, [this, bubble]() {
            // 检查bubble是否还有效（可能已被删除）
            if (bubble && m_chatLayout) {
                // 验证bubble还在布局中
                bool found = false;
                for (int i = 0; i < m_chatLayout->count(); ++i) {
                    QLayoutItem *item = m_chatLayout->itemAt(i);
                    if (item && item->widget() == bubble) {
                        found = true;
                        break;
                    }
                }
                
                if (found) {
                    bubble->forceUpdate();
                    m_chatLayout->invalidate();
                    m_chatLayout->activate();
                    scrollToBottom();
                }
            }
        });
    }
    
    // 保存到历史（只有内容不为空时才保存）
    if (!m_currentAssistantMessage.isEmpty()) {
        ChatMessage msg;
        msg.role = "assistant";
        msg.content = m_currentAssistantMessage;
        msg.timestamp = QDateTime::currentDateTime();
        m_messages.append(msg);
        
        // 保存对话历史
        if (m_hasQuestion) {
            saveConversationHistory();
        }
    }
    
    m_currentAssistantMessage.clear();
    m_currentAssistantBubble = nullptr;
}

void AIAssistantPanel::sendChatMessage(const QString &message)
{
    qDebug() << "[AIAssistantPanel] Sending chat message, length:" << message.length();
    
    if (!m_aiClient) {
        qCritical() << "[AIAssistantPanel] AI client is null!";
        onErrorOccurred("AI客户端未初始化");
        return;
    }
    
    if (!m_hasQuestion) {
        qWarning() << "[AIAssistantPanel] No question selected";
        onErrorOccurred("请先选择一道题目");
        return;
    }
    
    try {
        // 构建系统提示词
        QString systemPrompt = buildSystemPrompt();
        
        // 构建完整消息（包含题目上下文）
        QString fullMessage = QString("【当前题目】\n%1\n\n【题目描述】\n%2\n\n【学生的问题】\n%3")
            .arg(m_currentQuestion.title())
            .arg(m_currentQuestion.description())
            .arg(message);
        
        qDebug() << "[AIAssistantPanel] Full message length:" << fullMessage.length();
        qDebug() << "[AIAssistantPanel] Calling sendChatMessage...";
        
        // 发送消息
        m_aiClient->sendChatMessage(fullMessage, systemPrompt);
        
        qDebug() << "[AIAssistantPanel] Message sent successfully";
    } catch (const std::exception &e) {
        qCritical() << "[AIAssistantPanel] Exception in sendChatMessage:" << e.what();
        onErrorOccurred(QString("发送消息时发生错误：%1").arg(e.what()));
    } catch (...) {
        qCritical() << "[AIAssistantPanel] Unknown exception in sendChatMessage";
        onErrorOccurred("发送消息时发生未知错误");
    }
}

void AIAssistantPanel::loadConversationHistory()
{
    if (!m_hasQuestion) {
        return;
    }
    
    QString filePath = QString("data/conversations/%1.json").arg(m_currentQuestion.id());
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 文件不存在，清空历史
        qDebug() << "[AIAssistantPanel] No conversation history found for question:" << m_currentQuestion.id();
        clearHistory();
        return;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    
    if (!doc.isObject()) {
        qWarning() << "[AIAssistantPanel] Invalid conversation history format for question:" << m_currentQuestion.id();
        clearHistory();
        return;
    }
    
    qDebug() << "[AIAssistantPanel] Loading conversation history for question:" << m_currentQuestion.id();
    
    QJsonObject obj = doc.object();
    QJsonArray messagesArray = obj["messages"].toArray();
    
    qDebug() << "[AIAssistantPanel] Found" << messagesArray.size() << "messages in history";
    
    m_messages.clear();
    clearHistory();  // 清除现有气泡
    
    for (const QJsonValue &val : messagesArray) {
        QJsonObject msgObj = val.toObject();
        ChatMessage msg;
        msg.role = msgObj["role"].toString();
        
        // 清理内容：移除首尾空白和多余换行
        QString rawContent = msgObj["content"].toString();
        msg.content = rawContent.trimmed();
        // 将多个连续换行替换为最多两个换行（保留段落分隔）
        msg.content.replace(QRegularExpression("\\n{3,}"), "\n\n");
        
        msg.timestamp = QDateTime::fromString(msgObj["timestamp"].toString(), Qt::ISODate);
        
        m_messages.append(msg);
        
        // 创建气泡
        bool isUser = (msg.role == "user");
        ChatBubbleWidget *bubble = new ChatBubbleWidget(msg.content, isUser, m_chatContainer);
        bubble->setFontScale(m_fontScale);
        m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);
        
        qDebug() << "[AIAssistantPanel] Loaded message:" << msg.role 
                 << "raw length:" << rawContent.length() 
                 << "cleaned length:" << msg.content.length();
    }
    
    m_questionCount = obj["questionCount"].toInt(0);
    m_userLevel = obj["userLevel"].toString("beginner");
    
    qDebug() << "[AIAssistantPanel] Conversation loaded successfully, total messages:" << m_messages.size();
    
    // 强制更新所有气泡的尺寸
    QTimer::singleShot(50, this, [this]() {
        for (int i = 0; i < m_chatLayout->count(); ++i) {
            QLayoutItem *item = m_chatLayout->itemAt(i);
            if (item && item->widget()) {
                ChatBubbleWidget *bubble = qobject_cast<ChatBubbleWidget*>(item->widget());
                if (bubble) {
                    bubble->updateGeometry();
                }
            }
        }
        m_chatContainer->updateGeometry();
        m_chatLayout->update();
        
        // 再延迟一点滚动到底部
        QTimer::singleShot(50, this, [this]() {
            scrollToBottom();
        });
    });
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
        
        // 清理内容：移除首尾空白和多余换行
        QString cleanContent = msg.content.trimmed();
        // 将多个连续换行替换为最多两个换行（保留段落分隔）
        cleanContent.replace(QRegularExpression("\\n{3,}"), "\n\n");
        
        msgObj["content"] = cleanContent;
        msgObj["timestamp"] = msg.timestamp.toString(Qt::ISODate);
        messagesArray.append(msgObj);
    }
    obj["messages"] = messagesArray;
    
    QString filePath = QString("data/conversations/%1.json").arg(m_currentQuestion.id());
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "[AIAssistantPanel] Saved conversation to:" << filePath << "messages:" << m_messages.size();
    } else {
        qWarning() << "[AIAssistantPanel] Failed to save conversation to:" << filePath;
    }
}

void AIAssistantPanel::loadConversationById(const QString &questionId)
{
    QString filePath = QString("data/conversations/%1.json").arg(questionId);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "加载失败", "无法打开对话记录文件");
        qWarning() << "[AIAssistantPanel] Failed to open conversation file:" << filePath;
        return;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    
    if (!doc.isObject()) {
        QMessageBox::warning(this, "加载失败", "对话记录文件格式错误");
        qWarning() << "[AIAssistantPanel] Invalid JSON format in:" << filePath;
        return;
    }
    
    qDebug() << "[AIAssistantPanel] Loading conversation by ID:" << questionId;
    
    QJsonObject obj = doc.object();
    QJsonArray messagesArray = obj["messages"].toArray();
    
    // 清空当前对话
    m_messages.clear();
    clearHistory();
    
    // 加载消息
    for (const QJsonValue &val : messagesArray) {
        QJsonObject msgObj = val.toObject();
        ChatMessage msg;
        msg.role = msgObj["role"].toString();
        
        // 清理内容：移除首尾空白和多余换行
        QString rawContent = msgObj["content"].toString();
        msg.content = rawContent.trimmed();
        // 将多个连续换行替换为最多两个换行（保留段落分隔）
        msg.content.replace(QRegularExpression("\\n{3,}"), "\n\n");
        
        msg.timestamp = QDateTime::fromString(msgObj["timestamp"].toString(), Qt::ISODate);
        
        m_messages.append(msg);
        
        // 创建气泡
        bool isUser = (msg.role == "user");
        ChatBubbleWidget *bubble = new ChatBubbleWidget(msg.content, isUser, m_chatContainer);
        bubble->setFontScale(m_fontScale);
        m_chatLayout->insertWidget(m_chatLayout->count() - 1, bubble);
    }
    
    m_questionCount = obj["questionCount"].toInt(0);
    m_userLevel = obj["userLevel"].toString("beginner");
    
    // 强制更新所有气泡的尺寸
    QTimer::singleShot(50, this, [this, messagesArray]() {
        for (int i = 0; i < m_chatLayout->count(); ++i) {
            QLayoutItem *item = m_chatLayout->itemAt(i);
            if (item && item->widget()) {
                ChatBubbleWidget *bubble = qobject_cast<ChatBubbleWidget*>(item->widget());
                if (bubble) {
                    bubble->updateGeometry();
                }
            }
        }
        m_chatContainer->updateGeometry();
        m_chatLayout->update();
        
        // 再延迟一点滚动到底部
        QTimer::singleShot(50, this, [this, messagesArray]() {
            scrollToBottom();
            QMessageBox::information(this, "加载成功", 
                                    QString("已加载 %1 条历史消息").arg(messagesArray.size()));
        });
    });
}

QString AIAssistantPanel::buildSystemPrompt()
{
    return R"(你是一位编程导师，帮助学生学习C++编程。

【核心原则】
1. 🎯 准确第一：确保技术建议完全正确
2. 💡 简洁直接：少说废话，多给代码
3. 🔍 仔细验证：分析代码前必须手动模拟执行

【回答要求】
- 根据问题复杂度决定回答长度
- 简单问题：一句话 + 代码示例
- 复杂问题：可以适当展开，但避免冗长
- 直接指出问题，不要铺垫
- 用代码示例说明，而不是文字描述
- 一次只讲一个核心问题

【回答格式参考】
简单问题：
"循环条件错了，应该是 `i < n`：
```cpp
for (int i = 0; i < n; i++) {
    // ...
}
```
数组下标从0到n-1。"

复杂问题（可以多说一点）：
"你的算法思路有问题，这道题需要用动态规划：
```cpp
int dp[n+1];
dp[0] = 0;
for (int i = 1; i <= n; i++) {
    dp[i] = min(dp[i-1] + cost1, dp[i-2] + cost2);
}
```
dp[i]表示到第i步的最小代价。状态转移方程是从前一步或前两步转移过来。"

【代码分析要求】
- 逐行读代码，理解变量作用域
- 手动模拟执行，验证逻辑
- 区分数组不同元素（point[0]和point[1]独立）
- 确认分析正确再回答

【禁止】
- ❌ 过多的鼓励性话语
- ❌ 重复说明同一个问题
- ❌ 没验证就指出"问题"
- ❌ 使用过于专业的术语

记住：简洁、准确、多代码少废话。根据问题复杂度灵活调整回答长度。)";
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


void AIAssistantPanel::scrollToBottom()
{
    // 延迟滚动，确保内容已经渲染
    QTimer::singleShot(50, this, [this]() {
        QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    });
}

void AIAssistantPanel::updateAllBubbleScales()
{
    for (int i = 0; i < m_chatLayout->count(); ++i) {
        QLayoutItem *item = m_chatLayout->itemAt(i);
        if (item && item->widget()) {
            ChatBubbleWidget *bubble = qobject_cast<ChatBubbleWidget*>(item->widget());
            if (bubble) {
                bubble->setFontScale(m_fontScale);
            }
        }
    }
}

bool AIAssistantPanel::eventFilter(QObject *obj, QEvent *event)
{
    // 处理输入框的 Enter 键
    if (obj == m_inputField && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        // Enter 发送，Shift+Enter 换行
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                // Shift+Enter: 插入换行
                return false;  // 让默认行为处理（插入换行）
            } else {
                // 单独 Enter: 发送消息
                onSendMessage();
                return true;  // 阻止默认行为
            }
        }
    }
    
    // 处理滚动区域的滚轮事件
    if (obj == m_scrollArea->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        
        // 检查是否按下Ctrl键
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            // Ctrl+滚轮：缩放
            qreal delta = wheelEvent->angleDelta().y() / 120.0;
            m_fontScale += delta * 0.1;  // 每次缩放10%
            
            if (m_fontScale < 0.5) m_fontScale = 0.5;
            if (m_fontScale > 2.0) m_fontScale = 2.0;
            
            updateAllBubbleScales();
            
            return true;  // 事件已处理
        } else {
            // 普通滚轮：精细滚动
            QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
            int delta = wheelEvent->angleDelta().y();
            
            // 减小滚动步长，提高精度（原来是120一步，现在改为40一步）
            int step = scrollBar->singleStep() / 3;  // 减小到1/3
            int scrollAmount = -(delta / 40) * step;  // 每40单位滚动一个小步长
            
            scrollBar->setValue(scrollBar->value() + scrollAmount);
            
            return true;  // 事件已处理
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void AIAssistantPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 宽度变化时更新所有气泡
    if (event->oldSize().width() != event->size().width()) {
        for (int i = 0; i < m_chatLayout->count(); ++i) {
            QLayoutItem *item = m_chatLayout->itemAt(i);
            if (item && item->widget()) {
                ChatBubbleWidget *bubble = qobject_cast<ChatBubbleWidget*>(item->widget());
                if (bubble) {
                    bubble->forceUpdate();
                }
            }
        }
    }
}
