#ifndef COMPILERDETECTOR_H
#define COMPILERDETECTOR_H

#include <QString>
#include <QStringList>

struct CompilerInfo {
    QString path;
    QString name;
    QString version;
    bool isValid;
};

class CompilerDetector
{
public:
    static QList<CompilerInfo> detectCompilers();
    static CompilerInfo detectBestCompiler();
    static bool validateCompiler(const QString &path);
    static QString getCompilerVersion(const QString &path);
    
private:
    static QStringList getSearchPaths();
    static CompilerInfo testCompiler(const QString &path);
};

#endif // COMPILERDETECTOR_H
