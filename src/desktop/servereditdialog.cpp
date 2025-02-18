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

#include "servereditdialog.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include "commondelegate.h"
#include "../servers.h"
#include "../serversmodel.h"
#include "../libtremotesf/stdutils.h"
#include "../utils.h"

namespace tremotesf
{
    namespace
    {
        const QLatin1String removeIconName("list-remove");

        const Server::ProxyType proxyTypeComboBoxValues[] {
            Server::ProxyType::Default,
            Server::ProxyType::Http,
            Server::ProxyType::Socks5
        };
    }

    class MountedDirectoriesWidget : public QTableWidget
    {
        Q_OBJECT

    public:
        MountedDirectoriesWidget(int rows, int columns, QWidget* parent = nullptr)
            : QTableWidget(rows, columns, parent)
        {
            setMinimumHeight(192);
            setSelectionMode(QAbstractItemView::SingleSelection);
            setItemDelegate(new CommonDelegate(this));
            setHorizontalHeaderLabels({qApp->translate("tremotesf", "Local directory"),
                                       qApp->translate("tremotesf", "Remote directory")});
            horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            horizontalHeader()->setSectionsClickable(false);
            verticalHeader()->setVisible(false);

            setContextMenuPolicy(Qt::CustomContextMenu);

            auto removeAction = new QAction(QIcon::fromTheme(removeIconName), qApp->translate("tremotesf", "&Remove"), this);
            removeAction->setShortcut(QKeySequence::Delete);
            addAction(removeAction);

            QObject::connect(removeAction, &QAction::triggered, this, [=] {
                const auto items(selectionModel()->selectedIndexes());
                if (!items.isEmpty()) {
                    removeRow(items.first().row());
                }
            });

            QObject::connect(this, &QWidget::customContextMenuRequested, this, [=](auto pos) {
                const QModelIndex index(indexAt(pos));
                if (!index.isValid()) {
                    return;
                }

                QMenu contextMenu;

                QAction* selectAction = nullptr;
                if (index.column() == 0) {
                    selectAction = contextMenu.addAction(qApp->translate("tremotesf", "&Select..."));
                }

                contextMenu.addAction(removeAction);

                auto executed = contextMenu.exec(viewport()->mapToGlobal(pos));
                if (executed && executed == selectAction) {
                    const QString directory(QFileDialog::getExistingDirectory(this));
                    if (!directory.isEmpty()) {
                        const auto item = this->item(index.row(), index.column());
                        if (item) {
                            item->setText(directory);
                            item->setToolTip(directory);
                        }
                    }
                }
            });
        }

        void addRow(const QString& localDirectory, const QString& remoteDirectory)
        {
            const int row = rowCount();
            insertRow(row);
            const auto localItem = new QTableWidgetItem(localDirectory);
            localItem->setToolTip(localDirectory);
            setItem(row, 0, localItem);
            const auto remoteItem = new QTableWidgetItem(remoteDirectory);
            remoteItem->setToolTip(remoteDirectory);
            setItem(row, 1, remoteItem);
        }

    protected:
        void keyPressEvent(QKeyEvent* event) override
        {
            QTableWidget::keyPressEvent(event);
            switch (event->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter:
                event->accept();
                if (state() != EditingState) {
                    edit(currentIndex());
                }
                break;
            }
        }
    };

