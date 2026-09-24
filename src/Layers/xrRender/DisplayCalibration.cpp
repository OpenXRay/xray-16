#include "stdafx.h"
#include "DisplayCalibration.h"

namespace xray::render::fg
{
void DisplayCalibration::SetGamma(float gamma) { m_gamma = gamma; }
void DisplayCalibration::SetBrightness(float brightness) { m_brightness = brightness; }
void DisplayCalibration::SetContrast(float contrast) { m_contrast = contrast; }

float DisplayCalibration::GetGamma() const { return m_gamma; }
float DisplayCalibration::GetBrightness() const { return m_brightness; }
float DisplayCalibration::GetContrast() const { return m_contrast; }
}
