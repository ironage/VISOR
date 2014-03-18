#ifndef METADATAPARSER_H
#define METADATAPARSER_H

#include <QHash>
#include <QRegExp>
#include <QString>

enum MetaDataType {
    NAME,
    LAT,
    LON,
    ALT,
    ROLL,
    PITCH,
    YAW,

    NUM_META_DATA_TYPES // always the last element for robust iteration over types
};

struct MetaData {
    MetaData() : dataIsValid(false) {}
    bool dataIsValid;
    QString imageName;
    double data [NUM_META_DATA_TYPES];
};

class MetaDataParser
{
public:
    MetaDataParser();
    void setFileName(QString fileName);
    MetaData searchForImage(QString fullImagePath);
private:
    MetaData getDataFromElements(QStringList elements);
    QString metaDataFileName;
    QHash<MetaDataType, int> hashMap;
    QRegExp regex;
};

#endif // METADATAPARSER_H
