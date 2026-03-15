#include "settings_models.h"
#include "qpmu/defs.h"

#include <QFileInfo>
#include <QHostAddress>

#define QSL QStringLiteral

// --------------------------------------------------------
QStringList parsePrcoessString(const QString &processString)
{
    QStringList tokens;
    QString currToken;
    bool inQuotes = false;
    QChar lastChar = {};
    for (auto c : processString) {
        if (c == ' ' && !inQuotes) {
            if (!currToken.isEmpty()) {
                tokens.append(currToken);
                currToken.clear();
            }
        } else {
            currToken.append(c);
            if (c == '"' && lastChar != '\\') {
                inQuotes = !inQuotes;
            }
        }
        lastChar = c;
    }
    if (!currToken.isEmpty()) {
        tokens.append(currToken);
    }
    return tokens;
}

void SamplerSettings::load(QSettings settings)
{
    settings.beginGroup(QSL("sample_source"));

    auto connString = settings.value(QSL("connection"), "none").toString();
    if (connString == QSL("socket")) {
        connection = Socket;
    } else if (connString == QSL("process")) {
        connection = Process;
    } else {
        connection = None;
    }

    isDataBinary = settings.value(QSL("is_data_binary"), true).toBool();

    if (settings.contains(QSL("process"))) {
        auto processString = settings.value(QSL("process")).toString();
        QStringList parts = parsePrcoessString(processString);
        if (!parts.isEmpty()) {
            processConfig.prog = parts.takeFirst();
            processConfig.args = parts;
        }
    }

    if (settings.contains(QSL("socket"))) {
        auto socketSting = settings.value(QSL("socket")).toString();
        auto parts = socketSting.split(':');
        socketConfig.socketType =
                (parts.size() >= 1 && parts[0] == QSL("tcp")) ? TcpSocket : UdpSocket;
        socketConfig.host = (parts.size() >= 2) ? parts[1] : "127.0.0.1";
        socketConfig.port = (parts.size() >= 3) ? parts[2].toUShort() : 12345;
    }

    settings.endGroup();
}

bool SamplerSettings::save() const
{
    if (!validate().isEmpty()) {
        return false;
    }
    QSettings settings;
    settings.beginGroup(QSL("sample_source"));

    settings.setValue(QSL("socket"),
                      QSL("%1:%2:%3")
                              .arg((socketConfig.socketType == TcpSocket) ? QSL("tcp") : QSL("udp"))
                              .arg(socketConfig.host)
                              .arg(socketConfig.port));

    settings.setValue(QSL("process"),
                      QSL("%1 %2").arg(processConfig.prog).arg(processConfig.args.join(QSL(" "))));

    if (connection == Socket) {
        settings.setValue(QSL("connection"), QSL("socket"));
    } else if (connection == Process) {
        settings.setValue(QSL("connection"), QSL("process"));
    } else {
        settings.setValue(QSL("connection"), QSL("none"));
    }

    settings.setValue(QSL("is_data_binary"), isDataBinary);

    settings.endGroup();
    return true;
}

QString SamplerSettings::validate() const
{
    if (connection == Socket) {
        if (QHostAddress(socketConfig.host).isNull()) {
            return "Invalid host address";
        }
    } else if (connection == Process) {
        if (!QFileInfo(processConfig.prog).isExecutable()) {
            return "Invalid program path; file not found or not executable";
        }
    }
    return "";
}

// --------------------------------------------------------

void CalibrationSettings::load(QSettings settings)
{
    settings.beginGroup(QSL("calibration"));
    for (qpmu::USize i = 0; i < qpmu::CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(qpmu::NameOfSignal[i]));

        data[i].slope = settings.value(QSL("slope"), 1.0).toDouble();
        data[i].intercept = settings.value(QSL("intercept"), 0.0).toDouble();
        data[i].points.clear();
        for (const auto &point : settings.value(QSL("points")).toList()) {
            if (point.canConvert<QPointF>()) {
                data[i].points.append(point.toPointF());
            }
        }

        settings.endGroup();
    }
    settings.endGroup();
}

bool CalibrationSettings::save() const
{
    if (!validate().isEmpty()) {
        return false;
    }
    QSettings settings;
    settings.beginGroup(QSL("calibration"));
    for (qpmu::USize i = 0; i < qpmu::CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(qpmu::NameOfSignal[i]));
        settings.setValue(QSL("slope"), data[i].slope);
        settings.setValue(QSL("intercept"), data[i].intercept);
        settings.setValue(QSL("points"), QVariant::fromValue(data[i].points));
        settings.endGroup();
    }
    settings.endGroup();
    return true;
}

// --------------------------------------------------------

void VisualisationSettings::load(QSettings settings)
{
    settings.beginGroup(QSL("visualisation"));
    for (qpmu::USize i = 0; i < qpmu::CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(qpmu::NameOfSignal[i]));
        signalColors[i] = settings.value(QSL("color"), signalColors[i]).value<QColor>();
        settings.endGroup();
    }
    settings.endGroup();
}

QString VisualisationSettings::validate() const
{
    QStringList errors;
    for (qpmu::USize i = 0; i < qpmu::CountSignals; ++i) {
        if (!signalColors[i].isValid()) {
            errors << QSL("Invalid color for %1: %2")
                              .arg(qpmu::NameOfSignal[i], signalColors[i].name());
        }
    }
    return errors.join('\n');
}

bool VisualisationSettings::save() const
{
    if (!validate().isEmpty()) {
        return false;
    }
    QSettings settings;
    settings.beginGroup(QSL("visualisation"));
    for (qpmu::USize i = 0; i < qpmu::CountSignals; ++i) {
        settings.beginGroup(QSL("%1").arg(qpmu::NameOfSignal[i]));
        settings.setValue(QSL("color"), signalColors[i]);
        settings.endGroup();
    }
    settings.endGroup();
    return true;
}

#undef QSL