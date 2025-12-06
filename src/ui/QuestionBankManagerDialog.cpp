#include "QuestionBankManagerDialog.h"
#include "AIImportDialog.h"
#include "ImportDialog.h"
#include "../ai/OllamaClient.h"
#include "../core/ProgressManager.h"
#include "../core/QuestionBank.h"
#include "../core/Question.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

QuestionBankManagerDialog::QuestionBankManagerDialog(OllamaClient *aiClient, QWidget *parent)
    : QDialog(parent)
    , m_aiClient(aiClient)
{
    setupUI();
    refreshBankList();
    
    setWindowTitle("题库管理");
    resize(800, 600);
    
    // 连接信号
    connect(&QuestionBankManager::instance(), &QuestionBankManager::bankListChanged,
            this, &QuestionBankManagerDialog::onBankListChanged);
}

void QuestionBankManagerDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 标题
    QLabel *titleLabel = new QLabel("📚 题库管理", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #e8e8e8;");
    
    // 信息标签
    m_infoLabel = new QLabel("选择一个题库进行操作", this);
    m_infoLabel->setStyleSheet("color: #b0b0b0; font-size: 10pt;");
    
    // 题库列表
    m_bankList = new QListWidget(this);
    m_bankList->setStyleSheet(R"(
        QListWidget {
            background-color: #242424;
            color: #e8e8e8;
            border: 1px solid #4a4a4a;
            border-radius: 8px;
            padding: 8px;
        }
        QListWidget::item {
            padding: 12px;
            border-radius: 6px;
            margin: 4px 0;
        }
        QListWidget::item:hover {
            background-color: #2d2d2d;
        }
        QListWidget::item:selected {
            background-color: #660000;
            color: white;
            outline: none;
        }
        QListWidget::item:selected:hover {
            background-color: #880000;
            color: white;
        }
    )");
    
    connect(m_bankList, &QListWidget::itemSelectionChanged,
            this, &QuestionBankManagerDialog::onBankSelectionChanged);
    
    // 操作按钮
    QHBoxLayout *btnLayout1 = new QHBoxLayout();
    
    m_viewBtn = new QPushButton("👁️ 查看详情", this);
    m_refreshBtn = new QPushButton("🔄 刷新题库", this);
    m_renameBtn = new QPushButton("✏️ 重命名", this);
    
    btnLayout1->addWidget(m_viewBtn);
    btnLayout1->addWidget(m_refreshBtn);
    btnLayout1->addWidget(m_renameBtn);
    
    // 第二行操作按钮
    QHBoxLayout *btnLayout1_2 = new QHBoxLayout();
    m_deleteBtn = new QPushButton("🗑️ 删除题库", this);
    m_exportBtn = new QPushButton("📤 导出路径", this);
    
    btnLayout1_2->addWidget(m_deleteBtn);
    btnLayout1_2->addWidget(m_exportBtn);
    btnLayout1_2->addStretch();
    
    // 底部按钮
    QHBoxLayout *btnLayout2 = new QHBoxLayout();
    
    m_importBtn = new QPushButton("➕ 导入新题库", this);
    m_closeBtn = new QPushButton("关闭", this);
    
    btnLayout2->addWidget(m_importBtn);
    btnLayout2->addStretch();
    btnLayout2->addWidget(m_closeBtn);
    
    // 按钮样式
    QString btnStyle = R"(
        QPushButton {
            background-color: #660000;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #880000;
        }
        QPushButton:pressed {
            background-color: #440000;
        }
        QPushButton:disabled {
            background-color: #3a3a3a;
            color: #707070;
        }
    )";
    
    m_viewBtn->setStyleSheet(btnStyle);
    m_deleteBtn->setStyleSheet(btnStyle);
    m_refreshBtn->setStyleSheet(btnStyle);
    m_renameBtn->setStyleSheet(btnStyle);
    m_exportBtn->setStyleSheet(btnStyle);
    m_importBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);
    
    // 布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_bankList);
    mainLayout->addLayout(btnLayout1);
    mainLayout->addLayout(btnLayout1_2);
    mainLayout->addLayout(btnLayout2);
    
    // 连接信号
    connect(m_viewBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onViewBankDetails);
    connect(m_deleteBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onDeleteBank);
    connect(m_refreshBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onRefreshBank);
    connect(m_renameBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onRenameBank);
    connect(m_exportBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onExportPath);
    connect(m_importBtn, &QPushButton::clicked, this, &QuestionBankManagerDialog::onImportNewBank);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    // 应用对话框样式
    setStyleSheet(R"(
        QDialog {
            background-color: #242424;
        }
        QLabel {
            color: #e8e8e8;
        }
    )");
    
    updateButtons();
}

