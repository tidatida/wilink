/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
 * See AUTHORS file for a full list of contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QCache>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QLayout>
#include <QThread>

#include "QXmppClient.h"

#include "application.h"
#include "declarative.h"
#include "photos.h"

static const QSize UPLOAD_SIZE(2048, 2048);

static PhotoCache *photoCache = 0;
static QCache<QString, QImage> photoImageCache;
static bool photoInitialised = false;

class PhotoNetworkAccessManagerFactory : public FileSystemNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent)
    {
        return new NetworkAccessManager(parent);
    };
};

class PhotoDownloadItem
{
public:
    FileSystem *fs;
    FileSystem::ImageSize type;
    QUrl url;
};

enum PhotoRole
{
    ImageRole = ChatModel::UserRole,
    ImageReadyRole,
    IsDirRole,
    SizeRole,
    UrlRole,
};

PhotoCache::PhotoCache()
    : m_downloadItem(0),
    m_downloadJob(0)
{
}

/** When a download finishes, process the results.
 */
void PhotoCache::_q_jobFinished()
{
    Q_ASSERT(m_downloadItem);
    Q_ASSERT(m_downloadJob);

    if (m_downloadJob->error() == FileSystemJob::NoError) {
        // load image
        QImage *image = new QImage;
        image->load(m_downloadJob->data(), NULL);
        photoImageCache.insert(QString::number(m_downloadItem->type) + m_downloadItem->url.toString(), image);
        emit photoChanged(m_downloadItem->url, m_downloadItem->type);
    }

    m_downloadJob->deleteLater();
    m_downloadJob = 0;
    delete m_downloadItem;
    m_downloadItem = 0;

    processQueue();
}

bool PhotoCache::imageReady(const QUrl &url, FileSystem::ImageSize type) const
{
    const QString key = QString::number(type) + url.toString();
    return photoImageCache.contains(key);
}

QUrl PhotoCache::imageUrl(const FileInfo &info, FileSystem::ImageSize type, FileSystem *fs)
{
    if (info.isDir())
        return wApp->qmlUrl("128x128/album.png");

    const QString mimeType = info.mimeType();
    if (mimeType.startsWith("audio/"))
        return wApp->qmlUrl("128x128/audio-x-generic.png");
    else if (mimeType.startsWith("text/"))
        return wApp->qmlUrl("128x128/text-x-generic.png");
    else if (mimeType.startsWith("video/"))
        return wApp->qmlUrl("128x128/video-x-generic.png");
    else if (!mimeType.startsWith("image/"))
        return wApp->qmlUrl("128x128/file.png");

    const QUrl url = info.url();
    const QString key = QString::number(type) + url.toString();
    if (photoImageCache.contains(key)) {
        QUrl cacheUrl("image://photo");
        cacheUrl.setPath("/" + key);
        return cacheUrl;
    }

    // check if the url is already queued
    PhotoDownloadItem *job = 0;
    foreach (PhotoDownloadItem *ptr, m_downloadQueue) {
        if (ptr->url == url && ptr->type == type) {
            job = ptr;
            break;
        }
    }
    if (!job) {
        job = new PhotoDownloadItem;
        job->fs = fs;
        job->type = type;
        job->url = url;
        m_downloadQueue.append(job);
        processQueue();
    }

    // try returning a lower resolution
    for (int i = type - 1; i >= FileSystem::SmallSize; --i) {
        const QString key = QString::number(i) + url.toString();
        if (photoImageCache.contains(key)) {
            QUrl cacheUrl("image://photo");
            cacheUrl.setPath("/" + key);
            return cacheUrl;
        }
    }
    return wApp->qmlUrl("128x128/image-x-generic.png");
}

PhotoCache *PhotoCache::instance()
{
    if (!photoCache)
        photoCache = new PhotoCache;
    return photoCache;
}

void PhotoCache::processQueue()
{
    if (m_downloadJob || m_downloadQueue.isEmpty())
        return;

    PhotoDownloadItem *job = m_downloadQueue.takeFirst();
    if (!m_fileSystems.contains(job->fs)) {
        m_fileSystems << job->fs;
    }
    m_downloadItem = job;
    m_downloadJob = job->fs->get(job->url, job->type);
    connect(m_downloadJob, SIGNAL(finished()),
            this, SLOT(_q_jobFinished()));
}