    ServerEditDialog::ServerEditDialog(ServersModel* serversModel, int row, QWidget* parent)
        : QDialog(parent),
          mServersModel(serversModel)
    {
        setupUi();

        if (row == -1) {
            setWindowTitle(qApp->translate("tremotesf", "Add Server"));

            mPortSpinBox->setValue(9091);
            mApiPathLineEdit->setText(QLatin1String("/transmission/rpc"));
            mProxyTypeComboBox->setCurrentIndex(index_of_i(proxyTypeComboBoxValues, Server::ProxyType::Default));
            mHttpsGroupBox->setChecked(false);
            mAuthenticationGroupBox->setChecked(false);
            mUpdateIntervalSpinBox->setValue(5);
            mTimeoutSpinBox->setValue(30);
            mAutoReconnectGroupBox->setChecked(false);
            mAutoReconnectSpinBox->setValue(30);
        } else {
            const Server& server = mServersModel->servers()[static_cast<size_t>(row)];

            mServerName = server.name;
            setWindowTitle(mServerName);

            mNameLineEdit->setText(mServerName);
            mAddressLineEdit->setText(server.address);
            mPortSpinBox->setValue(server.port);
            mApiPathLineEdit->setText(server.apiPath);

            mProxyTypeComboBox->setCurrentIndex(index_of_i(proxyTypeComboBoxValues, server.proxyType));
            mProxyHostnameLineEdit->setText(server.proxyHostname);
            mProxyPortSpinBox->setValue(server.proxyPort);
            mProxyUserLineEdit->setText(server.proxyUser);
            mProxyPasswordLineEdit->setText(server.proxyPassword);

            mHttpsGroupBox->setChecked(server.https);
            mSelfSignedCertificateCheckBox->setChecked(server.selfSignedCertificateEnabled);
            mSelfSignedCertificateEdit->setPlainText(server.selfSignedCertificate);
            mClientCertificateCheckBox->setChecked(server.clientCertificateEnabled);
            mClientCertificateEdit->setPlainText(server.clientCertificate);

            mAuthenticationGroupBox->setChecked(server.authentication);
            mUsernameLineEdit->setText(server.username);
            mPasswordLineEdit->setText(server.password);

            mUpdateIntervalSpinBox->setValue(server.updateInterval);
            mTimeoutSpinBox->setValue(server.timeout);

            mAutoReconnectGroupBox->setChecked(server.autoReconnectEnabled);
            mAutoReconnectSpinBox->setValue(server.autoReconnectInterval);

            for (auto i = server.mountedDirectories.cbegin(), end = server.mountedDirectories.cend();
                 i != end;
                 ++i) {
                mMountedDirectoriesWidget->addRow(i.key(), i.value().toString());
            }
        }

        setProxyFieldsVisible();
    }

    QSize ServerEditDialog::sizeHint() const
    {
        return QDialog::sizeHint().expandedTo(QSize(384, 512));
    }

    void ServerEditDialog::accept()
    {
        if (mServersModel) {
            const QString newName(mNameLineEdit->text());
            if (newName != mServerName && mServersModel->hasServer(newName)) {
                QMessageBox messageBox(QMessageBox::Warning,
                                       qApp->translate("tremotesf", "Overwrite Server"),
                                       qApp->translate("tremotesf", "Server already exists"),
                                       QMessageBox::Ok | QMessageBox::Cancel,
                                       this);
                messageBox.setDefaultButton(QMessageBox::Cancel);
                messageBox.button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Overwrite"));
                if (messageBox.exec() != QMessageBox::Ok) {
                    return;
                }
            }
        }
        setServer();
        QDialog::accept();
    }