void QuestionBankManagerDialog::refreshBankList()
{
    m_bankList->clear();
    
    QVector<QuestionBankInfo> banks = QuestionBankManager::instance().getAllBanks();
    QString currentBankId = QuestionBankManager::instance().getCurrentBankId();
    
    if (banks.isEmpty()) {
        m_infoLabel->setText("暂无题库，点击\"导入新题库\"开始");
        return;
    }
    
    m_infoLabel->setText(QString("共有 %1 个题库").arg(banks.size()));
    
    for (QuestionBankInfo info : banks) {
        // 实时统计题目数量（确保准确）
        int actualCount = countQuestionsInDirectory(info.path);
        if (actualCount != info.questionCount) {
            // 更新题目数量
            QuestionBankManager::instance().updateQuestionCount(info.id, actualCount);
            info.questionCount = actualCount;
        }
        
        QString text = formatBankInfo(info);
        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, info.id);
        
        // 标记当前题库
        if (info.id == currentBankId) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            item->setText("⭐ " + text);
        }
        
        m_bankList->addItem(item);
    }
}

QString QuestionBankManagerDialog::formatBankInfo(const QuestionBankInfo &info) const
{
    // 计算完成度
    BankProgress progress = calculateBankProgress(info.path);
    
    QString text = QString("%1\n").arg(info.name);
    text += QString("  📊 %1 道题目").arg(info.questionCount);
    
    // 显示完成度
    if (info.questionCount > 0) {
        double completionRate = (double)progress.completedCount / info.questionCount * 100.0;
        text += QString(" | ✅ 完成度 %1% (%2/%3)")
            .arg(completionRate, 0, 'f', 1)
            .arg(progress.completedCount)
            .arg(info.questionCount);
    }
    
    text += QString(" | 📅 %1").arg(info.importTime.toString("yyyy-MM-dd hh:mm"));
    
    if (info.isAIParsed) {
        text += " | 🤖 AI";
    }
    
    return text;
}

QuestionBankManagerDialog::BankProgress QuestionBankManagerDialog::calculateBankProgress(const QString &bankPath) const
{
    BankProgress progress;
    
    // 加载题库中的所有题目
    QVector<Question> questions = loadQuestionsFromPath(bankPath);
    
    progress.totalCount = questions.size();
    
    // 统计各状态的题目数量
    for (const Question &q : questions) {
        QuestionProgressRecord record = ProgressManager::instance().getProgress(q.id());
        
        switch (record.status) {
            case QuestionStatus::NotStarted:
                progress.notStartedCount++;
                break;
            case QuestionStatus::InProgress:
                progress.inProgressCount++;
                break;
            case QuestionStatus::Completed:
                progress.completedCount++;
                break;
            case QuestionStatus::Mastered:
                progress.masteredCount++;
                progress.completedCount++; // 已掌握也算完成
                break;
        }
    }
    
    return progress;
}

QVector<Question> QuestionBankManagerDialog::loadQuestionsFromPath(const QString &dirPath) const
{
    QVector<Question> questions;
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        return questions;
    }
    
    // 递归加载所有 JSON 文件
    loadQuestionsRecursive(dirPath, questions);
    
    return questions;
}

