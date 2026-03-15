#include "main_window.h"
#include "settings_widget.h"
#include "settings_models.h"
#include "app.h"
#include "data_processor.h"
#include "settings_models.h"
#include "util.h"
#include "qpmu/util.h"
#include "qpmu/defs.h"

#include <QFileSelector>
#include <QFileDialog>
#include <QComboBox>
#include <QButtonGroup>
#include <QGroupBox>
#include <QPointF>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMargins>
#include <QStatusBar>
#include <QPushButton>
#include <QCheckBox>
#include <QRegularExpressionValidator>
#include <QWidget>
#include <QHeaderView>

#include <algorithm>
#include <qglobal.h>

using namespace qpmu;
#define QSL QStringLiteral

SettingsWidget::SettingsWidget(QWidget *parent) : QTabWidget(parent)
{
    hide();

    { /// Sampler settings page
        auto settings = SamplerSettings();
        settings.load();
        auto page = samplerSettingsPage(settings);
        addTab(page, "Signal Sampler");
    }

    { /// Calibration settings page
        auto page = new QTabWidget();
        auto settings = CalibrationSettings();
        m_oldCalibrationSettings = settings;
        for (USize i = 0; i < CountSignals; ++i) {
            page->addTab(calibrationWidget(i, settings.data[i]), NameOfSignal[i]);
        }
        addTab(page, "Calibration");
    }
}

