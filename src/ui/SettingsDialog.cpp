#include "SettingsDialog.h"
#include "../utils/ConfigManager.h"
#include "../utils/CompilerDetector.h"
#include "../utils/ErrorHandler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI()
{
    setWindowTitle("设置");
    resize(600, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);
    
    // 标签页
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // === 编译器设置 ===
    QWidget *compilerTab = new QWidget();
    QVBoxLayout *compilerLayout = new QVBoxLayout(compilerTab);
    compilerLayout->setSpacing(12);
    
    QGroupBox *compilerGroup = new QGroupBox("编译器配置", this);
    QFormLayout *compilerForm = new QFormLayout(compilerGroup);
    compilerForm->setSpacing(12);
    
    m_compilerPathEdit = new QLineEdit(this);
    m_compilerPathEdit->setPlaceholderText("例如: g++ 或 C:/MinGW/bin/g++.exe");
    
    QHBoxLayout *compilerBtnLayout = new QHBoxLayout();
    m_browseCompilerBtn = new QPushButton("浏览...", this);
    m_testCompilerBtn = new QPushButton("测试", this);
    m_detectCompilerBtn = new QPushButton("自动检测", this);
    
    compilerBtnLayout->addWidget(m_browseCompilerBtn);
    compilerBtnLayout->addWidget(m_testCompilerBtn);
    compilerBtnLayout->addWidget(m_detectCompilerBtn);
    compilerBtnLayout->addStretch();
    
    QVBoxLayout *compilerPathLayout = new QVBoxLayout();
    compilerPathLayout->addWidget(m_compilerPathEdit);
    compilerPathLayout->addLayout(compilerBtnLayout);
    
    compilerForm->addRow("编译器路径:", compilerPathLayout);
    
    QLabel *compilerHint = new QLabel(
        QString::fromUtf8("💡 提示：\n"
        "• 可以使用命令名（如 g++）或完整路径\n"
        "• 点击\"自动检测\"查找系统中的编译器\n"
        "• 点击\"测试\"验证编译器是否可用"),
        this
    );
    compilerHint->setStyleSheet("color: #b0b0b0; font-size: 9pt;");
    compilerHint->setWordWrap(true);
    
    compilerLayout->addWidget(compilerGroup);
    compilerLayout->addWidget(compilerHint);
    compilerLayout->addStretch();
    
    // === AI设置 ===
    QWidget *aiTab = new QWidget();
    QVBoxLayout *aiLayout = new QVBoxLayout(aiTab);
    aiLayout->setSpacing(16);
    
    // AI模式选择标签页
    m_aiTabWidget = new QTabWidget(aiTab);
    m_aiTabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            background: #1e1e1e;
        }
        QTabBar::tab {
            background: #2a2a2a;
            color: #b0b0b0;
            padding: 10px 20px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: #660000;
            color: white;
        }
        QTabBar::tab:hover {
            background: #3a3a3a;
        }
    )");
    
    // === 本地Ollama标签页 ===
    QWidget *localTab = new QWidget();
    QVBoxLayout *localLayout = new QVBoxLayout(localTab);
    localLayout->setSpacing(12);
    
    QLabel *localInfo = new QLabel(
        "💻 使用本地Ollama服务\n"
        "• 完全免费，数据隐私\n"
        "• 需要先安装Ollama并下载模型",
        localTab
    );
    localInfo->setStyleSheet("color: #b0b0b0; padding: 10px; background: #1a1a1a; border-radius: 5px;");
    localLayout->addWidget(localInfo);
    
    QFormLayout *ollamaForm = new QFormLayout();
    ollamaForm->setSpacing(12);
    
    m_ollamaUrlEdit = new QLineEdit(localTab);
    m_ollamaUrlEdit->setPlaceholderText("http://localhost:11434");
    m_ollamaUrlEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 8px;
        }
        QLineEdit:focus {
            border-color: #660000;
        }
    )");
    
    // 模型选择下拉框（可编辑）
    m_ollamaModelCombo = new QComboBox(localTab);
    m_ollamaModelCombo->setEditable(true);
    m_ollamaModelCombo->setPlaceholderText("选择或输入模型名称");
    m_ollamaModelCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 8px;
        }
        QComboBox:focus {
            border-color: #660000;
        }
        QComboBox::drop-down {
            border: none;
            width: 30px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #e8e8e8;
            margin-right: 10px;
        }
        QComboBox QAbstractItemView {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #660000;
            selection-background-color: #660000;
        }
    )");
    
    // 检测模型按钮
    m_detectModelsBtn = new QPushButton("🔍 检测模型", localTab);
    m_detectModelsBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2a5a2a;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #3a7a3a;
        }
        QPushButton:pressed {
            background-color: #1a4a1a;
        }
    )");
    
    QHBoxLayout *modelLayout = new QHBoxLayout();
    modelLayout->addWidget(m_ollamaModelCombo, 1);
    modelLayout->addWidget(m_detectModelsBtn);
    
    QLabel *urlLabel = new QLabel("服务地址:", localTab);
    urlLabel->setStyleSheet("color: #e8e8e8; font-weight: bold;");
    QLabel *modelLabel = new QLabel("模型名称:", localTab);
    modelLabel->setStyleSheet("color: #e8e8e8; font-weight: bold;");
    
    ollamaForm->addRow(urlLabel, m_ollamaUrlEdit);
    ollamaForm->addRow(modelLabel, modelLayout);
    
    localLayout->addLayout(ollamaForm);
    
    QLabel *localTip = new QLabel(
        "💡 提示：\n"
        "1. 访问 https://ollama.ai 下载安装\n"
        "2. 运行命令：ollama pull qwen2.5:7b（或其他模型）\n"
        "3. 启动服务：ollama serve\n"
        "4. 点击\"检测模型\"按钮自动识别已安装的模型",
        localTab
    );
    localTip->setStyleSheet("color: #888; font-size: 9pt; margin-top: 10px;");
    localLayout->addWidget(localTip);
    localLayout->addStretch();
    
    // === 云端API标签页 ===
    QWidget *cloudTab = new QWidget();
    QVBoxLayout *cloudLayout = new QVBoxLayout(cloudTab);
    cloudLayout->setSpacing(12);
    
    QLabel *cloudInfo = new QLabel(
        "☁️ 使用云端AI服务\n"
        "• 支持OpenAI、DeepSeek等API\n"
        "• 需要API Key（可能需要付费）",
        cloudTab
    );
    cloudInfo->setStyleSheet("color: #b0b0b0; padding: 10px; background: #1a1a1a; border-radius: 5px;");
    cloudLayout->addWidget(cloudInfo);
    
    QLabel *apiKeyLabel = new QLabel("API Key:", cloudTab);
    apiKeyLabel->setStyleSheet("color: #e8e8e8; font-weight: bold; margin-top: 15px;");
    cloudLayout->addWidget(apiKeyLabel);
    
    m_cloudApiKeyEdit = new QLineEdit(cloudTab);
    m_cloudApiKeyEdit->setPlaceholderText("输入你的API Key...");
    m_cloudApiKeyEdit->setEchoMode(QLineEdit::Password);
    m_cloudApiKeyEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #1a1a1a;
            color: #e8e8e8;
            border: 2px solid #3a3a3a;
            border-radius: 8px;
            padding: 10px;
            font-size: 10pt;
        }
        QLineEdit:focus {
            border-color: #660000;
        }
    )");
    cloudLayout->addWidget(m_cloudApiKeyEdit);
    
    QLabel *cloudTip = new QLabel(
        "💡 提示：\n"
        "• OpenAI: 使用默认地址\n"
        "• DeepSeek等兼容OpenAI API的服务也可使用\n"
        "• 配置云端API后将自动切换到云端模式",
        cloudTab
    );
    cloudTip->setStyleSheet("color: #888; font-size: 9pt; margin-top: 10px;");
    cloudLayout->addWidget(cloudTip);
    cloudLayout->addStretch();
    
    m_aiTabWidget->addTab(localTab, "🖥️ 本地Ollama");
    m_aiTabWidget->addTab(cloudTab, "☁️ 云端API");
    
    aiLayout->addWidget(m_aiTabWidget);
    
    QLabel *aiNote = new QLabel(
        "⚠️ 注意：保存设置后，AI模式将根据配置自动切换\n"
        "• 如果配置了云端API Key，将使用云端模式\n"
        "• 如果只配置了本地模型，将使用本地模式",
        aiTab
    );
    aiNote->setStyleSheet("color: #ff8800; font-size: 9pt; padding: 10px; background: #2a1a00; border-radius: 5px;");
    aiLayout->addWidget(aiNote);
    
    // === 编辑器设置 ===
    QWidget *editorTab = new QWidget();
    QVBoxLayout *editorLayout = new QVBoxLayout(editorTab);
    editorLayout->setSpacing(12);
    
    QGroupBox *editorGroup = new QGroupBox("编辑器配置", this);
    QFormLayout *editorForm = new QFormLayout(editorGroup);
    editorForm->setSpacing(12);
    
    m_fontSizeCombo = new QComboBox(this);
    m_fontSizeCombo->addItems({"8", "9", "10", "11", "12", "14", "16"});
    m_fontSizeCombo->setCurrentText("10");
    
    m_tabWidthCombo = new QComboBox(this);
    m_tabWidthCombo->addItems({"2", "4", "8"});
    m_tabWidthCombo->setCurrentText("4");
    
    editorForm->addRow("字体大小:", m_fontSizeCombo);
    editorForm->addRow("Tab 宽度:", m_tabWidthCombo);
    
    editorLayout->addWidget(editorGroup);
    editorLayout->addStretch();
    
    // 添加标签页
    tabWidget->addTab(compilerTab, "🔧 编译器");
    tabWidget->addTab(aiTab, "🤖 AI");
    tabWidget->addTab(editorTab, "✏️ 编辑器");
    
    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton("保存", this);
    m_cancelBtn = new QPushButton("取消", this);
    
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 10px 24px;
            font-weight: 500;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
    )";
    
    m_saveBtn->setStyleSheet(btnStyle);
    m_cancelBtn->setStyleSheet(btnStyle);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_cancelBtn);
    
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(btnLayout);
    
    // 应用样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
        QGroupBox {
            color: #e8e8e8;
            border: 1px solid #4a4a4a;
            border-radius: 10px;
            margin-top: 12px;
            padding-top: 12px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 8px;
        }
        QLineEdit, QComboBox {
            background-color: #242424;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-radius: 8px;
            padding: 8px 12px;
        }
        QLineEdit:focus, QComboBox:focus {
            border-color: #660000;
        }
        QTabWidget::pane {
            border: 1px solid #3a3a3a;
            border-radius: 8px;
            background-color: #242424;
        }
        QTabBar::tab {
            background-color: #2d2d2d;
            color: #e8e8e8;
            border: 1px solid #3a3a3a;
            border-bottom: none;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            padding: 10px 20px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: #660000;
        }
        QTabBar::tab:hover {
            background-color: #363636;
        }
    )");
    
    // 连接信号
    connect(m_browseCompilerBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseCompiler);
    connect(m_testCompilerBtn, &QPushButton::clicked, this, &SettingsDialog::onTestCompiler);
    connect(m_detectCompilerBtn, &QPushButton::clicked, this, &SettingsDialog::onDetectCompiler);
    connect(m_detectModelsBtn, &QPushButton::clicked, this, &SettingsDialog::onDetectOllamaModels);
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);
}

