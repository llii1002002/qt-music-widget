#include "FftAnalyzer.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void FftAnalyzer::init(int n)
{
    if (n == m_n)
        return;
    m_n = n;
    const int bits = int(std::log2(double(n)));

    m_hann.resize(n);
    m_re.resize(n);
    m_im.resize(n);
    m_reverse.resize(n);

    for (int i = 0; i < n; ++i) {
        m_hann[i] = 0.5f * (1.0f - float(std::cos(2.0 * M_PI * i / (n - 1))));
        int rev = 0;
        for (int b = 0; b < bits; ++b)
            rev = (rev << 1) | ((i >> b) & 1);
        m_reverse[i] = rev;
    }
}

void FftAnalyzer::magnitude(const float *input, float *output)
{
    const int n = m_n;
    for (int i = 0; i < n; ++i) {
        m_re[i] = input[m_reverse[i]] * m_hann[i];
        m_im[i] = 0.0f;
    }

    for (int len = 2; len <= n; len <<= 1) {
        const int half = len >> 1;
        const double ang = -2.0 * M_PI / len;
        const float wRe = float(std::cos(ang));
        const float wIm = float(std::sin(ang));
        for (int i = 0; i < n; i += len) {
            float curRe = 1.0f, curIm = 0.0f;
            for (int k = 0; k < half; ++k) {
                const float aRe = m_re[i + k];
                const float aIm = m_im[i + k];
                const float bRe = m_re[i + k + half] * curRe - m_im[i + k + half] * curIm;
                const float bIm = m_re[i + k + half] * curIm + m_im[i + k + half] * curRe;
                m_re[i + k] = aRe + bRe;
                m_im[i + k] = aIm + bIm;
                m_re[i + k + half] = aRe - bRe;
                m_im[i + k + half] = aIm - bIm;
                const float nxtRe = curRe * wRe - curIm * wIm;
                curIm = curRe * wIm + curIm * wRe;
                curRe = nxtRe;
            }
        }
    }

    const float norm = 2.0f / n;
    for (int i = 0; i <= n / 2; ++i)
        output[i] = std::sqrt(m_re[i] * m_re[i] + m_im[i] * m_im[i]) * norm;
}