QWidget *SettingsWidget::samplerSettingsPage(const SamplerSettings &settings)
{

    auto page = new QWidget();
    m_oldSamplerSettings = settings;

    auto outerLayout = new QVBoxLayout(page);

    auto dataFormatGroup = new QButtonGroup();
    auto isDataBinaryCheckBox = new QCheckBox();
    auto isDataFormattedCheckBox = new QCheckBox();

    auto connectionTypeGroup = new QButtonGroup();
    auto noConnectionRadio = new QRadioButton();
    auto socketConnectionRadio = new QRadioButton();
    auto processConnectionRadio = new QRadioButton();

    auto socketTypeGroup = new QButtonGroup();
    auto udpRadio = new QRadioButton();
    auto tcpRadio = new QRadioButton();
    auto hostEdit = new QLineEdit();
    auto portEdit = new QLineEdit();

    auto browseFileButton = new QPushButton();
    auto progEdit = new QLineEdit();
    auto argsEdit = new QLineEdit();

    auto dialogButtonBox = new QDialogButtonBox();
    auto updateConnectionButton = dialogButtonBox->addButton(QDialogButtonBox::Apply);
    auto resetButton = dialogButtonBox->addButton(QDialogButtonBox::Reset);
    auto restoreDefaultsButton = dialogButtonBox->addButton(QDialogButtonBox::RestoreDefaults);

    /// Common operations

    /// * Extract settings data from UI
    auto extractSettings = [=]() -> SamplerSettings {
        SamplerSettings newSettings;
        newSettings.isDataBinary = dataFormatGroup->checkedId();
        newSettings.connection = (SamplerSettings::ConnectionType)connectionTypeGroup->checkedId();

        newSettings.socketConfig.host = hostEdit->text();
        newSettings.socketConfig.port = portEdit->text().toInt();
        newSettings.socketConfig.socketType =
                (SamplerSettings::SocketType)socketTypeGroup->checkedId();

        newSettings.processConfig.prog = progEdit->text();
        newSettings.processConfig.args = parsePrcoessString(argsEdit->text());

        return newSettings;
    };

    /// * Load settings data into UI
    auto loadSettings = [=](const SamplerSettings &settings) {
        m_oldSamplerSettings = extractSettings();

        dataFormatGroup->button(settings.isDataBinary)->setChecked(true);
        connectionTypeGroup->button(settings.connection)->setChecked(true);

        hostEdit->setText(settings.socketConfig.host);
        portEdit->setText(QString::number(settings.socketConfig.port));
        socketTypeGroup->button(settings.socketConfig.socketType)->setChecked(true);

        progEdit->setText(settings.processConfig.prog);
        argsEdit->setText(settings.processConfig.args.join(" "));
    };

    /// * Check if settings are changed
    auto isSettingsChanged = [=] { return extractSettings() != m_oldSamplerSettings; };

    /// * Update enabled state of buttons
    auto updateEnabledState = [=] {
        auto changed = isSettingsChanged();
        resetButton->setEnabled(changed);
    };

    /// * Save settings to the file
    auto saveSettings = [=]() -> bool {
        auto newSettings = extractSettings();
        if (newSettings.save()) {
            m_oldSamplerSettings = newSettings;
            updateEnabledState();
            qInfo() << "Settings saved";
            return true;
        }
        qInfo() << "Failed to save settings";
        APP->mainWindow()->statusBar()->showMessage("Failed to save settings: invalid settings",
                                                    5000);
        return false;
    };

    { /// Process/socket configuration tab-widget
        auto tabWidget = new QTabWidget(page);
        outerLayout->addWidget(tabWidget);

        auto socketPage = new QWidget();
        auto processPage = new QWidget();
        tabWidget->addTab(socketPage, "Socket configuratin");
        tabWidget->addTab(processPage, "Process configuratin");

        { /// Socket configuration
            auto socketConfigForm = new QFormLayout(socketPage);
            auto socketTypeLayout = new QHBoxLayout();

            socketConfigForm->addRow("Host IP", hostEdit);
            socketConfigForm->addRow("Port", portEdit);
            socketConfigForm->addRow("Socket type", socketTypeLayout);

            { /// Host edit
                auto ipPattern = QString("^(%1\\.%1\\.%1\\.%1)|localhost$")
                                         .arg("(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]{1,2})");
                auto ipValidator = new QRegularExpressionValidator(QRegularExpression(ipPattern));
                hostEdit->setValidator(ipValidator);
                hostEdit->setPlaceholderText("xxx.xxx.xxx.xxx");
                hostEdit->setFixedWidth(150);
                hostEdit->setText(settings.socketConfig.host);
            }

            { /// Port edit
                auto portValidator = new QIntValidator(0, 65535);
                portEdit->setValidator(portValidator);
                portEdit->setPlaceholderText("0-65535");
                portEdit->setFixedWidth(150);
                portEdit->setText(QString::number(settings.socketConfig.port));
            }

            { /// Socket type
                udpRadio->setText("UDP");
                tcpRadio->setText("TCP");

                socketTypeLayout->addWidget(udpRadio);
                socketTypeLayout->addWidget(tcpRadio);
                socketTypeLayout->setAlignment(Qt::AlignLeft);

                socketTypeGroup->addButton(udpRadio, SamplerSettings::UdpSocket);
                socketTypeGroup->addButton(tcpRadio, SamplerSettings::TcpSocket);
                socketTypeGroup->button(settings.socketConfig.socketType)->setChecked(true);
            }
        }

        { /// Process configuration
            auto processConfigForm = new QFormLayout(processPage);
            auto programLayout = new QHBoxLayout();

            processConfigForm->addRow("Program", programLayout);
            processConfigForm->addRow("Arguments", argsEdit);

            { /// Program edit
                programLayout->addWidget(progEdit, 1);
                programLayout->addWidget(browseFileButton);

                progEdit->setText(settings.processConfig.prog);
                browseFileButton->setText("Browse");
                browseFileButton->setFixedWidth(80);

                connect(browseFileButton, &QPushButton::clicked, [=] {
                    auto file = QFileDialog::getOpenFileName(this, "Select program");
                    if (!file.isEmpty()) {
                        progEdit->setText(file);
                    }
                });
            }

            { /// Arguments edit
                argsEdit->setText(settings.processConfig.args.join(" "));
            }
        }
    }

    { /// Connection type and binary-data flag
        auto form = new QFormLayout();
        outerLayout->addLayout(form);

        auto connectionTypeLayout = new QHBoxLayout();
        auto dataFormatLayout = new QVBoxLayout();

        form->addRow("Connection type", connectionTypeLayout);
        form->addRow("Data format", dataFormatLayout);

        { /// Connection type config

            noConnectionRadio->setText("None (disable connection)");
            socketConnectionRadio->setText("Socket connection");
            processConnectionRadio->setText("Process connection");

            connectionTypeLayout->addWidget(noConnectionRadio);
            connectionTypeLayout->addWidget(socketConnectionRadio);
            connectionTypeLayout->addWidget(processConnectionRadio);
            connectionTypeLayout->setAlignment(Qt::AlignLeft);

            connectionTypeGroup->addButton(noConnectionRadio, SamplerSettings::None);
            connectionTypeGroup->addButton(socketConnectionRadio, SamplerSettings::Socket);
            connectionTypeGroup->addButton(processConnectionRadio, SamplerSettings::Process);
            connectionTypeGroup->button(settings.connection)->setChecked(true);
        }

        { /// Data format config
            dataFormatLayout->addWidget(isDataBinaryCheckBox);
            dataFormatLayout->addWidget(isDataFormattedCheckBox);
            dataFormatLayout->setAlignment(Qt::AlignLeft);

            isDataBinaryCheckBox->setText("Binary-encoded (raw)");
            isDataFormattedCheckBox->setText("ASCII string formatted");

            dataFormatGroup->addButton(isDataBinaryCheckBox, true);
            dataFormatGroup->addButton(isDataFormattedCheckBox, false);
            isDataBinaryCheckBox->setChecked(settings.isDataBinary);
        }
    }

    { /// Initialize
        for (QObject *obj : { (QObject *)dataFormatGroup, (QObject *)connectionTypeGroup,
                              (QObject *)socketTypeGroup, (QObject *)hostEdit, (QObject *)portEdit,
                              (QObject *)progEdit, (QObject *)argsEdit }) {

            if (auto edit = qobject_cast<QLineEdit *>(obj)) {
                connect(edit, &QLineEdit::textChanged, updateEnabledState);
            } else if (auto group = qobject_cast<QButtonGroup *>(obj)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                connect(group, &QButtonGroup::idToggled, updateEnabledState);
#else
                connect(group, QOverload<int, bool>::of(&QButtonGroup::buttonToggled),
                        updateEnabledState);
#endif
            }
        }
        updateEnabledState();
        loadSettings(settings);
    }

    { /// Dialog button box
        outerLayout->addWidget(dialogButtonBox);
        dialogButtonBox->setOrientation(Qt::Horizontal);

        { /// Reconnect button
            updateConnectionButton->setText("Save and Update Connection");
            connect(updateConnectionButton, &QPushButton::clicked, [=] {
                if (saveSettings()) {
                    APP->dataProcessor()->updateSampleReader();
                }
            });
        }

        { /// Reset button
            connect(resetButton, &QPushButton::clicked, [=] {
                loadSettings(m_oldSamplerSettings);
                updateEnabledState();
            });
        }

        { /// Restore defaults button
            connect(restoreDefaultsButton, &QPushButton::clicked, [=] {
                loadSettings(SamplerSettings());
                updateEnabledState();
            });
        }
    }
    return page;
}