void SettingsDialog::loadSettings()
{
    ConfigManager &config = ConfigManager::instance();
    
    m_compilerPathEdit->setText(config.compilerPath());
    m_ollamaUrlEdit->setText(config.ollamaUrl());
    
    // 设置当前模型到下拉框
    QString currentModel = config.ollamaModel();
    m_ollamaModelCombo->setCurrentText(currentModel);
    
    m_cloudApiKeyEdit->setText(config.cloudApiKey());
    
    // 根据当前模式选择标签页
    if (config.useCloudMode()) {
        m_aiTabWidget->setCurrentIndex(1);  // 云端标签页
    } else {
        m_aiTabWidget->setCurrentIndex(0);  // 本地标签页
    }
}

void SettingsDialog::saveSettings()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 保存编译器配置
    config.setCompilerPath(m_compilerPathEdit->text());
    
    // 保存AI配置 - 始终保存所有配置，避免丢失
    QString cloudApiKey = m_cloudApiKeyEdit->text().trimmed();
    QString ollamaModel = m_ollamaModelCombo->currentText().trimmed();
    QString ollamaUrl = m_ollamaUrlEdit->text().trimmed();
    
    // 保存本地Ollama配置（如果有）
    if (!ollamaModel.isEmpty()) {
        config.setOllamaModel(ollamaModel);
        config.setOllamaUrl(ollamaUrl.isEmpty() ? "http://localhost:11434" : ollamaUrl);
    }
    
    // 保存云端API配置（如果有）
    if (!cloudApiKey.isEmpty()) {
        config.setCloudApiKey(cloudApiKey);
    }
    
    // 获取当前选中的标签页，决定使用哪种模式
    int currentTab = m_aiTabWidget->currentIndex();
    
    if (currentTab == 0) {
        // 本地Ollama标签页
        if (ollamaModel.isEmpty()) {
            QMessageBox::warning(this, "配置错误", "请输入本地模型名称或点击\"检测模型\"");
            return;
        }
        
        config.setUseCloudMode(false);  // 设置当前使用本地模式
        QMessageBox::information(this, "配置成功",
            QString("✅ 已切换到本地Ollama模式\n\n"
                    "模型：%1\n"
                    "地址：%2\n\n"
                    "💡 云端API配置已保留").arg(ollamaModel, ollamaUrl.isEmpty() ? "http://localhost:11434" : ollamaUrl));
    } else {
        // 云端API标签页
        if (cloudApiKey.isEmpty()) {
            QMessageBox::warning(this, "配置错误", "请输入API Key");
            return;
        }
        
        config.setUseCloudMode(true);  // 设置当前使用云端模式
        QMessageBox::information(this, "配置成功",
            "✅ 已切换到云端API模式\n\n"
            "AI分析功能将使用云端API服务\n\n"
            "💡 本地Ollama配置已保留");
    }
    
    config.save();
}

