#pragma once

#include <QVector>

/** In-place radix-2 FFT with Hann windowing. */
class FftAnalyzer
{
public:
    void init(int n);
    int size() const { return m_n; }

    /** Process n mono samples; output magnitude spectrum of n/2+1 bins. */
    void magnitude(const float *input, float *output);

private:
    int m_n = 0;
    QVector<float> m_hann;
    QVector<float> m_re;
    QVector<float> m_im;
    QVector<int> m_reverse;
};
