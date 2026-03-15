#ifndef QPMU_APP_SETTINGS_MODELS_H
#define QPMU_APP_SETTINGS_MODELS_H

#include "qpmu/defs.h"
#include "app.h"

#include <QSettings>
#include <QtGlobal>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QtGlobal>

#include <array>
#include <type_traits>

struct AbstractSettingsModel
{
    virtual ~AbstractSettingsModel() = default;

    /// Loads the settings from the settings file.
    virtual void load(QSettings settings) = 0;

    /// Saves the settings to the settings file.
    virtual bool save() const = 0;

    /// Returns true if the settings are valid.
    virtual QString validate() const { return QString(); }
};

#define STATIC_ASSERT_SETTINGS_MODEL_CONCEPTS(T)                                                   \
  static_assert(std::is_default_constructible<T>::value, #T " must be default constructible");     \
  static_assert(std::is_copy_constructible<T>::value, #T " must be trivially copy assignable");    \
  static_assert(std::is_move_constructible<T>::value, #T " must be trivially move constructible"); \
  static_assert(std::is_copy_assignable<T>::value, #T " must be trivially copyable");              \
  static_assert(std::is_move_assignable<T>::value, #T " must be trivially move assignable");

struct SamplerSettings : public AbstractSettingsModel
{
    enum ConnectionType {
        None = 0,
        Socket = 1,
        Process = 2,
    };
    enum SocketType {
        UdpSocket = 0,
        TcpSocket = 1,
    };

    struct SocketConfig
    {
        SocketType socketType = UdpSocket;
        QString host = "127.0.0.1";
        quint16 port = 12345;
    };

    struct ProcessConfig
    {
        QString prog = "";
        QStringList args = {};
    };

    ConnectionType connection = None;
    SocketConfig socketConfig = {};
    ProcessConfig processConfig = {};
    bool isDataBinary = true;

    void load(QSettings settings = QSettings()) override;
    bool save() const override;
    QString validate() const override;

    bool operator==(const SamplerSettings &other) const
    {
        return connection == other.connection
                && socketConfig.socketType == other.socketConfig.socketType
                && socketConfig.host == other.socketConfig.host
                && socketConfig.port == other.socketConfig.port
                && processConfig.prog == other.processConfig.prog
                && processConfig.args == other.processConfig.args
                && isDataBinary == other.isDataBinary;
    }

    bool operator!=(const SamplerSettings &other) const { return !(*this == other); }
};

STATIC_ASSERT_SETTINGS_MODEL_CONCEPTS(SamplerSettings)

struct CalibrationSettings : public AbstractSettingsModel
{
    static constexpr quint32 MaxPoints = 10;
    struct DataPerSignal
    {
        qreal slope = 1.0;
        qreal intercept = 0.0;
        QVector<QPointF> points = {};
    };

    std::array<DataPerSignal, qpmu::CountSignals> data = {};

    void load(QSettings settings = QSettings()) override;
    bool save() const override;

    friend bool operator==(const CalibrationSettings::DataPerSignal &lhs,
                           const CalibrationSettings::DataPerSignal &rhs)
    {
        return lhs.slope == rhs.slope && lhs.intercept == rhs.intercept && lhs.points == rhs.points;
    }

    friend bool operator!=(const CalibrationSettings::DataPerSignal &lhs,
                           const CalibrationSettings::DataPerSignal &rhs)
    {
        return !(lhs == rhs);
    }

    bool operator==(const CalibrationSettings &other) const { return data == other.data; }

    bool operator!=(const CalibrationSettings &other) const { return !(*this == other); }
};
STATIC_ASSERT_SETTINGS_MODEL_CONCEPTS(CalibrationSettings)

struct VisualisationSettings : public AbstractSettingsModel
{

    std::array<QColor, qpmu::CountSignals> signalColors = { "#404040", "#ff0000", "#00ffff",
                                                            "#f1dd38", "#0000ff", "#22bb45" };

    void load(QSettings settings = QSettings()) override;
    bool save() const override;
    QString validate() const override;

    bool operator==(const VisualisationSettings &other) const
    {
        return signalColors == other.signalColors;
    }

    bool operator!=(const VisualisationSettings &other) const { return !(*this == other); }
};
STATIC_ASSERT_SETTINGS_MODEL_CONCEPTS(VisualisationSettings)

QStringList parsePrcoessString(const QString &processString);

#endif // QPMU_APP_SETTINGS_MODELS_H