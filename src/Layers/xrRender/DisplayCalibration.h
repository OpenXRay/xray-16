#pragma once

namespace xray::render::fg
{
class DisplayCalibration
{
public:
    void SetGamma(float gamma);
    void SetBrightness(float brightness);
    void SetContrast(float contrast);

    float GetGamma() const;
    float GetBrightness() const;
    float GetContrast() const;

private:
    float m_gamma = 1.0f;
    float m_brightness = 1.0f;
    float m_contrast = 1.0f;
};
}
