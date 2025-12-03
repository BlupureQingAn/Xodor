#include "HistoryWidget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void HistoryWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    m_historyTable = new QTableWidget(this);
    m_historyTable->setColumnCount(5);
    m_historyTable->setHorizontalHeaderLabels(
        {"题目", "难度", "状态", "提交时间", "用时"});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    
    layout->addWidget(m_historyTable);
}

void HistoryWidget::loadHistory()
{
    m_historyTable->setRowCount(0);
    
    QDir dir("data/user_answers");
    if (!dir.exists()) return;
    
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    for (const auto &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject obj = doc.object();
            
            int row = m_historyTable->rowCount();
            m_historyTable->insertRow(row);
            
            m_historyTable->setItem(row, 0, new QTableWidgetItem(obj["questionId"].toString()));
            m_historyTable->setItem(row, 1, new QTableWidgetItem("未知"));
            m_historyTable->setItem(row, 2, new QTableWidgetItem("已保存"));
            m_historyTable->setItem(row, 3, new QTableWidgetItem(obj["lastModified"].toString()));
            
            QString code = obj["code"].toString();
            int lines = code.split('\n').count();
            m_historyTable->setItem(row, 4, new QTableWidgetItem(QString::number(lines) + " 行"));
            
            file.close();
        }
    }
}