void SettingsDialog::onBrowseCompiler()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "选择编译器",
        "",
        "可执行文件 (*.exe);;所有文件 (*.*)"
    );
    
    if (!file.isEmpty()) {
        m_compilerPathEdit->setText(file);
    }
}

void SettingsDialog::onTestCompiler()
{
    QString path = m_compilerPathEdit->text();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入编译器路径");
        return;
    }
    
    if (CompilerDetector::validateCompiler(path)) {
        QString version = CompilerDetector::getCompilerVersion(path);
        QMessageBox::information(this, "测试成功",
            QString("编译器可用！\n\n版本: %1").arg(version));
    } else {
        QMessageBox::warning(this, "测试失败",
            "编译器不可用或路径错误\n\n请检查路径是否正确");
    }
}

void SettingsDialog::onDetectCompiler()
{
    QList<CompilerInfo> compilers = CompilerDetector::detectCompilers();
    
    if (compilers.isEmpty()) {
        QMessageBox::warning(this, "未找到编译器",
            "未检测到系统中的 C++ 编译器\n\n"
            "请安装 MinGW 或 Clang");
        return;
    }
    
    QString message = "检测到以下编译器：\n\n";
    for (const auto &compiler : compilers) {
        message += QString("• %1 %2\n  路径: %3\n\n")
            .arg(compiler.name, compiler.version, compiler.path);
    }
    
    message += "是否使用第一个编译器？";
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "检测结果", message,
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_compilerPathEdit->setText(compilers.first().path);
    }
}

