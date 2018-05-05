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

#ifndef TREMOTESF_SERVERSDIALOG_H
#define TREMOTESF_SERVERSDIALOG_H

#include <QDialog>

class QListView;
class KMessageWidget;

namespace tremotesf
{
    class ServersModel;
    class BaseProxyModel;

    class ServersDialog : public QDialog
    {
    public:
        explicit ServersDialog(QWidget* parent = nullptr);
        QSize sizeHint() const override;
        void accept() override;

    private:
        void showEditDialogs();
        void removeServers();

    private:
        KMessageWidget* mNoServersWidget;
        ServersModel* mModel;
        BaseProxyModel* mProxyModel;
        QListView* mServersView;
    };
}

#endif // TREMOTESF_SERVERSDIALOG_H