Float sampleMagnitude(USize signalIndex)
{
    const auto &phasors = APP->dataProcessor()->lastEstimation().phasors;
    return std::abs(phasors[signalIndex]);
}

QWidget *SettingsWidget::calibrationWidget(const USize signalIndex,
                                           const CalibrationSettings::DataPerSignal &signalData)
{
#define FORMAT_NUMBER(x) (QString::number((x), 'f', 3))
    auto widget = new QWidget();
    auto outerLayout = new QVBoxLayout(widget);

    auto dataSection = new QHBoxLayout();
    auto slopeEdit = new QLineEdit();
    auto interceptEdit = new QLineEdit();

    auto table = new QTableWidget();

    auto tableButtonBox = new QDialogButtonBox();
    auto newRowButton = tableButtonBox->addButton(QDialogButtonBox::Open);
    auto removeRowButton = tableButtonBox->addButton(QDialogButtonBox::Discard);
    auto updateSampleButton = tableButtonBox->addButton(QDialogButtonBox::Retry);
    auto calibrateButton = tableButtonBox->addButton(QDialogButtonBox::Apply);

    auto dialogButtonBox = new QDialogButtonBox();
    auto saveButton = dialogButtonBox->addButton(QDialogButtonBox::Save);
    auto resetButton = dialogButtonBox->addButton(QDialogButtonBox::Reset);
    auto restoreDefaultsButton = dialogButtonBox->addButton(QDialogButtonBox::RestoreDefaults);

    /// Common operations

    /// * Validate row
    auto isValidRowData = [=](int row) {
        if (row < 0 || row >= table->rowCount()) {
            return true;
        }

        auto sampleItem = table->item(row, 0);
        auto actualItem = table->item(row, 1);
        if (!sampleItem || !actualItem) {
            return false;
        }
        if (sampleItem->text().isEmpty() || actualItem->text().isEmpty()) {
            return false;
        }

        bool ok1 = false;
        auto v1 = sampleItem->text().toDouble(&ok1);
        bool ok2 = false;
        auto v2 = actualItem->text().toDouble(&ok2);

        if (!ok1 || !ok2) {
            return false;
        }

        if (v1 < 0 || v2 < 0) {
            return false;
        }

        return true;
    };

    /// * Create and return new row items (without adding to table)
    auto newRow = [=]() -> std::pair<QTableWidgetItem *, QTableWidgetItem *> {
        /// Sample magnitude cell
        auto sampleItem = new QTableWidgetItem();
        sampleItem->setTextAlignment(Qt::AlignRight);
        sampleItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        sampleItem->setText(FORMAT_NUMBER(sampleMagnitude(signalIndex)));

        /// Actual magnitude cell
        auto actualItem = new QTableWidgetItem();
        actualItem->setTextAlignment(Qt::AlignRight);
        actualItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled | Qt::ItemIsSelectable
                             | Qt::ItemIsEditable);
        actualItem->setToolTip(
                QSL("Enter the actual %1").arg(NameOfSignalType[TypeOfSignal[signalIndex]]));
        actualItem->setText(FORMAT_NUMBER(0.0));

        return std::make_pair(sampleItem, actualItem);
    };

    /// * Extract settings data from UI
    auto extractSettingsSignalData = [=] {
        CalibrationSettings::DataPerSignal data;
        data.slope = slopeEdit->text().toDouble();
        data.intercept = interceptEdit->text().toDouble();
        for (int row = 0; row < table->rowCount(); ++row) {
            auto sampleItem = table->item(row, 0);
            auto actualItem = table->item(row, 1);
            if (sampleItem && actualItem) {
                auto x = sampleItem->text().toDouble();
                auto y = actualItem->text().toDouble();
                data.points.append(QPointF(x, y));
            }
        }
        return data;
    };

    /// * Check if settings are changed
    auto isSettingsChanged = [=] {
        return extractSettingsSignalData() != m_oldCalibrationSettings.data[signalIndex];
    };

    /// * Validate table data
    auto isValidTableData = [=] {
        auto rowCount = table->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            if (!isValidRowData(row)) {
                return false;
            }
        }
        return true;
    };

    /// * Update enabled state of buttons
    auto updateEnabledState = [=] {
        auto rowCount = table->rowCount();
        newRowButton->setEnabled(rowCount < (int)CalibrationSettings::MaxPoints);
        removeRowButton->setEnabled(rowCount > 0 && table->currentRow() >= 0);
        updateSampleButton->setEnabled(rowCount > 0 && table->currentRow() >= 0);
        calibrateButton->setEnabled(rowCount > 1 && isValidTableData());
        saveButton->setEnabled(isSettingsChanged());
    };

    /// * Load settings data into UI
    auto loadSettingsSignalData = [=](CalibrationSettings::DataPerSignal data) {
        slopeEdit->setText(FORMAT_NUMBER(data.slope));
        interceptEdit->setText(FORMAT_NUMBER(data.intercept));
        table->setRowCount(0);
        for (const auto &point : data.points) {
            auto row = table->rowCount();
            if (row >= (int)CalibrationSettings::MaxPoints) {
                break;
            }
            auto [sampleItem, actualItem] = newRow();
            table->insertRow(row);
            table->setItem(row, 0, sampleItem);
            table->setItem(row, 1, actualItem);
            if (sampleItem && actualItem) {
                sampleItem->setText(FORMAT_NUMBER(point.x()));
                actualItem->setText(FORMAT_NUMBER(point.y()));
            }
        }
        updateEnabledState();
    };

    /// * Update sample magnitude
    auto updateSampleMagnitude = [=] {
        auto row = table->currentRow();
        if (row >= 0) {
            auto sampleItem = table->item(row, 0);
            if (sampleItem) {
                sampleItem->setText(FORMAT_NUMBER(sampleMagnitude(signalIndex)));
            }
            updateEnabledState();
        }
    };

    { /// Data section
        outerLayout->addLayout(dataSection);
        loadSettingsSignalData(signalData);

        { /// Column 1: Results
            auto resultsForm = new QFormLayout();
            dataSection->addLayout(resultsForm);

            resultsForm->addRow("Slope", slopeEdit);
            resultsForm->addRow("Intercept", interceptEdit);
            slopeEdit->setText(FORMAT_NUMBER(signalData.slope));
            interceptEdit->setText(FORMAT_NUMBER(signalData.intercept));

            for (auto edit : { slopeEdit, interceptEdit }) {
                edit->setValidator(new QDoubleValidator());
                edit->setAlignment(Qt::AlignRight);
                edit->setReadOnly(true);
                edit->setFixedWidth(100);
                connect(edit, &QLineEdit::textChanged, [=] { updateEnabledState(); });
            }
        }

        { /// Column 2: Table of data points
            dataSection->addWidget(table, 1);
            table->setRowCount(0);
            table->setColumnCount(2); // Sample, Actual
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
            table->setSortingEnabled(true);
            table->setSelectionBehavior(QAbstractItemView::SelectRows);
            table->setSelectionMode(QAbstractItemView::SingleSelection);

            auto headerLabels = QStringList();
            headerLabels << QSL("Sample Value");
            headerLabels << QSL("Actual %1 (%2)")
                                    .arg(NameOfSignalType[TypeOfSignal[signalIndex]])
                                    .arg(UnitSymbolOfSignalType[TypeOfSignal[signalIndex]]);
            table->setHorizontalHeaderLabels(headerLabels);
            connect(table, &QTableWidget::itemSelectionChanged, updateEnabledState);
        }

        { /// Column 3: Table button box
            dataSection->addWidget(tableButtonBox);
            tableButtonBox->setOrientation(Qt::Vertical);

            { /// Add new row button
                newRowButton->setText(QSL("Add new row"));
                newRowButton->setIcon(QIcon(":/add.png"));
                connect(newRowButton, &QPushButton::clicked, [=] {
                    auto row = table->rowCount();
                    if (row < (int)CalibrationSettings::MaxPoints) {
                        auto [sampleItem, actualItem] = newRow();
                        table->insertRow(row);
                        table->setItem(row, 0, sampleItem);
                        table->setItem(row, 1, actualItem);
                        updateEnabledState();
                    }
                });
            }

            { /// Remove row button
                removeRowButton->setText(QSL("Remove row"));
                removeRowButton->setIcon(QIcon(":/remove.png"));
                connect(removeRowButton, &QPushButton::clicked, [=] {
                    auto row = table->currentRow();
                    if (row >= 0) {
                        table->removeRow(row);
                    }
                });
            }

            { /// Button to update sample value
                updateSampleButton->setText(QSL("Re-fetch sample"));
                updateSampleButton->setIcon(QIcon(":/refresh.png"));
                connect(updateSampleButton, &QPushButton::clicked, updateSampleMagnitude);
            }

            { /// Calibrate button
                calibrateButton->setText(QSL("Calibrate"));
                calibrateButton->setIcon(QIcon(":/calibrate.png"));
                connect(calibrateButton, &QPushButton::clicked, [=] {
                    auto rows = table->rowCount();
                    auto samples = std::vector<Float>();
                    auto actuals = std::vector<Float>();

                    for (int row = 0; row < rows; ++row) {
                        auto sampleItem = table->item(row, 0);
                        auto actualItem = table->item(row, 1);

                        if (sampleItem && actualItem) {
                            samples.push_back(sampleItem->text().toDouble());
                            actuals.push_back(actualItem->text().toDouble());
                        }
                    }

                    auto [slope, intercept] = linearRegression(samples, actuals);
                    slopeEdit->setText(QString::number(slope, 'f', 2));
                    interceptEdit->setText(QString::number(intercept, 'f', 2));
                });
            }
        }
    }

    { /// Dialog buttons section
        outerLayout->addWidget(dialogButtonBox);
        dialogButtonBox->setOrientation(Qt::Horizontal);

        { /// Save button
            connect(saveButton, &QPushButton::clicked, [=] {
                auto newSettingsData = extractSettingsSignalData();
                auto newSettings = m_oldCalibrationSettings;
                newSettings.data[signalIndex] = newSettingsData;

                if (newSettings.save()) {
                    m_oldCalibrationSettings = newSettings;
                    saveButton->setEnabled(false);
                }
            });
        }

        { /// Reset button
            connect(resetButton, &QPushButton::clicked,
                    [=] { loadSettingsSignalData(m_oldCalibrationSettings.data[signalIndex]); });
        }

        { /// Restore defaults button
            connect(restoreDefaultsButton, &QPushButton::clicked,
                    [=] { loadSettingsSignalData(CalibrationSettings::DataPerSignal()); });
        }

        return widget;
    }
}

#undef FORMAT_NUMBER
#undef QSL