PhotoImageProvider::PhotoImageProvider()
    : QDeclarativeImageProvider(Image)
{
}

QImage PhotoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage image;
    QImage *cached = photoImageCache.object(id);
    if (cached) {
        image = *cached;
    } else {
        qWarning("Could not get photo for %s", qPrintable(id));
        image = QImage(":/128x128/file.png");
    }

    if (requestedSize.isValid())
        image = image.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);

    if (size)
        *size = image.size();
    return image;
}

class PhotoItem : public ChatModelItem, public FileInfo
{
public:
    PhotoItem(const FileInfo &info) : FileInfo(info) {};
};

PhotoModel::PhotoModel(QObject *parent)
    : ChatModel(parent)
    , m_fs(0)
    , m_listJob(0)
    , m_permissions(FileSystemJob::None)
    , m_showFiles(true)
{
    bool check;
    Q_UNUSED(check);

    QHash<int, QByteArray> names = roleNames();
    names.insert(ImageRole, "image");
    names.insert(ImageReadyRole, "imageReady");
    names.insert(IsDirRole, "isDir");
    names.insert(SizeRole, "size");
    names.insert(UrlRole, "url");
    setRoleNames(names);

    m_uploads = new PhotoQueueModel(this);

    check = connect(PhotoCache::instance(), SIGNAL(photoChanged(QUrl,FileSystem::ImageSize)),
                    this, SLOT(_q_photoChanged(QUrl,FileSystem::ImageSize)));
    Q_ASSERT(check);
}

QVariant PhotoModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(m_fs);
    PhotoItem *item = static_cast<PhotoItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        return PhotoCache::instance()->imageUrl(*item, FileSystem::SmallSize, m_fs);
    }
    else if (role == ImageRole) {
        return PhotoCache::instance()->imageUrl(*item, FileSystem::LargeSize, m_fs);
    }
    else if (role == ImageReadyRole)
        return PhotoCache::instance()->imageReady(item->url(), FileSystem::LargeSize);
    else if (role == IsDirRole)
        return item->isDir();
    else if (role == NameRole)
        return item->name();
    else if (role == SizeRole)
        return item->size();
    else if (role == UrlRole)
        return item->url();
    return QVariant();
}

bool PhotoModel::canCreateAlbum() const
{
    return m_permissions & FileSystemJob::Mkdir;
}

bool PhotoModel::canUpload() const
{
    return m_permissions & FileSystemJob::Put;
}

void PhotoModel::createAlbum(const QString &name)
{
    if (name.isEmpty())
        return;

    QUrl newUrl = m_rootUrl;
    newUrl.setPath(newUrl.path() + "/" + name);
    m_fs->mkdir(newUrl);
}

void PhotoModel::download(int row)
{
    if (row < 0 || row > rootItem->children.size() - 1)
        return;

    PhotoItem *item = static_cast<PhotoItem*>(rootItem->children.at(row));
    m_uploads->download(*item, m_fs);
}

/** Refresh the contents of the current folder.
 */
void PhotoModel::refresh()
{
    removeRows(0, rootItem->children.size());
    m_listJob = m_fs->list(m_rootUrl);
}

QUrl PhotoModel::rootUrl() const
{
    return m_rootUrl;
}

void PhotoModel::setRootUrl(const QUrl &rootUrl)
{
    bool check;
    Q_UNUSED(check);

    if (rootUrl == m_rootUrl)
        return;

    m_rootUrl = rootUrl;
    m_fs = m_fileSystems.value(rootUrl.scheme());
    if (m_fs) {
        refresh();
    } else {
        if (!photoInitialised) {
            FileSystem::setNetworkAccessManagerFactory(new PhotoNetworkAccessManagerFactory);
            photoInitialised = true;
        }

        m_fs = FileSystem::create(m_rootUrl, this);
        if (m_fs) {
            check = connect(m_fs, SIGNAL(jobFinished(FileSystemJob*)),
                            this, SLOT(_q_jobFinished(FileSystemJob*)));
            Q_ASSERT(check);

            m_fileSystems.insert(m_rootUrl.scheme(), m_fs);
            m_fs->open(m_rootUrl);
        } else {
            removeRows(0, rootItem->children.size());
        }
    }
    emit rootUrlChanged(m_rootUrl);
}

