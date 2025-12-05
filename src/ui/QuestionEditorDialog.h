#ifndef QUESTIONEDITORDIALOG_H
#define QUESTIONEDITORDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QPushButton>
#include "../core/Question.h"

// 测试用例编辑项
class TestCaseItem : public QWidget
{
    Q_OBJECT
public:
    explicit TestCaseItem(int index, QWidget *parent = nullptr);
    
    void setTestCase(const TestCase &testCase);
    TestCase getTestCase() const;
    
signals:
    void removeRequested();
    
private:
    int m_index;
    QTextEdit *m_inputEdit;
    QTextEdit *m_outputEdit;
    QPushButton *m_removeButton;
};

// 题目编辑对话框
class QuestionEditorDialog : public QDialog
{
    Q_OBJECT
    
public:
    enum Mode {
        CreateMode,     // 创建新题目
        EditMode,       // 编辑现有题目
        ImportMode      // 从文件导入
    };
    
    explicit QuestionEditorDialog(Mode mode, QWidget *parent = nullptr);
    explicit QuestionEditorDialog(const Question &question, QWidget *parent = nullptr);
    
    // 获取编辑后的题目
    Question getQuestion() const;
    
    // 设置题目（用于编辑模式）
    void setQuestion(const Question &question);
    
    // 从文件导入
    void importFromFile(const QString &filePath);
    
public slots:
    void onImportFromFile();
    
private slots:
    void onAddTestCase();
    void onRemoveTestCase(int index);
    void onAccept();
    void onCancel();
    
private:
    void setupUI();
    void setupConnections();
    void updateTestCaseIndices();
    bool validateInput();
    QString generateQuestionId() const;
    
    Mode m_mode;
    
    // 基本信息
    QLineEdit *m_titleEdit;
    QComboBox *m_difficultyCombo;
    QLineEdit *m_tagsEdit;
    QTextEdit *m_descriptionEdit;
    
    // 限制条件
    QSpinBox *m_timeLimitSpin;
    QSpinBox *m_memoryLimitSpin;
    
    // 测试用例
    QListWidget *m_testCaseList;
    QPushButton *m_addTestCaseButton;
    
    // 按钮
    QPushButton *m_importButton;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    
    // 数据
    Question m_question;
};

#endif // QUESTIONEDITORDIALOG_H