void SettingsDialog::onSave()
{
    saveSettings();
    
    // 发送AI配置更改信号
    emit aiConfigChanged();
    
    QMessageBox::information(this, "保存成功", "设置已保存并立即生效");
    accept();
}

void SettingsDialog::onDetectOllamaModels()
{
    QString ollamaUrl = m_ollamaUrlEdit->text().trimmed();
    if (ollamaUrl.isEmpty()) {
        ollamaUrl = "http://localhost:11434";
        m_ollamaUrlEdit->setText(ollamaUrl);
    }
    
    // 禁用按钮，显示检测中
    m_detectModelsBtn->setEnabled(false);
    m_detectModelsBtn->setText("🔄 检测中...");
    
    // 创建临时的OllamaClient来检测模型
    QNetworkAccessManager *tempManager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(ollamaUrl + "/api/tags"));
    request.setTransferTimeout(5000);
    
    QNetworkReply *reply = tempManager->get(request);
    
    // 连接完成信号
    connect(reply, &QNetworkReply::finished, this, [this, reply, tempManager]() {
        // 恢复按钮状态
        m_detectModelsBtn->setEnabled(true);
        m_detectModelsBtn->setText("🔍 检测模型");
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonArray modelsArray = doc.object()["models"].toArray();
                
                if (modelsArray.isEmpty()) {
                    QMessageBox::information(this, "检测结果",
                        "Ollama服务运行正常，但未检测到已安装的模型\n\n"
                        "请先下载模型，例如：\n"
                        "ollama pull qwen2.5:7b\n"
                        "ollama pull llama3.2:3b\n"
                        "ollama pull deepseek-coder:6.7b");
                } else {
                    // 清空现有选项
                    m_ollamaModelCombo->clear();
                    
                    QStringList modelNames;
                    for (const QJsonValue &val : modelsArray) {
                        QString modelName = val.toObject()["name"].toString();
                        if (!modelName.isEmpty()) {
                            modelNames.append(modelName);
                            m_ollamaModelCombo->addItem(modelName);
                        }
                    }
                    
                    // 自动选择第一个模型
                    if (!modelNames.isEmpty()) {
                        m_ollamaModelCombo->setCurrentIndex(0);
                    }
                    
                    QMessageBox::information(this, "检测成功",
                        QString("✅ 检测到 %1 个已安装的模型：\n\n%2\n\n"
                                "已自动选择第一个模型，你也可以手动选择其他模型")
                        .arg(modelNames.size())
                        .arg(modelNames.join("\n")));
                }
            } else {
                QMessageBox::warning(this, "检测失败",
                    "无法解析Ollama服务响应\n\n"
                    "请检查Ollama服务是否正常运行");
            }
        } else {
            QString errorMsg;
            switch (reply->error()) {
                case QNetworkReply::ConnectionRefusedError:
                    errorMsg = "无法连接到Ollama服务\n\n"
                              "请检查：\n"
                              "1. Ollama是否已安装\n"
                              "2. 服务是否正在运行（ollama serve）\n"
                              "3. 服务地址是否正确";
                    break;
                case QNetworkReply::HostNotFoundError:
                    errorMsg = "找不到Ollama服务器\n\n"
                              "请检查服务地址配置是否正确";
                    break;
                case QNetworkReply::TimeoutError:
                    errorMsg = "连接超时\n\n"
                              "请检查网络连接和Ollama服务状态";
                    break;
                default:
                    errorMsg = QString("连接失败：%1\n\n"
                                      "请检查Ollama服务状态")
                              .arg(reply->errorString());
                    break;
            }
            
            QMessageBox::warning(this, "检测失败", errorMsg);
        }
        
        reply->deleteLater();
        tempManager->deleteLater();
    });
}

void SettingsDialog::onCancel()
{
    reject();
}
