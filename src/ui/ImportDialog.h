#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QRadioButton>
#include <QPushButton>

class ImportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImportDialog(QWidget *parent = nullptr);
    
    QString selectedPath() const;
    enum ImportMode { OneFileOneQuestion, OneFileMultipleQuestions, MixedMode };
    ImportMode importMode() const;
    
private slots:
    void onBrowse();
    
private:
    void setupUI();
    
    QLineEdit *m_pathEdit;
    QRadioButton *m_oneFileOneQuestionRadio;
    QRadioButton *m_oneFileMultipleRadio;
    QRadioButton *m_mixedRadio;
    QPushButton *m_browseBtn;
};

#endif // IMPORTDIALOG_H
