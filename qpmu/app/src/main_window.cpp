#include "main_window.h"
#include "main_page_interface.h"
#include "phasor_monitor.h"
#include "settings_widget.h"
#include "data_processor.h"
#include "src/app.h"
#include "util.h"

#include <QIcon>
#include <QPushButton>
#include <QToolButton>
#include <QLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QDockWidget>
#include <QLabel>
#include <QSettings>
#include <QToolBox>
#include <QStackedWidget>
#include <QToolBar>
#include <QString>
#include <QIcon>
#include <QBoxLayout>
#include <QSplitter>
#include <qpixmap.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{

    auto centralWidgetSplitter = new QSplitter(this);
    auto toolBar = new QToolBar();
    auto mainPageStack = new QStackedWidget();
    auto sidePanel = new QToolBox();
    auto toggleSidePanelAction = new QAction();
    auto goBackAction = new QAction();

    { /// Status bar
        statusBar(); /// first call to `statusBar()` creates and sets it
        statusBar()->setSizeGripEnabled(true);
        statusBar()->setContentsMargins(QMargins(5, 0, 0, 0));

        /// 1. Sampling indicators
        statusBar()->addPermanentWidget(new QLabel("Sampling:", statusBar()));
        auto createSamplingIndicator = [=](const QString &name, SampleReader::StateFlag flag) {
            const auto redCircle = circlePixmap(QColor("red"), 12);
            const auto greenCircle = circlePixmap(QColor("green"), 12);
            const QPixmap pixmapChoices[2] = { redCircle, greenCircle };
            auto indicator = new QLabel(statusBar());
            statusBar()->addPermanentWidget(indicator);
            indicator->setToolTip(name + "?");
            auto updateIndicator = [=](int state) {
                indicator->setPixmap(pixmapChoices[bool(state & flag)]);
            };
            connect(APP->dataProcessor(), &DataProcessor::sampleReaderStateChanged,
                    updateIndicator);
            return updateIndicator;
        };
        for (auto f : { createSamplingIndicator("Enabled", SampleReader::Enabled),
                        createSamplingIndicator("Connected", SampleReader::Connected),
                        createSamplingIndicator("Data Reading", SampleReader::DataReading),
                        createSamplingIndicator("Data Valid", SampleReader::DataValid) }) {
            f(APP->dataProcessor()->sampleReaderState());
        }

        /// 3. Spacer
        auto spacer = new QWidget(statusBar());
        // spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        statusBar()->addPermanentWidget(spacer, 1);

        /// 4. Time and date
        auto timeLabel = new QLabel(statusBar());
        statusBar()->addPermanentWidget(timeLabel);
        auto dateLabel = new QLabel(statusBar());
        statusBar()->addPermanentWidget(dateLabel);

        { /// Set up date and time labels

            /// Make font monospace
            auto font = dateLabel->font();
            font.setFamily("'Source Sans Pro', 'Roboto Mono', 'Fira Code', Consolas, Monospace");
            dateLabel->setFont(font);
            timeLabel->setFont(font);

            /// Update date and time at every timer tick (check `APP->timer()` for the interval)
            connect(APP->timer(), &QTimer::timeout, [=] {
                auto now = QDateTime::currentDateTime();
                dateLabel->setText(now.date().toString());
                timeLabel->setText(now.time().toString());
            });
        }

        /// Must call show to make the status bar visible
        statusBar()->show();
    }

    { /// Central widget
        setCentralWidget(centralWidgetSplitter);
        centralWidgetSplitter->setOrientation(Qt::Horizontal);
    }

    { /// Side panel (tool box)
        centralWidgetSplitter->addWidget(sidePanel);
        sidePanel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
        sidePanel->setFrameStyle(QFrame::Panel | QFrame::Raised);
        sidePanel->hide();
    }

    { /// Tool bar
        addToolBar(Qt::LeftToolBarArea, toolBar);
        toolBar->setMovable(false);

        { /// Back button
            toolBar->addAction(goBackAction);
            goBackAction->setIcon(QIcon(":/back.png"));
            goBackAction->setText("Back");
            goBackAction->setEnabled(false);
            connect(mainPageStack, &QStackedWidget::currentChanged,
                    [=](int index) { goBackAction->setEnabled(index >= 1); });
            connect(goBackAction, &QAction::triggered, [=] {
                auto i = mainPageStack->currentIndex();
                if (i <= 0) {
                    return;
                }
                mainPageStack->setCurrentIndex(i - 1);
                mainPageStack->removeWidget(mainPageStack->widget(i));
            });
        }

        { /// Toggle side panel button
            toolBar->addAction(toggleSidePanelAction);
            toggleSidePanelAction->setIcon(QIcon(":/more.png"));
            toggleSidePanelAction->setText("Menu");
            toggleSidePanelAction->setCheckable(true);
            connect(toggleSidePanelAction, &QAction::toggled, sidePanel, &QWidget::setVisible);
            toggleSidePanelAction->setVisible(false);
        }
    }

    { /// Main page stack
        centralWidgetSplitter->addWidget(mainPageStack);
        mainPageStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mainPageStack->setContentsMargins(QMargins(20, 10, 20, 10));

        connect(mainPageStack, &QStackedWidget::currentChanged, [=](int index) {
            /// Clear side-panel items, and hide it
            for (int i = 0; i < sidePanel->count(); ++i) {
                sidePanel->removeItem(i);
            }
            toggleSidePanelAction->setChecked(false);
            toggleSidePanelAction->setVisible(false);

            /// If the current page has side-panel items, add them
            if (auto page = qobject_cast<MainPageInterface *>(mainPageStack->widget(index))) {
                const auto &items = page->sidePanelItems();
                for (const auto &[widget, title, icon] : items) {
                    auto index = sidePanel->addItem(widget, title);
                    sidePanel->setItemToolTip(index, title);
                    if (!icon.isNull()) {
                        sidePanel->setItemIcon(index, icon);
                    }
                }
                toggleSidePanelAction->setVisible(items.count() > 0);
            }
        });
    }

    { /// Home page widget (first view)
        auto homePage = new QWidget(mainPageStack);
        mainPageStack->setCurrentIndex(mainPageStack->addWidget(homePage));
        homePage->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        auto homeGrid = new QGridLayout(homePage);
        homeGrid->setSpacing(10);

        using HomeOption = std::tuple<QString, QString, QWidget *>;
        QVector<HomeOption> homeOptions;
        homeOptions.append(HomeOption{ "Monitor", ":/monitor.png", new PhasorMonitor() });
        homeOptions.append(HomeOption{ "Settings", ":/control-panel.png", new SettingsWidget() });

        const int rowCount = 2;
        const int colCount = 2;

        /// Add option buttons to home widget
        for (int i = 0; i < (int)homeOptions.size(); ++i) {
            const auto [title, iconUrl, page] = homeOptions[i];
            page->setParent(this);
            auto button = new QPushButton(homePage);
            auto label = new QLabel(homePage);

            { /// Button with icon
                button->setIcon(QIcon(iconUrl));
                auto size = QSize(80, 80);
                button->setMinimumSize(size);
                button->setMaximumSize(size);
                button->setIconSize(size * 0.9);
                connect(button, &QPushButton::clicked, [=] {
                    const auto [title, iconUrl, page] = homeOptions[i];
                    auto index = mainPageStack->addWidget(page);
                    mainPageStack->setCurrentIndex(index);
                    page->show();
                });
            }

            { /// Label
                label->setText(title);
                label->setAlignment(Qt::AlignCenter);
            }

            { /// Layout to hold button and label
                auto vbox = new QVBoxLayout();
                vbox->addWidget(button);
                vbox->setAlignment(button, Qt::AlignCenter);
                vbox->addWidget(label);
                vbox->setAlignment(label, Qt::AlignTop | Qt::AlignHCenter);
                homeGrid->addLayout(vbox, (i / rowCount), (i % colCount));
                homeGrid->setAlignment(vbox, Qt::AlignCenter);
            }
        }
    }

    resize(1000, 500);
}
