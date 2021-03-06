/*
 * Copyright (c) 2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include "audioloudnessscopewidget.h"
#include <Logger.h>
#include <QVBoxLayout>
#include <QQmlEngine>
#include <QDir>
#include <QQuickWidget>
#include <QQuickItem>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <MltProfile.h>
#include <math.h>
#include <qmlutilities.h>
#include "mltcontroller.h"
#include "settings.h"

static double onedec( double in )
{
	return round( in * 10.0 ) / 10.0;
}

AudioLoudnessScopeWidget::AudioLoudnessScopeWidget()
  : ScopeWidget("AudioLoudnessMeter")
  , m_loudnessFilter(nullptr)
  , m_peak(-100)
  , m_true_peak(-100)
  , m_newData(false)
  , m_orientation(static_cast<Qt::Orientation>(-1))
  , m_qview(new QQuickWidget(QmlUtilities::sharedEngine(), this))
  , m_timeLabel(new QLabel(this))
{
    LOG_DEBUG() << "begin";
    m_loudnessFilter = new Mlt::Filter(MLT.profile(), "loudness_meter");
    m_loudnessFilter->set("calc_program", Settings.loudnessScopeShowMeter("integrated"));
    m_loudnessFilter->set("calc_shortterm", Settings.loudnessScopeShowMeter("shortterm"));
    m_loudnessFilter->set("calc_momentary", Settings.loudnessScopeShowMeter("momentary"));
    m_loudnessFilter->set("calc_range", Settings.loudnessScopeShowMeter("range"));
    m_loudnessFilter->set("calc_peak", Settings.loudnessScopeShowMeter("peak"));
    m_loudnessFilter->set("calc_true_peak", Settings.loudnessScopeShowMeter("truepeak"));

    setAutoFillBackground(true);

    // Use a timer to update the meters for two reasons:
    // 1) The spec requires 10Hz updates
    // 2) Minimize QML GUI updates
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateMeters()));
    m_timer->start(100);

    m_qview->setFocusPolicy(Qt::StrongFocus);
    QmlUtilities::setCommonProperties(m_qview->rootContext());

    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(4, 4, 4, 4);
    vlayout->addWidget(m_qview);

    QHBoxLayout* hlayout = new QHBoxLayout();
    vlayout->addLayout(hlayout);

    // Create config menu
    QMenu* configMenu = new QMenu(this);
    QAction* action;
    action = configMenu->addAction(tr("Momentary Loudness"), this, SLOT(onMomentaryToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.loudnessScopeShowMeter("momentary"));
    action = configMenu->addAction(tr("Short Term Loudness"), this, SLOT(onShorttermToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.loudnessScopeShowMeter("shortterm"));
    action = configMenu->addAction(tr("Integrated Loudness"), this, SLOT(onIntegratedToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.loudnessScopeShowMeter("integrated"));
    action = configMenu->addAction(tr("Loudness Range"), this, SLOT(onRangeToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.loudnessScopeShowMeter("range"));
    action = configMenu->addAction(tr("Peak"), this, SLOT(onPeakToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.loudnessScopeShowMeter("peak"));
    action = configMenu->addAction(tr("True Peak"), this, SLOT(onTruePeakToggled(bool)));
    action->setCheckable(true);
    action->setChecked(Settings.loudnessScopeShowMeter("truepeak"));

    // Add config button
    QToolButton* configButton = new QToolButton(this);
    configButton->setToolTip(tr("Configure Graphs"));
    configButton->setIcon(QIcon(":/icons/oxygen/16x16/actions/format-justify-fill.png"));
    configButton->setPopupMode(QToolButton::InstantPopup);
    configButton->setMenu(configMenu);
    hlayout->addWidget(configButton);

    // Add reset button
    QPushButton* resetButton = new QPushButton(tr("Reset"), this);
    resetButton->setToolTip(tr("Reset the measurement."));
    resetButton->setCheckable(false);
    resetButton->setMaximumWidth(100);
    hlayout->addWidget(resetButton);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(onResetButtonClicked()));

    // Add time label
    m_timeLabel->setToolTip(tr("Time Since Reset"));
    m_timeLabel->setText("00:00:00:00");
    m_timeLabel->setFixedSize(this->fontMetrics().width("HH:MM:SS:MM"), this->fontMetrics().height());
    hlayout->addWidget(m_timeLabel);

    hlayout->addStretch();

    connect(m_qview->quickWindow(), SIGNAL(sceneGraphInitialized()), SLOT(resetQview()));

    LOG_DEBUG() << "end";
}

AudioLoudnessScopeWidget::~AudioLoudnessScopeWidget()
{
    m_timer->stop();
    delete m_loudnessFilter;
}

void AudioLoudnessScopeWidget::refreshScope(const QSize& /*size*/, bool /*full*/)
{
    SharedFrame sFrame;
    while (m_queue.count() > 0) {
        sFrame = m_queue.pop();
        if (sFrame.is_valid() && sFrame.get_audio_samples() > 0) {
            mlt_audio_format format = mlt_audio_f32le;
            int channels = sFrame.get_audio_channels();
            int frequency = sFrame.get_audio_frequency();
            int samples = sFrame.get_audio_samples();
            if (channels && frequency && samples) {
                Mlt::Frame mFrame = sFrame.clone(true, false, false);
                m_loudnessFilter->process(mFrame);
                mFrame.get_audio(format, frequency, channels, samples);
                if( m_peak < m_loudnessFilter->get_double("peak") ) {
                    m_peak = m_loudnessFilter->get_double("peak");
                }
                if( m_true_peak < m_loudnessFilter->get_double("true_peak") ) {
                    m_true_peak = m_loudnessFilter->get_double("true_peak");
                }
                m_newData = true;
            }
        }
    }

    // Update the time with every frame.
    m_timeLabel->setText( m_loudnessFilter->get_time( "frames_processed" ) );
}

