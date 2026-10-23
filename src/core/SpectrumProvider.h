#pragma once

#include <QObject>
#include <QVector>

/** Attack/release smoothing + normalization, shared by real and simulated data. */
class SpectrumProvider : public QObject
{
    Q_OBJECT
public:
    explicit SpectrumProvider(QObject *parent = nullptr) : QObject(parent) {}

    QVector<qreal> process(const QVector<qreal> &in)
    {
        if (in.size() != m_smoothed.size())
            m_smoothed = QVector<qreal>(in.size(), 0.0);
        for (int i = 0; i < in.size(); ++i) {
            const qreal x = qBound(0.0, in[i], 1.0);
            const qreal coef = x > m_smoothed[i] ? m_attack : m_release;
            m_smoothed[i] = qBound(0.0, m_smoothed[i] * coef + x * (1.0 - coef), 1.0);
        }
        return m_smoothed;
    }

    void reset()
    {
        std::fill(m_smoothed.begin(), m_smoothed.end(), 0.0);
    }

private:
    QVector<qreal> m_smoothed;
    const qreal m_attack = 0.55;
    const qreal m_release = 0.80;
};
