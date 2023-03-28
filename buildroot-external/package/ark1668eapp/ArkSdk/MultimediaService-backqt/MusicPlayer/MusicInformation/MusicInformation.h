#ifndef MUSICINFORMATION_H
#define MUSICINFORMATION_H

#include <QScopedPointer>
#include "attachedpictureframe.h"
#include "mpegfile.h"
#include "id3v2tag.h"
#include "fileref.h"
#include "tag.h"
#include "flacmetadatablock.h"
#include "flacfile.h"
#include "tstring.h"
#include "mp4file.h"
#include "mp4item.h"
#include "mp4coverart.h"
#include "mp4tag.h"
#include "apefile.h"
#include "apetag.h"
#include "apeitem.h"
#include "oggfile.h"
#include "vorbisfile.h"
#include "xiphcomment.h"
class MusicInformationPrivate;
class MusicInformation
{
    Q_DISABLE_COPY(MusicInformation)
public:
    MusicInformation();
    ~MusicInformation();
    void parserFilePath(const QString &path);
    QString getTitle();
    QString getArtist();
    QString getAlbum();
private:
    friend class MusicInformationPrivate;
    QScopedPointer<MusicInformationPrivate> m_Private;
};

#endif // MUSICINFORMATION_H