void QuestionBankManagerDialog::loadQuestionsRecursive(const QString &dirPath, QVector<Question> &questions) const
{
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.json";
    
    // 加载当前目录的 JSON 文件
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    for (const auto &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const auto &val : arr) {
                    questions.append(Question(val.toObject()));
                }
            } else if (doc.isObject()) {
                questions.append(Question(doc.object()));
            }
            
            file.close();
        }
    }
    
    // 递归扫描子目录
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &subDirInfo : subDirs) {
        loadQuestionsRecursive(subDirInfo.absoluteFilePath(), questions);
    }
}

void QuestionBankManagerDialog::onBankListChanged()
{
    refreshBankList();
}

void QuestionBankManagerDialog::onBankSelectionChanged()
{
    QListWidgetItem *item = m_bankList->currentItem();
    if (item) {
        m_selectedBankId = item->data(Qt::UserRole).toString();
    } else {
        m_selectedBankId.clear();
    }
    
    updateButtons();
}

void QuestionBankManagerDialog::updateButtons()
{
    bool hasSelection = !m_selectedBankId.isEmpty();
    
    m_viewBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_refreshBtn->setEnabled(hasSelection);
    m_renameBtn->setEnabled(hasSelection);
    m_exportBtn->setEnabled(hasSelection);
}



void QuestionBankManagerDialog::onDeleteBank()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setText(QString("确定要删除题库\"%1\"吗？").arg(info.name));
    msgBox.setInformativeText(QString("此操作将删除程序内部的题库副本（%1 道题目）。\n\n"
                                     "注意：不会删除原始导入文件夹。")
                             .arg(info.questionCount));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    
    if (msgBox.exec() == QMessageBox::Yes) {
        if (QuestionBankManager::instance().deleteQuestionBank(m_selectedBankId)) {
            emit bankDeleted(m_selectedBankId);
            
            QMessageBox::information(this, "删除成功",
                QString("题库\"%1\"已删除").arg(info.name));
            
            m_selectedBankId.clear();
            refreshBankList();
            updateButtons();
        } else {
            QMessageBox::critical(this, "删除失败",
                "删除题库时发生错误");
        }
    }
}

void QuestionBankManagerDialog::onRefreshBank()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    // 重新统计题目数量
    int oldCount = info.questionCount;
    int newCount = countQuestionsInDirectory(info.path);
    
    if (newCount != oldCount) {
        QuestionBankManager::instance().updateQuestionCount(m_selectedBankId, newCount);
        
        QMessageBox::information(this, "刷新完成",
            QString("题库\"%1\"已刷新\n\n"
                    "原题目数量：%2 道\n"
                    "当前题目数量：%3 道\n"
                    "变化：%4%5 道")
            .arg(info.name)
            .arg(oldCount)
            .arg(newCount)
            .arg(newCount > oldCount ? "+" : "")
            .arg(newCount - oldCount));
    } else {
        QMessageBox::information(this, "刷新完成",
            QString("题库\"%1\"已刷新\n\n题目数量：%2 道（无变化）")
            .arg(info.name).arg(newCount));
    }
    
    emit bankRefreshed(m_selectedBankId);
    refreshBankList();
}

int QuestionBankManagerDialog::countQuestionsInDirectory(const QString &dirPath) const
{
    int count = 0;
    QDir dir(dirPath);
    
    if (!dir.exists()) {
        return 0;
    }
    
    // 统计当前目录的 JSON 文件
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    for (const auto &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            
            if (doc.isArray()) {
                count += doc.array().size();
            } else if (doc.isObject()) {
                count += 1;
            }
            
            file.close();
        }
    }
    
    // 递归统计子目录
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &subDirInfo : subDirs) {
        count += countQuestionsInDirectory(subDirInfo.absoluteFilePath());
    }
    
    return count;
}

void QuestionBankManagerDialog::onRenameBank()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    bool ok;
    QString newName = QInputDialog::getText(this, "重命名题库",
                                           "新名称:", QLineEdit::Normal,
                                           info.name, &ok);
    
    if (ok && !newName.isEmpty() && newName != info.name) {
        if (QuestionBankManager::instance().renameQuestionBank(m_selectedBankId, newName)) {
            QMessageBox::information(this, "重命名成功",
                QString("题库已重命名为：%1").arg(newName));
            
            refreshBankList();
        }
    }
}

