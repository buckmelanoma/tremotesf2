/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include "torrentsproxymodel.h"

#include "settings.h"
#include "torrentsmodel.h"

#include "libtremotesf/torrent.h"
#include "libtremotesf/tracker.h"

namespace tremotesf
{
    TorrentsProxyModel::TorrentsProxyModel(TorrentsModel* sourceModel, int sortRole, QObject* parent)
        : BaseProxyModel(sourceModel, sortRole, parent),
          mStatusFilterEnabled(Settings::instance()->isTorrentsStatusFilterEnabled()),
          mStatusFilter(Settings::instance()->torrentsStatusFilter()),
          mTrackerFilterEnabled(Settings::instance()->isTorrentsTrackerFilterEnabled()),
          mTrackerFilter(Settings::instance()->torrentsTrackerFilter()),
          mDownloadDirectoryFilterEnabled(Settings::instance()->isTorrentsDownloadDirectoryFilterEnabled()),
          mDownloadDirectoryFilter(Settings::instance()->torrentsDownloadDirectoryFilter())
    {
    }

    TorrentsProxyModel::~TorrentsProxyModel()
    {
        auto settings = Settings::instance();
        settings->setTorrentsStatusFilterEnabled(mStatusFilterEnabled);
        settings->setTorrentsStatusFilter(mStatusFilter);
        settings->setTorrentsTrackerFilterEnabled(mTrackerFilterEnabled);
        settings->setTorrentsTrackerFilter(mTrackerFilter);
        settings->setTorrentsDownloadDirectoryFilterEnabled(mDownloadDirectoryFilterEnabled);
        settings->setTorrentsDownloadDirectoryFilter(mDownloadDirectoryFilter);
    }

    QString TorrentsProxyModel::searchString() const
    {
        return mSearchString;
    }

    void TorrentsProxyModel::setSearchString(const QString& string)
    {
        if (string != mSearchString) {
            mSearchString = string;
            invalidateFilter();
            emit searchStringChanged();
        }
    }

    bool TorrentsProxyModel::isStatusFilterEnabled() const
    {
        return mStatusFilterEnabled;
    }

    void TorrentsProxyModel::setStatusFilterEnabled(bool enabled)
    {
        if (enabled != mStatusFilterEnabled) {
            mStatusFilterEnabled = enabled;
            invalidateFilter();
            emit statusFilterEnabledChanged();
        }
    }

    TorrentsProxyModel::StatusFilter TorrentsProxyModel::statusFilter() const
    {
        return mStatusFilter;
    }

    void TorrentsProxyModel::setStatusFilter(TorrentsProxyModel::StatusFilter filter)
    {
        if (filter != mStatusFilter) {
            mStatusFilter = filter;
            invalidateFilter();
            emit statusFilterChanged();
        }
    }

    bool TorrentsProxyModel::isTrackerFilterEnabled() const
    {
        return mTrackerFilterEnabled;
    }

    void TorrentsProxyModel::setTrackerFilterEnabled(bool enabled)
    {
        if (enabled != mTrackerFilterEnabled) {
            mTrackerFilterEnabled = enabled;
            invalidateFilter();
            emit trackerFilterEnabledChanged();
        }
    }

    QString TorrentsProxyModel::trackerFilter() const
    {
        return mTrackerFilter;
    }

    void TorrentsProxyModel::setTrackerFilter(const QString& filter)
    {
        if (filter != mTrackerFilter) {
            mTrackerFilter = filter;
            invalidateFilter();
            emit trackerFilterChanged();
        }
    }

    bool TorrentsProxyModel::isDownloadDirectoryFilterEnabled() const
    {
        return mDownloadDirectoryFilterEnabled;
    }

    void TorrentsProxyModel::setDownloadDirectoryFilterEnabled(bool enabled)
    {
        if (enabled != mDownloadDirectoryFilterEnabled) {
            mDownloadDirectoryFilterEnabled = enabled;
            invalidateFilter();
            emit downloadDirectoryFilterEnabledChanged();
        }
    }

    QString TorrentsProxyModel::downloadDirectoryFilter() const
    {
        return mDownloadDirectoryFilter;
    }

    void TorrentsProxyModel::setDownloadDirectoryFilter(const QString& filter)
    {
        if (filter != mDownloadDirectoryFilter) {
            mDownloadDirectoryFilter = filter;
            invalidateFilter();
            emit downloadDirectoryFilterChanged();
        }
    }

    bool TorrentsProxyModel::statusFilterAcceptsTorrent(const libtremotesf::Torrent* torrent, StatusFilter filter)
    {
        using libtremotesf::TorrentData;
        switch (filter) {
        case Active:
            return (torrent->status() == TorrentData::Downloading || torrent->status() == TorrentData::Seeding);
        case Downloading:
            return (torrent->status() == TorrentData::Downloading ||
                    torrent->status() == TorrentData::StalledDownloading ||
                    torrent->status() == TorrentData::QueuedForDownloading);
        case Seeding:
            return (torrent->status() == TorrentData::Seeding ||
                    torrent->status() == TorrentData::StalledSeeding ||
                    torrent->status() == TorrentData::QueuedForSeeding);
        case Paused:
            return torrent->status() == TorrentData::Paused;
        case Checking:
            return (torrent->status() == TorrentData::Checking ||
                    torrent->status() == TorrentData::QueuedForChecking);
        case Errored:
            return torrent->status() == TorrentData::Errored;
        default:
            return true;
        }
    }

    bool TorrentsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex&) const
    {
        bool accepts = true;

        const libtremotesf::Torrent* torrent = static_cast<TorrentsModel*>(sourceModel())->torrentAtRow(sourceRow);

        if (!mSearchString.isEmpty() &&
            !torrent->name().contains(mSearchString, Qt::CaseInsensitive)) {

            accepts = false;
        }

        if (mStatusFilterEnabled && !statusFilterAcceptsTorrent(torrent, mStatusFilter)) {
            accepts = false;
        }

        if (mTrackerFilterEnabled && !mTrackerFilter.isEmpty()) {
            bool found = false;
            for (const libtremotesf::Tracker& tracker : torrent->trackers()) {
                if (tracker.site() == mTrackerFilter) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                accepts = false;
            }
        }

        if (mDownloadDirectoryFilterEnabled && !mDownloadDirectoryFilter.isEmpty()) {
            if (torrent->downloadDirectory() != mDownloadDirectoryFilter) {
                accepts = false;
            }
        }

        return accepts;
    }
}
