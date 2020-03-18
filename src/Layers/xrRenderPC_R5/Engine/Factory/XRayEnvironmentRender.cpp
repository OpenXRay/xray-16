#include "pch.h"

XRayEnvironmentRender::XRayEnvironmentRender()
{
}

void XRayEnvironmentRender::Copy(IEnvironmentRender & _in)
{
}

void XRayEnvironmentRender::OnFrame(CEnvironment& env)
{
}



void XRayEnvironmentRender::OnLoad()
{
}

void XRayEnvironmentRender::OnUnload()
{
}

void XRayEnvironmentRender::RenderSky(CEnvironment& env)
{
}

void XRayEnvironmentRender::RenderClouds(CEnvironment& env)
{
}


void XRayEnvironmentRender::OnDeviceCreate()
{
}

void XRayEnvironmentRender::OnDeviceDestroy()
{
}
particles_systems::library_interface *null = 0;
particles_systems::library_interface const & XRayEnvironmentRender::particles_systems_library()
{
	return *null;
}
