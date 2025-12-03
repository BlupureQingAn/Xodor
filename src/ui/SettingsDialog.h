#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    
private slots:
    void onBrowseCompiler();
    void onTestCompiler();
    void onDetectCompiler();
    void onSave();
    void onCancel();
    
private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    
    // 编译器设置
    QLineEdit *m_compilerPathEdit;
    QPushButton *m_browseCompilerBtn;
    QPushButton *m_testCompilerBtn;
    QPushButton *m_detectCompilerBtn;
    
    // AI设置
    QTabWidget *m_aiTabWidget;
    QLineEdit *m_ollamaUrlEdit;
    QLineEdit *m_ollamaModelEdit;
    QLineEdit *m_cloudApiKeyEdit;
    
    // 编辑器设置
    QComboBox *m_fontSizeCombo;
    QComboBox *m_tabWidthCombo;
    
    // 按钮
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // SETTINGSDIALOG_H