bool PhotoModel::removeRow(int row)
{
    if (row < 0 || row >= rootItem->children.size())
        return false;

    PhotoItem *item = static_cast<PhotoItem*>(rootItem->children.at(row));
    if (item->isDir())
        m_fs->rmdir(item->url());
    else
        m_fs->remove(item->url());
    return true;
}

bool PhotoModel::showFiles() const
{
    return m_showFiles;
}

void PhotoModel::setShowFiles(bool show)
{
    if (show != m_showFiles) {
        m_showFiles = show;
        emit showFilesChanged();
    }
}

void PhotoModel::upload(const QString &filePath)
{
    QString base = m_rootUrl.toString();
    while (base.endsWith("/"))
        base.chop(1);

    m_uploads->append(filePath, m_fs, base + "/" + QFileInfo(filePath).fileName());
}

PhotoQueueModel *PhotoModel::uploads() const
{
    return m_uploads;
}

/** When a command finishes, process its results.
 *
 * @param job
 */
void PhotoModel::_q_jobFinished(FileSystemJob *job)
{
    Q_ASSERT(m_fs);

    if (job->error() != FileSystemJob::NoError) {
        qWarning() << job->operationName() << "command failed";
        return;
    }

    switch (job->operation())
    {
    case FileSystemJob::Open:
    case FileSystemJob::Mkdir:
    case FileSystemJob::Put:
    case FileSystemJob::Remove:
    case FileSystemJob::Rmdir:
        refresh();
        break;
    case FileSystemJob::List:
        if (job == m_listJob) {
            m_permissions = job->allowedOperations();
            emit permissionsChanged();

            removeRows(0, rootItem->children.size());
            foreach (const FileInfo& info, job->results()) {
                if (info.isDir() || m_showFiles) {
                    PhotoItem *item = new PhotoItem(info);
                    addItem(item, rootItem);
                }
            }
            m_listJob = 0;
        }
        break;
    default:
        break;
    }
}

/** When a photo changes, emit notifications.
 */
void PhotoModel::_q_photoChanged(const QUrl &url, FileSystem::ImageSize size)
{
    Q_UNUSED(size);
    foreach (ChatModelItem *ptr, rootItem->children) {
        PhotoItem *item = static_cast<PhotoItem*>(ptr);
        if (item->url() == url) {
            emit dataChanged(createIndex(item), createIndex(item));
        }
    }
}

class PhotoQueueItem : public ChatModelItem
{
public:
    PhotoQueueItem();

    FileInfo info;
    FileInfoList items;
    QString sourcePath;
    FileSystem *fileSystem;
    bool finished;
    bool isUpload;

    FileSystemJob *job;
    qint64 jobDoneBytes;
    qint64 jobTotalBytes;

    qint64 doneBytes;
    qint64 doneFiles;
    qint64 totalBytes;
    qint64 totalFiles;
};

PhotoQueueItem::PhotoQueueItem()
    : fileSystem(0)
    , finished(false)
    , isUpload(false)
    , job(0)
    , jobDoneBytes(0)
    , jobTotalBytes(0)
    , doneBytes(0)
    , doneFiles(0)
    , totalBytes(0)
    , totalFiles(0)
{
}

PhotoQueueModel::PhotoQueueModel(QObject *parent)
    : ChatModel(parent)
    , m_resizer(0)
    , m_resizerThread(0)
    , m_downloadItem(0)
    , m_uploadDevice(0)
    , m_uploadItem(0)
{
    // set role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(IsDirRole, "isDir");
    names.insert(SpeedRole, "speed");
    names.insert(DoneBytesRole, "doneBytes");
    names.insert(DoneFilesRole, "doneFiles");
    names.insert(TotalBytesRole, "totalBytes");
    names.insert(TotalFilesRole, "totalFiles");
    setRoleNames(names);

    m_resizerThread = new QThread;
    m_resizerThread->start();

    m_resizer = new PhotoResizer;
    m_resizer->moveToThread(m_resizerThread);
    connect(m_resizer, SIGNAL(finished(QIODevice*)),
            this, SLOT(_q_uploadResized(QIODevice*)));
}

