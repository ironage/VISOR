#include "metadataparser.h"

#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>
#include <iostream>

MetaDataParser::MetaDataParser() : regex("[\\s,]+") {
}

void printStringList(QStringList list) {
    foreach (QString item, list) {
        std::cout << item.toStdString() << " ";
    }
    std::cout << std::endl;
}

int findIndexForString(QStringList haystack, QStringList needles) {
    foreach (QString needle, needles) {
        for (int i = 0; i < haystack.size(); ++i) {
        if (haystack.at(i).compare(needle, Qt::CaseInsensitive) == 0) {
                return i;
            }
        }
    }

    std::cout << "Error in MetaDataParser, could not find these words:\n";
    printStringList(needles);

    return -1;
}

void MetaDataParser::setFileName(QString fileName) {
    metaDataFileName = fileName;

    QFile file(metaDataFileName);
    hashMap.clear();

    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        std::cout << "Could not open meta data for reading or file does not exist! " << metaDataFileName.toStdString() << std::endl;
        return;
    }

    QByteArray firstLineBytes = file.readLine();
    QString firstLine(firstLineBytes);
    // \W matches non-word character (for example: space, comma, period etc)
    QStringList list = firstLine.split(regex, QString::SkipEmptyParts);
    foreach (QString item, list) {
        std::cout << "item: " << item.toStdString() << std::endl;
    }

    QStringList idWords, latWords, lonWords, altWords, rollWords, pitchWords, yawWords;
    idWords << "filename" << "image";
    latWords << "lat" << "latitude";
    lonWords << "lon" << "longitude";
    altWords << "alt" << "altitude";
    rollWords << "roll";
    pitchWords << "pitch";
    yawWords << "heading" << "yaw";

    hashMap[NAME] = findIndexForString(list, idWords);
    hashMap[LAT] = findIndexForString(list, latWords);
    hashMap[LON] = findIndexForString(list, lonWords);
    hashMap[ALT] = findIndexForString(list, altWords);
    hashMap[ROLL] = findIndexForString(list, rollWords);
    hashMap[PITCH] = findIndexForString(list, pitchWords);
    hashMap[YAW] = findIndexForString(list, yawWords);

    file.close();
}

MetaData MetaDataParser::searchForImage(QString fullImagePath) {
    QFileInfo info(fullImagePath);
    QString imageName = info.fileName(); // strips off the path info
    MetaData data;  // All fields are currently uninitialized.
    data.dataIsValid = false;

    QFile file(metaDataFileName);

    if (!file.exists() || !file.open(QIODevice::ReadOnly)) return data;

    QTextStream in(&file);
    while ( !in.atEnd() )
    {
        QString line = in.readLine();
        QStringList elements = line.split(regex, QString::SkipEmptyParts);
        int elementSize = elements.size();
        if (elementSize > 0) {
            if (hashMap[NAME] < elementSize) {
                QString lineName = elements.at(hashMap[NAME]);
                if (lineName.compare(imageName, Qt::CaseInsensitive) == 0) {    // found the right line
                    file.close();
                    return getDataFromElements(elements);
                }
            }
        }
    }
    file.close();
    return data;
}

MetaData MetaDataParser::getDataFromElements(QStringList elements) {
    MetaData data;
    data.dataIsValid = false;

    int len = elements.length();

    for (int i = 0; i < NUM_META_DATA_TYPES; ++i) {
        MetaDataType type = (MetaDataType)i;
        if (hashMap[type] < len) {
            if (type == NAME) {
                data.imageName = elements.at(hashMap[type]);
            } else {
                data.data[type] = elements.at(hashMap[type]).toDouble();
            }
        } else {
            std::cout << "could not find meta data (" << type << ") in line containing elements: ";
            printStringList(elements);
            return data;
        }
    }
    data.dataIsValid = true;
    return data;
}








