#include "ImportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDialogButtonBox>

ImportDialog::ImportDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("导入题库");
}

void ImportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("题库路径:", this));
    m_pathEdit = new QLineEdit(this);
    m_browseBtn = new QPushButton("浏览...", this);
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(m_browseBtn);
    
    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(new QLabel("导入模式:", this));
    
    m_oneFileOneQuestionRadio = new QRadioButton("每个文件一道题", this);
    m_oneFileMultipleRadio = new QRadioButton("每个文件一套题", this);
    m_mixedRadio = new QRadioButton("混合模式（AI自动识别）", this);
    m_mixedRadio->setChecked(true);
    
    mainLayout->addWidget(m_oneFileOneQuestionRadio);
    mainLayout->addWidget(m_oneFileMultipleRadio);
    mainLayout->addWidget(m_mixedRadio);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    
    connect(m_browseBtn, &QPushButton::clicked, this, &ImportDialog::onBrowse);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ImportDialog::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择题库文件夹", "", QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

QString ImportDialog::selectedPath() const
{
    return m_pathEdit->text();
}

ImportDialog::ImportMode ImportDialog::importMode() const
{
    if (m_oneFileOneQuestionRadio->isChecked())
        return OneFileOneQuestion;
    if (m_oneFileMultipleRadio->isChecked())
        return OneFileMultipleQuestions;
    return MixedMode;
}