PhotoQueueModel::~PhotoQueueModel()
{
    m_resizerThread->quit();
    m_resizerThread->wait();
    delete m_resizer;
    delete m_resizerThread;
}

void PhotoQueueModel::append(const QString &filePath, FileSystem *fileSystem, const QUrl &url)
{
    PhotoQueueItem *item = new PhotoQueueItem;

    item->isUpload = true;
    item->info.setName(QFileInfo(filePath).fileName());
    item->info.setUrl(url);
    item->sourcePath = filePath;
    item->fileSystem = fileSystem;
    item->totalFiles = 1;
    addItem(item, rootItem);

    processQueue();
}

void PhotoQueueModel::download(const FileInfo &info, FileSystem *fileSystem)
{
    bool check;
    Q_UNUSED(check);

    PhotoQueueItem *item = new PhotoQueueItem;
    item->fileSystem = fileSystem;
    item->info = info;

    if (info.isDir()) {
        item->job = item->fileSystem->list(info.url());

        check = connect(item->job, SIGNAL(finished()),
                        this, SLOT(_q_listFinished()));
        Q_ASSERT(check);
    } else {
        item->items << info;
        item->totalBytes = info.size();
        item->totalFiles = 1;
    }
    
    addItem(item, rootItem);
    processQueue();
}

void PhotoQueueModel::cancel(int row)
{
    if (row < 0 || row > rootItem->children.size() - 1)
        return;

    PhotoQueueItem *item = static_cast<PhotoQueueItem*>(rootItem->children.at(row));
    if (item->job)
        item->job->abort();
    else
        removeRow(row);
}

QVariant PhotoQueueModel::data(const QModelIndex &index, int role) const
{
    PhotoQueueItem *item = static_cast<PhotoQueueItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        // FIXME: using a thumbnail for large pictures
        // is a total performance killer
        //return QUrl::fromLocalFile(item->sourcePath);
        return wApp->qmlUrl("file.png");
    } else if (role == NameRole) {
        return item->info.name();
    } else if (role == SpeedRole) {
        // FIXME: implement speed calculation
        return 0;
    } else if (role == DoneBytesRole) {
        return item->doneBytes + item->jobDoneBytes;
    } else if (role == DoneFilesRole) {
        return item->doneFiles;
    } else if (role == TotalBytesRole) {
        return item->totalBytes;
    } else if (role == TotalFilesRole) {
        return item->totalFiles;
    }
    return QVariant();
}

void PhotoQueueModel::processQueue()
{
    bool check;
    Q_UNUSED(check);

    // process downloads
    if (!m_downloadItem) {
        foreach (ChatModelItem *ptr, rootItem->children) {
            PhotoQueueItem *item = static_cast<PhotoQueueItem*>(ptr);
            if (!item->isUpload && !item->items.isEmpty()) {
                FileInfo childInfo = item->items.takeFirst();

                m_downloadItem = item;
                m_downloadItem->job = item->fileSystem->get(childInfo.url(), FileSystem::FullSize);
                m_downloadItem->jobTotalBytes = childInfo.size();

                check = connect(m_downloadItem->job, SIGNAL(finished()),
                                this, SLOT(_q_downloadFinished()));
                Q_ASSERT(check);

                check = connect(m_downloadItem->job, SIGNAL(downloadProgress(qint64,qint64)),
                                this, SLOT(_q_downloadProgress(qint64,qint64)));
                Q_ASSERT(check);
                break;
            }
        }
    }

    // process uploads
    if (!m_uploadItem) {
        foreach (ChatModelItem *ptr, rootItem->children) {
            PhotoQueueItem *item = static_cast<PhotoQueueItem*>(ptr);
            if (item->isUpload && !item->finished) {
                m_uploadItem = item;
                QMetaObject::invokeMethod(m_resizer, "resize", Q_ARG(QString, item->sourcePath));
                break;
            }
        }
    }
}

