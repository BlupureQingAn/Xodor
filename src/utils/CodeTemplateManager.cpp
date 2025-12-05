#include "CodeTemplateManager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

CodeTemplateManager& CodeTemplateManager::instance()
{
    static CodeTemplateManager inst;
    return inst;
}

CodeTemplateManager::CodeTemplateManager()
{
    initDefaultTemplates();
    loadTemplates();
}

void CodeTemplateManager::initDefaultTemplates()
{
    // 基础模板（移除第一行注释，避免行号偏移）
    m_templates["基础模板"] = 
        "#include <iostream>\n"
        "using namespace std;\n"
        "\n"
        "int main() {\n"
        "    \n"
        "    return 0;\n"
        "}\n";
    
    // 输入输出模板
    m_templates["输入输出模板"] = 
        "#include <iostream>\n"
        "using namespace std;\n"
        "\n"
        "int main() {\n"
        "    int n;\n"
        "    cin >> n;\n"
        "    \n"
        "    // 处理输入\n"
        "    \n"
        "    cout << result << endl;\n"
        "    return 0;\n"
        "}\n";
    
    // 数组处理模板
    m_templates["数组处理模板"] = 
        "#include <iostream>\n"
        "#include <vector>\n"
        "using namespace std;\n"
        "\n"
        "int main() {\n"
        "    int n;\n"
        "    cin >> n;\n"
        "    \n"
        "    vector<int> arr(n);\n"
        "    for (int i = 0; i < n; i++) {\n"
        "        cin >> arr[i];\n"
        "    }\n"
        "    \n"
        "    // 处理数组\n"
        "    \n"
        "    return 0;\n"
        "}\n";
    
    // 字符串处理模板
    m_templates["字符串处理模板"] = 
        "#include <iostream>\n"
        "#include <string>\n"
        "using namespace std;\n"
        "\n"
        "int main() {\n"
        "    string str;\n"
        "    getline(cin, str);\n"
        "    \n"
        "    // 处理字符串\n"
        "    \n"
        "    cout << result << endl;\n"
        "    return 0;\n"
        "}\n";
    
    // 算法模板
    m_templates["算法模板"] = 
        "#include <iostream>\n"
        "#include <vector>\n"
        "#include <algorithm>\n"
        "using namespace std;\n"
        "\n"
        "int main() {\n"
        "    // 读取输入\n"
        "    \n"
        "    // 算法处理\n"
        "    \n"
        "    // 输出结果\n"
        "    \n"
        "    return 0;\n"
        "}\n";
}

QStringList CodeTemplateManager::templateNames() const
{
    return m_templates.keys();
}

QString CodeTemplateManager::getTemplate(const QString &name) const
{
    return m_templates.value(name, m_templates["基础模板"]);
}

void CodeTemplateManager::addTemplate(const QString &name, const QString &code)
{
    m_templates[name] = code;
    saveTemplates();
}

void CodeTemplateManager::removeTemplate(const QString &name)
{
    m_templates.remove(name);
    saveTemplates();
}

void CodeTemplateManager::loadTemplates()
{
    QFile file("data/templates.json");
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject json = doc.object();
    for (auto it = json.begin(); it != json.end(); ++it) {
        m_templates[it.key()] = it.value().toString();
    }
}

void CodeTemplateManager::saveTemplates()
{
    QDir dir("data");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonObject json;
    for (auto it = m_templates.begin(); it != m_templates.end(); ++it) {
        json[it.key()] = it.value();
    }
    
    QFile file("data/templates.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
        file.close();
    }
}
