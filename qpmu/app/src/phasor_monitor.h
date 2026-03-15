#ifndef QPMU_APP_PHASOR_MONITOR_H
#define QPMU_APP_PHASOR_MONITOR_H

#include "qpmu/defs.h"
#include "main_page_interface.h"

#include <QWidget>
#include <QtCharts>
#include <QVector>
#include <QVector>
#include <QLabel>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>

#include <array>
#include <qcheckbox.h>

class PhasorMonitor : public QWidget, public MainPageInterface
{
    Q_OBJECT
    Q_INTERFACES(MainPageInterface)

public:
    explicit PhasorMonitor(QWidget *parent = nullptr);

    QVector<SidePanelItem> sidePanelItems() const override;

private slots:
    // void setPlotVisibility(bool phasorsVisibility, bool waveformsVisibility,
    //                        bool connectorsVisibility);
    // void setSignalVisibility(const std::array<bool, qpmu::CountSignals> &signalVisibility);

    void updateVisibility();
    void updateData(const qpmu::Estimation &estimation);

private:
    using PointVector = QVector<QPointF>;

    void createChartView();
    void createSummaryBar();
    void createTable();
    void createControls();

    QVBoxLayout *m_outerLayout;

    bool m_playing = true;

    struct
    {
        struct
        {
            QGroupBox *box = nullptr;

            QCheckBox *checkSignalsOfType[qpmu::CountSignalTypes][qpmu::CountSignalPhases + 1] = {
                { nullptr }
            };
            QCheckBox *checkPhasors = nullptr;
            QCheckBox *checkWaveforms = nullptr;
            QCheckBox *checkConnectors = nullptr;

            QVector<QLineSeries *> bucket[2]; // 0 -> hide, 1 -> show
        } visibility;

        struct
        {
            QGroupBox *box = nullptr;

            QComboBox *voltage = nullptr;
            QComboBox *current = nullptr;
            QCheckBox *checkApplyToTable = nullptr;
        } phaseRef;

        struct
        {
            QGroupBox *box = nullptr;

            QComboBox *voltage = nullptr;
            QComboBox *current = nullptr;
        } ampliScale;

    } m_ctrl;

    struct
    {
        QVector<QLineSeries *> phasor;
        QVector<QLineSeries *> waveform;
    } m_fakeAxesSeriesList[2]; // index 0 when both phasor and waveform are present, index 1 when
                               // only one of them is present

    struct
    {
        QLineSeries *phasors[qpmu::CountSignals];
        QLineSeries *waveforms[qpmu::CountSignals];
        QLineSeries *connectors[qpmu::CountSignals];
    } m_seriesList[2];

    struct
    {
        PointVector phasors[qpmu::CountSignals];
        PointVector waveforms[qpmu::CountSignals];
        PointVector connectors[qpmu::CountSignals];
    } m_seriesPointList[2];

    struct
    {
        QLabel *ampli[qpmu::CountSignals];
        QLabel *phase[qpmu::CountSignals];
        QLabel *frequ[qpmu::CountSignals];
        QLabel *phaseDiff[qpmu::CountSignalPhases];

        QLabel *summaryFrequency;
        QLabel *summarySamplingRate;
    } m_labels;

    QVector<QLabel *> m_colorLabels[qpmu::CountSignals];
};

#endif // QPMU_APP_PHASOR_MONITOR_H