#ifndef IAPPLICATIONMETA_H
#define IAPPLICATIONMETA_H

#include <QString>

typedef QString ApplicationVersion;


class IApplicationMeta
{
public:
    IApplicationMeta() {}
    virtual ~IApplicationMeta() { } //qDebug() << "IApplicationMeta destructor"; }

    // combination of applicationId + version -> unique id
    virtual QString id() const = 0;

    virtual QString applicationId() const = 0;
    virtual ApplicationVersion version() const = 0;
    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QString applicationDirPath() const = 0;
    virtual QString applicationExecutablePath() const = 0;
    virtual QString catalogImageFilePath() const = 0;
};

#endif // IAPPLICATIONMETA_H