void QuestionBankManagerDialog::onViewBankDetails()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    // 获取题库统计信息
    int questionCount = countQuestionsInDirectory(info.path);
    QDir bankDir(info.path);
    
    // 统计子目录数量
    int subDirCount = bankDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
    
    // 统计文件数量
    int fileCount = 0;
    countFilesRecursive(info.path, fileCount);
    
    // 计算完成度
    BankProgress progress = calculateBankProgress(info.path);
    double completionRate = questionCount > 0 ? (double)progress.completedCount / questionCount * 100.0 : 0.0;
    
    // 获取绝对路径用于显示
    QDir currentDir;
    QString absolutePath = currentDir.absoluteFilePath(info.path);
    QString absoluteOriginalPath = info.originalPath.isEmpty() ? "无" : currentDir.absoluteFilePath(info.originalPath);
    
    // 构建详情信息
    QString details = QString(
        "� 题库名称：：%1\n\n"
        "【题目统计】\n"
        "📊 总题目数：%2 道\n"
        "✅ 已完成：%3 道 (%4%)\n"
        "⭐ 已掌握：%5 道\n"
        "🔵 进行中：%6 道\n"
        "⚪ 未开始：%7 道\n\n"
        "【文件信息】\n"
        "📁 文件数量：%8 个\n"
        "📂 子目录数：%9 个\n\n"
        "【时间信息】\n"
        "📅 导入时间：%10\n"
        "🕐 最后访问：%11\n\n"
        "【其他信息】\n"
        "🤖 AI解析：%12\n"
        "📍 存储路径：\n%13\n\n"
        "💾 原始路径：\n%14"
    )
    .arg(info.name)
    .arg(questionCount)
    .arg(progress.completedCount)
    .arg(completionRate, 0, 'f', 1)
    .arg(progress.masteredCount)
    .arg(progress.inProgressCount)
    .arg(progress.notStartedCount)
    .arg(fileCount)
    .arg(subDirCount)
    .arg(info.importTime.toString("yyyy-MM-dd hh:mm:ss"))
    .arg(info.lastAccessTime.toString("yyyy-MM-dd hh:mm:ss"))
    .arg(info.isAIParsed ? "是" : "否")
    .arg(absolutePath)
    .arg(absoluteOriginalPath);
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("题库详情");
    msgBox.setText(details);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void QuestionBankManagerDialog::onExportPath()
{
    if (m_selectedBankId.isEmpty()) return;
    
    QuestionBankInfo info = QuestionBankManager::instance().getBankInfo(m_selectedBankId);
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("题库路径");
    msgBox.setText(QString("题库【%1】的存储路径：").arg(info.name));
    msgBox.setInformativeText(info.path);
    msgBox.setIcon(QMessageBox::Information);
    
    QPushButton *copyBtn = msgBox.addButton("📋 复制路径", QMessageBox::ActionRole);
    QPushButton *openBtn = msgBox.addButton("📂 打开文件夹", QMessageBox::ActionRole);
    QPushButton *closeBtn = msgBox.addButton("关闭", QMessageBox::RejectRole);
    
    msgBox.setDefaultButton(closeBtn);
    msgBox.exec();
    
    if (msgBox.clickedButton() == copyBtn) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(info.path);
        QMessageBox::information(this, "复制成功", "路径已复制到剪贴板");
    } else if (msgBox.clickedButton() == openBtn) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.path));
    }
}

void QuestionBankManagerDialog::countFilesRecursive(const QString &dirPath, int &count) const
{
    QDir dir(dirPath);
    if (!dir.exists()) return;
    
    // 统计当前目录的文件
    QFileInfoList files = dir.entryInfoList(QDir::Files);
    count += files.size();
    
    // 递归统计子目录
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &subDirInfo : subDirs) {
        countFilesRecursive(subDirInfo.absoluteFilePath(), count);
    }
}

void QuestionBankManagerDialog::onImportNewBank()
{
    // 关闭当前对话框，让MainWindow处理导入
    accept();
    // 用户需要通过菜单导入新题库
}