/** When an download finishes, process the results.
 */
void PhotoQueueModel::_q_downloadFinished()
{
    bool check;
    Q_UNUSED(check);

    if (m_downloadItem) {
        m_downloadItem->job->deleteLater();
        m_downloadItem->doneBytes += m_downloadItem->jobTotalBytes;
        m_downloadItem->doneFiles++;
        m_downloadItem->jobDoneBytes = 0;

        if (m_downloadItem->items.isEmpty())
            removeItem(m_downloadItem);

        m_downloadItem = 0;
    }

    processQueue();
}

/** When upload progress changes, emit notifications.
 */
void PhotoQueueModel::_q_downloadProgress(qint64 done, qint64 total)
{
    if (m_downloadItem) {
        m_downloadItem->jobDoneBytes = done;
        emit dataChanged(createIndex(m_downloadItem), createIndex(m_downloadItem));
    }
}

void PhotoQueueModel::_q_listFinished()
{
    FileSystemJob *job = qobject_cast<FileSystemJob*>(sender());
    if (!job)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        PhotoQueueItem *item = static_cast<PhotoQueueItem*>(ptr);
        if (item->job == job) {
            if (job->error() != FileSystemJob::NoError) {
                removeItem(item);
            } else {
                foreach (const FileInfo &child, job->results()) {
                    // FIXME: recurse
                    if (!child.isDir()) {
                        item->items << child;
                        item->totalFiles++;
                        item->totalBytes += child.size();
                    }
                }
                emit dataChanged(createIndex(item), createIndex(item));
                processQueue();
            }
            break;
        }
    }
}

/** When an upload finishes, process the results.
 */
void PhotoQueueModel::_q_uploadFinished()
{
    if (m_uploadItem) {
        removeItem(m_uploadItem);
        m_uploadItem = 0;
    }

    if (m_uploadDevice) {
        delete m_uploadDevice;
        m_uploadDevice = 0;
    }

    processQueue();
}

/** When upload progress changes, emit notifications.
 */
void PhotoQueueModel::_q_uploadProgress(qint64 done, qint64 total)
{
    if (m_uploadItem) {
        m_uploadItem->doneBytes = done;
        m_uploadItem->totalBytes = total;
        emit dataChanged(createIndex(m_uploadItem), createIndex(m_uploadItem));
    }
}

void PhotoQueueModel::_q_uploadResized(QIODevice *device)
{
    bool check;
    Q_UNUSED(check);

    if (m_uploadItem) {
        m_uploadDevice = device;
        m_uploadItem->job = m_uploadItem->fileSystem->put(m_uploadItem->info.url(), m_uploadDevice);
        check = connect(m_uploadItem->job, SIGNAL(finished()),
                        this, SLOT(_q_uploadFinished()));
        Q_ASSERT(check);

        check = connect(m_uploadItem->job, SIGNAL(uploadProgress(qint64,qint64)),
                        this, SLOT(_q_uploadProgress(qint64,qint64)));
        Q_ASSERT(check);
    }
}

PhotoResizer::PhotoResizer(QObject *parent)
    : QObject(parent)
{
}

void PhotoResizer::resize(const QString &path)
{
    QIODevice *device = 0;

    // process the next file to upload
    const QByteArray imageFormat = QImageReader::imageFormat(path);
    QImage image;
    if (!imageFormat.isEmpty() && image.load(path, imageFormat.constData()))
    {
        if (image.width() > UPLOAD_SIZE.width() || image.height() > UPLOAD_SIZE.height())
            image = image.scaled(UPLOAD_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        device = new QBuffer;
        device->open(QIODevice::WriteOnly);
        image.save(device, imageFormat.constData());
        device->open(QIODevice::ReadOnly);
    } else {
        device = new QFile(path);
        device->open(QIODevice::ReadOnly);
    }
    emit finished(device);
}