    void ServerEditDialog::setupUi()
    {
        auto topLayout = new QVBoxLayout(this);
        auto scrollArea = new QScrollArea(this);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        scrollArea->setWidgetResizable(true);
        topLayout->addWidget(scrollArea);

        auto widget = new QWidget(this);
        auto formLayout = new QFormLayout(widget);
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mNameLineEdit = new QLineEdit(this);
        mNameLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("^\\S.*")), this));
        QObject::connect(mNameLineEdit, &QLineEdit::textChanged, this, &ServerEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Name:"), mNameLineEdit);

        mAddressLineEdit = new QLineEdit(this);
        auto addressValidator = new QRegularExpressionValidator(QRegularExpression(QLatin1String("^\\S+")), this);
        mAddressLineEdit->setValidator(addressValidator);
        QObject::connect(mAddressLineEdit, &QLineEdit::textChanged, this, &ServerEditDialog::canAcceptUpdate);
        formLayout->addRow(qApp->translate("tremotesf", "Address:"), mAddressLineEdit);

        mPortSpinBox = new QSpinBox(this);
        const int maxPort = 65535;
        mPortSpinBox->setMaximum(maxPort);
        formLayout->addRow(qApp->translate("tremotesf", "Port:"), mPortSpinBox);

        mApiPathLineEdit = new QLineEdit(this);
        formLayout->addRow(qApp->translate("tremotesf", "API path:"), mApiPathLineEdit);

        auto proxyGroupBox = new QGroupBox(qApp->translate("tremotesf", "Proxy"), this);
        mProxyLayout = new QFormLayout(proxyGroupBox);

        mProxyTypeComboBox = new QComboBox(this);
        mProxyTypeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for (Server::ProxyType type : proxyTypeComboBoxValues) {
            switch (type) {
            case Server::ProxyType::Default:
                mProxyTypeComboBox->addItem(qApp->translate("tremotesf", "Default"));
                break;
            case Server::ProxyType::Http:
                mProxyTypeComboBox->addItem(qApp->translate("tremotesf", "HTTP"));
                break;
            case Server::ProxyType::Socks5:
                mProxyTypeComboBox->addItem(qApp->translate("tremotesf", "SOCKS5"));
                break;
            }
        }
        QObject::connect(mProxyTypeComboBox, &QComboBox::currentTextChanged, this, &ServerEditDialog::setProxyFieldsVisible);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Proxy type:"), mProxyTypeComboBox);

        mProxyHostnameLineEdit = new QLineEdit(this);
        mProxyHostnameLineEdit->setValidator(addressValidator);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Address:"), mProxyHostnameLineEdit);

        mProxyPortSpinBox = new QSpinBox(this);
        mProxyPortSpinBox->setMaximum(maxPort);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Port:"), mProxyPortSpinBox);

        mProxyUserLineEdit = new QLineEdit(this);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Username:"), mProxyUserLineEdit);
        mProxyPasswordLineEdit = new QLineEdit(this);
        mProxyPasswordLineEdit->setEchoMode(QLineEdit::Password);
        mProxyLayout->addRow(qApp->translate("tremotesf", "Password:"), mProxyPasswordLineEdit);

        formLayout->addRow(proxyGroupBox);

        mHttpsGroupBox = new QGroupBox(qApp->translate("tremotesf", "HTTPS"), this);
        mHttpsGroupBox->setCheckable(true);

        auto httpsGroupBoxLayout = new QVBoxLayout(mHttpsGroupBox);

        mSelfSignedCertificateCheckBox = new QCheckBox(qApp->translate("tremotesf", "Server uses self-signed certificate"), this);
        httpsGroupBoxLayout->addWidget(mSelfSignedCertificateCheckBox);
        mSelfSignedCertificateEdit = new QPlainTextEdit(this);
        mSelfSignedCertificateEdit->setMinimumHeight(192);
        mSelfSignedCertificateEdit->setPlaceholderText(qApp->translate("tremotesf", "Server's certificate in PEM format"));
        mSelfSignedCertificateEdit->setVisible(false);
        httpsGroupBoxLayout->addWidget(mSelfSignedCertificateEdit);
        auto* selfSignedCertificateLoadFromFile = new QPushButton(qApp->translate("tremotesf", "Load from file..."), this);
        selfSignedCertificateLoadFromFile->setVisible(false);
        QObject::connect(selfSignedCertificateLoadFromFile, &QPushButton::clicked, this, [this] {
            loadCertificateFromFile(mSelfSignedCertificateEdit);
        });
        httpsGroupBoxLayout->addWidget(selfSignedCertificateLoadFromFile);
        QObject::connect(mSelfSignedCertificateCheckBox, &QCheckBox::toggled, this, [=](bool checked) {
            mSelfSignedCertificateEdit->setVisible(checked);
            selfSignedCertificateLoadFromFile->setVisible(checked);
        });


        mClientCertificateCheckBox = new QCheckBox(qApp->translate("tremotesf", "Use client certificate authentication"), this);
        httpsGroupBoxLayout->addWidget(mClientCertificateCheckBox);
        mClientCertificateEdit = new QPlainTextEdit(this);
        mClientCertificateEdit->setMinimumHeight(192);
        mClientCertificateEdit->setPlaceholderText(qApp->translate("tremotesf", "Certificate in PEM format with private key"));
        mClientCertificateEdit->setVisible(false);
        httpsGroupBoxLayout->addWidget(mClientCertificateEdit);
        auto* clientCertificateLoadFromFile = new QPushButton(qApp->translate("tremotesf", "Load from file..."), this);
        clientCertificateLoadFromFile->setVisible(false);
        QObject::connect(clientCertificateLoadFromFile, &QPushButton::clicked, this, [this] {
            loadCertificateFromFile(mClientCertificateEdit);
        });
        httpsGroupBoxLayout->addWidget(clientCertificateLoadFromFile);
        QObject::connect(mClientCertificateCheckBox, &QCheckBox::toggled, this, [=](bool checked) {
            mClientCertificateEdit->setVisible(checked);
            clientCertificateLoadFromFile->setVisible(checked);
        });

        formLayout->addRow(mHttpsGroupBox);

        mAuthenticationGroupBox = new QGroupBox(qApp->translate("tremotesf", "Authentication"), this);
        mAuthenticationGroupBox->setCheckable(true);
        auto authenticationGroupBoxLayout = new QFormLayout(mAuthenticationGroupBox);
        mUsernameLineEdit = new QLineEdit(this);
        authenticationGroupBoxLayout->addRow(qApp->translate("tremotesf", "Username:"), mUsernameLineEdit);
        mPasswordLineEdit = new QLineEdit(this);
        mPasswordLineEdit->setEchoMode(QLineEdit::Password);
        authenticationGroupBoxLayout->addRow(qApp->translate("tremotesf", "Password:"), mPasswordLineEdit);
        formLayout->addRow(mAuthenticationGroupBox);

        mUpdateIntervalSpinBox = new QSpinBox(this);
        mUpdateIntervalSpinBox->setMinimum(1);
        mUpdateIntervalSpinBox->setMaximum(3600);
        //: Seconds
        mUpdateIntervalSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Update interval:"), mUpdateIntervalSpinBox);

        mTimeoutSpinBox = new QSpinBox(this);
        mTimeoutSpinBox->setMinimum(5);
        mTimeoutSpinBox->setMaximum(60);
        //: Seconds
        mTimeoutSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        formLayout->addRow(qApp->translate("tremotesf", "Timeout:"), mTimeoutSpinBox);

        mAutoReconnectGroupBox = new QGroupBox(qApp->translate("tremotesf", "Auto reconnect on error"), this);
        mAutoReconnectGroupBox->setCheckable(true);
        formLayout->addRow(mAutoReconnectGroupBox);
        auto* autoReconnectFormLayout = new QFormLayout(mAutoReconnectGroupBox);
        autoReconnectFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        mAutoReconnectSpinBox = new QSpinBox(this);
        mAutoReconnectSpinBox->setMinimum(1);
        mAutoReconnectSpinBox->setMaximum(3600);
        //: Seconds
        mAutoReconnectSpinBox->setSuffix(qApp->translate("tremotesf", " s"));
        autoReconnectFormLayout->addRow(qApp->translate("tremotesf", "Auto reconnect interval:"), mAutoReconnectSpinBox);

        auto mountedDirectoriesGroupBox = new QGroupBox(qApp->translate("tremotesf", "Mounted directories"), this);
        auto mountedDirectoriesLayout = new QGridLayout(mountedDirectoriesGroupBox);
        mMountedDirectoriesWidget = new MountedDirectoriesWidget(0, 2);
        mountedDirectoriesLayout->addWidget(mMountedDirectoriesWidget, 0, 0, 1, 2);
        auto addDirectoriesButton = new QPushButton(QIcon::fromTheme(QLatin1String("list-add")), qApp->translate("tremotesf", "Add"), this);
        QObject::connect(addDirectoriesButton, &QPushButton::clicked, this, [=] {
            const QString directory(QFileDialog::getExistingDirectory(this));
            if (!directory.isEmpty()) {
                mMountedDirectoriesWidget->addRow(directory, QString());
            }
        });
        mountedDirectoriesLayout->addWidget(addDirectoriesButton, 1, 0);
        auto removeDirectoriesButton = new QPushButton(QIcon::fromTheme(removeIconName), qApp->translate("tremotesf", "Remove"), this);
        QObject::connect(removeDirectoriesButton, &QPushButton::clicked, this, [=] {
            const auto items(mMountedDirectoriesWidget->selectionModel()->selectedIndexes());
            if (!items.isEmpty()) {
                mMountedDirectoriesWidget->removeRow(items.first().row());
            }
        });
        mountedDirectoriesLayout->addWidget(removeDirectoriesButton, 1, 1);
        formLayout->addRow(mountedDirectoriesGroupBox);

        scrollArea->setWidget(widget);

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &ServerEditDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &ServerEditDialog::reject);
        topLayout->addWidget(mDialogButtonBox);

        setMinimumSize(minimumSizeHint());
    }

    void ServerEditDialog::setProxyFieldsVisible()
    {
        const bool visible = (proxyTypeComboBoxValues[mProxyTypeComboBox->currentIndex()] != Server::ProxyType::Default);
        for (int i = 1, max = mProxyLayout->rowCount(); i < max; ++i) {
            mProxyLayout->itemAt(i, QFormLayout::LabelRole)->widget()->setVisible(visible);
            mProxyLayout->itemAt(i, QFormLayout::FieldRole)->widget()->setVisible(visible);
        }
    }

    void ServerEditDialog::canAcceptUpdate()
    {
        mDialogButtonBox->button(QDialogButtonBox::Ok)
            ->setEnabled(mNameLineEdit->hasAcceptableInput() &&
                         mAddressLineEdit->hasAcceptableInput());
    }

    void ServerEditDialog::setServer()
    {
        QVariantMap mountedDirectories;
        for (int i = 0, max = mMountedDirectoriesWidget->rowCount(); i < max; ++i) {
            const auto localItem = mMountedDirectoriesWidget->item(i, 0);
            const QString localDirectory(localItem ? localItem->text().trimmed() : QString());
            const auto remoteItem = mMountedDirectoriesWidget->item(i, 1);
            const QString remoteDirectory(remoteItem ? remoteItem->text().trimmed() : QString());
            if (!localDirectory.isEmpty() && !remoteDirectory.isEmpty()) {
                mountedDirectories.insert(localDirectory, remoteDirectory);
            }
        }

        if (mServersModel) {
            mServersModel->setServer(mServerName,
                                     mNameLineEdit->text(),
                                     mAddressLineEdit->text(),
                                     mPortSpinBox->value(),
                                     mApiPathLineEdit->text(),

                                     proxyTypeComboBoxValues[mProxyTypeComboBox->currentIndex()],
                                     mProxyHostnameLineEdit->text(),
                                     mProxyPortSpinBox->value(),
                                     mProxyUserLineEdit->text(),
                                     mProxyPasswordLineEdit->text(),

                                     mHttpsGroupBox->isChecked(),
                                     mSelfSignedCertificateCheckBox->isChecked(),
                                     mSelfSignedCertificateEdit->toPlainText().toLatin1(),
                                     mClientCertificateCheckBox->isChecked(),
                                     mClientCertificateEdit->toPlainText().toLatin1(),

                                     mAuthenticationGroupBox->isChecked(),
                                     mUsernameLineEdit->text(),
                                     mPasswordLineEdit->text(),

                                     mUpdateIntervalSpinBox->value(),
                                     mTimeoutSpinBox->value(),

                                     mAutoReconnectGroupBox->isChecked(),
                                     mAutoReconnectSpinBox->value(),

                                     mountedDirectories);
        } else {
            Servers::instance()->setServer(mServerName,
                                           mNameLineEdit->text(),
                                           mAddressLineEdit->text(),
                                           mPortSpinBox->value(),
                                           mApiPathLineEdit->text(),

                                           proxyTypeComboBoxValues[mProxyTypeComboBox->currentIndex()],
                                           mProxyHostnameLineEdit->text(),
                                           mProxyPortSpinBox->value(),
                                           mProxyUserLineEdit->text(),
                                           mProxyPasswordLineEdit->text(),

                                           mHttpsGroupBox->isChecked(),
                                           mSelfSignedCertificateCheckBox->isChecked(),
                                           mSelfSignedCertificateEdit->toPlainText().toLatin1(),
                                           mClientCertificateCheckBox->isChecked(),
                                           mClientCertificateEdit->toPlainText().toLatin1(),

                                           mAuthenticationGroupBox->isChecked(),
                                           mUsernameLineEdit->text(),
                                           mPasswordLineEdit->text(),

                                           mUpdateIntervalSpinBox->value(),
                                           mTimeoutSpinBox->value(),

                                           mAutoReconnectGroupBox->isChecked(),
                                           mAutoReconnectSpinBox->value(),

                                           mountedDirectories);
        }
    }

    void ServerEditDialog::loadCertificateFromFile(QPlainTextEdit* target)
    {
        auto *fileDialog = new QFileDialog(this,
                                           qApp->translate("tremotesf", "Select Files"),
                                           {},
                                           /*qApp->translate("tremotesf", "Torrent Files (*.torrent)")*/ {});
        fileDialog->setAttribute(Qt::WA_DeleteOnClose);
        fileDialog->setFileMode(QFileDialog::ExistingFile);
        fileDialog->setMimeTypeFilters({QLatin1String("application/x-pem-file")});

        QObject::connect(fileDialog, &QFileDialog::accepted, this, [=] {
            target->setPlainText(Utils::readTextFile(fileDialog->selectedFiles().first()));
        });

#ifdef Q_OS_WIN
        fileDialog->open();
#else
        fileDialog->show();
#endif
    }
}

#include "servereditdialog.moc"
