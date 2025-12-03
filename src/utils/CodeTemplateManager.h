#ifndef CODETEMPLATEMANAGER_H
#define CODETEMPLATEMANAGER_H

#include <QString>
#include <QMap>

class CodeTemplateManager
{
public:
    static CodeTemplateManager& instance();
    
    QStringList templateNames() const;
    QString getTemplate(const QString &name) const;
    void addTemplate(const QString &name, const QString &code);
    void removeTemplate(const QString &name);
    
    void loadTemplates();
    void saveTemplates();
    
private:
    CodeTemplateManager();
    CodeTemplateManager(const CodeTemplateManager&) = delete;
    CodeTemplateManager& operator=(const CodeTemplateManager&) = delete;
    
    void initDefaultTemplates();
    
    QMap<QString, QString> m_templates;
};

#endif // CODETEMPLATEMANAGER_H