QString AudioLoudnessScopeWidget::getTitle()
{
   return tr("Audio Loudness");
}

void AudioLoudnessScopeWidget::setOrientation(Qt::Orientation orientation)
{
    if (orientation != m_orientation) {
        if (orientation == Qt::Vertical) {
            setMinimumSize(300, 250);
            setMaximumSize(300, 500);
        } else {
            setMinimumSize(250, 320);
            setMaximumSize(500, 320);
        }
        updateGeometry();
        m_orientation = orientation;
    }
}

void AudioLoudnessScopeWidget::onResetButtonClicked()
{
    m_loudnessFilter->set("reset", 1);
    m_timeLabel->setText( "00:00:00:00" );
    resetQview();
}

void AudioLoudnessScopeWidget::onIntegratedToggled(bool checked)
{
    m_loudnessFilter->set("calc_program", checked);
    Settings.setLoudnessScopeShowMeter("integrated", checked);
    resetQview();
}

void AudioLoudnessScopeWidget::onShorttermToggled(bool checked)
{
    m_loudnessFilter->set("calc_shortterm", checked);
    Settings.setLoudnessScopeShowMeter("shortterm", checked);
    resetQview();
}

void AudioLoudnessScopeWidget::onMomentaryToggled(bool checked)
{
    m_loudnessFilter->set("calc_momentary", checked);
    Settings.setLoudnessScopeShowMeter("momentary", checked);
    resetQview();
}

void AudioLoudnessScopeWidget::onRangeToggled(bool checked)
{
    m_loudnessFilter->set("calc_range", checked);
    Settings.setLoudnessScopeShowMeter("range", checked);
    resetQview();
}

void AudioLoudnessScopeWidget::onPeakToggled(bool checked)
{
    m_loudnessFilter->set("calc_peak", checked);
    Settings.setLoudnessScopeShowMeter("peak", checked);
    resetQview();
}

void AudioLoudnessScopeWidget::onTruePeakToggled(bool checked)
{
    m_loudnessFilter->set("calc_true_peak", checked);
    Settings.setLoudnessScopeShowMeter("truepeak", checked);
    resetQview();
}

void AudioLoudnessScopeWidget::updateMeters(void)
{
    if (!m_newData) return;
    if (m_loudnessFilter->get_int("calc_program") )
        m_qview->rootObject()->setProperty("integrated", onedec(m_loudnessFilter->get_double("program")));
    if (m_loudnessFilter->get_int("calc_shortterm") )
        m_qview->rootObject()->setProperty("shortterm", onedec(m_loudnessFilter->get_double("shortterm")));
    if (m_loudnessFilter->get_int("calc_momentary") )
        m_qview->rootObject()->setProperty("momentary", onedec(m_loudnessFilter->get_double("momentary")));
    if (m_loudnessFilter->get_int("calc_range") )
        m_qview->rootObject()->setProperty("range", onedec(m_loudnessFilter->get_double("range")));
    if (m_loudnessFilter->get_int("calc_peak") )
        m_qview->rootObject()->setProperty("peak", onedec(m_peak));
    if (m_loudnessFilter->get_int("calc_true_peak") )
        m_qview->rootObject()->setProperty("truePeak", onedec(m_true_peak));
    m_peak = -100;
    m_true_peak = -100;
    m_newData = false;
}

bool AudioLoudnessScopeWidget::event(QEvent *event)
{
    bool result = ScopeWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        resetQview();
    }
    return result;
}

void AudioLoudnessScopeWidget::resetQview()
{
    LOG_DEBUG() << "begin";
    if (m_qview->status() != QQuickWidget::Null) {
        m_qview->setSource(QUrl(""));
    }

    QDir viewPath = QmlUtilities::qmlDir();
    viewPath.cd("scopes");
    viewPath.cd("audioloudness");
    m_qview->engine()->addImportPath(viewPath.path());

    QDir modulePath = QmlUtilities::qmlDir();
    modulePath.cd("modules");
    m_qview->engine()->addImportPath(modulePath.path());

    m_qview->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qview->quickWindow()->setColor(palette().window().color());
    QUrl source = QUrl::fromLocalFile(viewPath.absoluteFilePath("audioloudnessscope.qml"));
    m_qview->setSource(source);

    m_qview->rootObject()->setProperty("enableIntegrated", Settings.loudnessScopeShowMeter("integrated"));
    m_qview->rootObject()->setProperty("enableShortterm", Settings.loudnessScopeShowMeter("shortterm"));
    m_qview->rootObject()->setProperty("enableMomentary", Settings.loudnessScopeShowMeter("momentary"));
    m_qview->rootObject()->setProperty("enableRange", Settings.loudnessScopeShowMeter("range"));
    m_qview->rootObject()->setProperty("enablePeak", Settings.loudnessScopeShowMeter("peak"));
    m_qview->rootObject()->setProperty("enableTruePeak", Settings.loudnessScopeShowMeter("truepeak"));
}
