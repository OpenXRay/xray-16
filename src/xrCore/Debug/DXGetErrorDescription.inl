// XXX: don't use strcpy_s/wcscpy_s functions
#pragma warning(push)
#pragma warning(disable : 4995) // strcpy_s marked as deprecated
if (!count)
    return;

*desc = 0;

// First try to see if FormatMessage knows this hr
UINT icount = static_cast<UINT>(std::min<size_t>(count, 32767));

DWORD result = DX_FORMATMESSAGE(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), desc, icount, nullptr);

if (result > 0)
    return;

switch (hr)
{
// Commmented out codes are actually alises for other codes

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)

    // -------------------------------------------------------------
    // ddraw.h error codes
    // -------------------------------------------------------------
    CHK_ERR(DDERR_ALREADYINITIALIZED, "This object is already initialized")
    CHK_ERR(DDERR_CANNOTATTACHSURFACE, "This surface can not be attached to the requested surface.")
    CHK_ERR(DDERR_CANNOTDETACHSURFACE, "This surface can not be detached from the requested surface.")
    CHK_ERR(DDERR_CURRENTLYNOTAVAIL, "Support is currently not available.")
    CHK_ERR(DDERR_EXCEPTION, "An exception was encountered while performing the requested operation")
    //CHK_ERR(DDERR_GENERIC, "DDERR_GENERIC")
    CHK_ERR(DDERR_HEIGHTALIGN, "Height of rectangle provided is not a multiple of reqd alignment")
    CHK_ERR(DDERR_INCOMPATIBLEPRIMARY, "Unable to match primary surface creation request with existing primary surface.")
    CHK_ERR(DDERR_INVALIDCAPS, "One or more of the caps bits passed to the callback are incorrect.")
    CHK_ERR(DDERR_INVALIDCLIPLIST, "DirectDraw does not support provided Cliplist.")
    CHK_ERR(DDERR_INVALIDMODE, "DirectDraw does not support the requested mode")
    CHK_ERR(DDERR_INVALIDOBJECT, "DirectDraw received a pointer that was an invalid DIRECTDRAW object.")
    //CHK_ERR(DDERR_INVALIDPARAMS, "DDERR_INVALIDPARAMS")
    CHK_ERR(DDERR_INVALIDPIXELFORMAT, "pixel format was invalid as specified")
    CHK_ERR(DDERR_INVALIDRECT, "Rectangle provided was invalid.")
    CHK_ERR(DDERR_LOCKEDSURFACES, "Operation could not be carried out because one or more surfaces are locked")
    CHK_ERR(DDERR_NO3D, "There is no 3D present.")
    CHK_ERR(DDERR_NOALPHAHW, "Operation could not be carried out because there is no alpha accleration hardware present or available.")
    CHK_ERR(DDERR_NOSTEREOHARDWARE, "Operation could not be carried out because there is no stereo hardware present or available.")
    CHK_ERR(DDERR_NOSURFACELEFT, "Operation could not be carried out because there is no hardware present which supports stereo surfaces")
    CHK_ERR(DDERR_NOCLIPLIST, "no clip list available")
    CHK_ERR(DDERR_NOCOLORCONVHW, "Operation could not be carried out because there is no color conversion hardware present or available.")
    CHK_ERR(DDERR_NOCOOPERATIVELEVELSET, "Create function called without DirectDraw object method SetCooperativeLevel being called.")
    CHK_ERR(DDERR_NOCOLORKEY, "Surface doesn't currently have a color key")
    CHK_ERR(DDERR_NOCOLORKEYHW, "Operation could not be carried out because there is no hardware support of the dest color key.")
    CHK_ERR(DDERR_NODIRECTDRAWSUPPORT, "No DirectDraw support possible with current display driver")
    CHK_ERR(DDERR_NOEXCLUSIVEMODE, "Operation requires the application to have exclusive mode but the application does not have exclusive mode.")
    CHK_ERR(DDERR_NOFLIPHW, "Flipping visible surfaces is not supported.")
    CHK_ERR(DDERR_NOGDI, "There is no GDI present.")
    CHK_ERR(DDERR_NOMIRRORHW, "Operation could not be carried out because there is no hardware present or available.")
    CHK_ERR(DDERR_NOTFOUND, "Requested item was not found")
    CHK_ERR(DDERR_NOOVERLAYHW, "Operation could not be carried out because there is no overlay hardware present or available.")
    CHK_ERR(DDERR_OVERLAPPINGRECTS, "Operation could not be carried out because the source and destination rectangles are on the same surface and overlap each other.")
    CHK_ERR(DDERR_NORASTEROPHW, "Operation could not be carried out because there is no appropriate raster op hardware present or available.")
    CHK_ERR(DDERR_NOROTATIONHW, "Operation could not be carried out because there is no rotation hardware present or available.")
    CHK_ERR(DDERR_NOSTRETCHHW, "Operation could not be carried out because there is no hardware support for stretching")
    CHK_ERR(DDERR_NOT4BITCOLOR, "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.")
    CHK_ERR(DDERR_NOT4BITCOLORINDEX, "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.")
    CHK_ERR(DDERR_NOT8BITCOLOR, "DirectDraw Surface is not in 8 bit color mode and the requested operation requires 8 bit color.")
    CHK_ERR(DDERR_NOTEXTUREHW, "Operation could not be carried out because there is no texture mapping hardware present or available.")
    CHK_ERR(DDERR_NOVSYNCHW, "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.")
    CHK_ERR(DDERR_NOZBUFFERHW, "Operation could not be carried out because there is no hardware support for zbuffer blting.")
    CHK_ERR(DDERR_NOZOVERLAYHW, "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.")
    CHK_ERR(DDERR_OUTOFCAPS, "The hardware needed for the requested operation has already been allocated.")
    //CHK_ERR(DDERR_OUTOFMEMORY, "DDERR_OUTOFMEMORY")
    //CHK_ERR(DDERR_OUTOFVIDEOMEMORY, "DDERR_OUTOFVIDEOMEMORY")
    CHK_ERR(DDERR_OVERLAYCANTCLIP, "hardware does not support clipped overlays")
    CHK_ERR(DDERR_OVERLAYCOLORKEYONLYONEACTIVE, "Can only have ony color key active at one time for overlays")
    CHK_ERR(DDERR_PALETTEBUSY, "Access to this palette is being refused because the palette is already locked by another thread.")
    CHK_ERR(DDERR_COLORKEYNOTSET, "No src color key specified for this operation.")
    CHK_ERR(DDERR_SURFACEALREADYATTACHED, "This surface is already attached to the surface it is being attached to.")
    CHK_ERR(DDERR_SURFACEALREADYDEPENDENT, "This surface is already a dependency of the surface it is being made a dependency of.")
    CHK_ERR(DDERR_SURFACEBUSY, "Access to this surface is being refused because the surface is already locked by another thread.")
    CHK_ERR(DDERR_CANTLOCKSURFACE, "Access to this surface is being refused because no driver exists which can supply a pointer to the surface. This is most likely to happen when attempting to lock the primary surface when no DCI provider is present. Will also happen on attempts to lock an optimized surface.")
    CHK_ERR(DDERR_SURFACEISOBSCURED, "Access to Surface refused because Surface is obscured.")
    CHK_ERR(DDERR_SURFACELOST, "Access to this surface is being refused because the surface is gone. The DIRECTDRAWSURFACE object representing this surface should have Restore called on it.")
    CHK_ERR(DDERR_SURFACENOTATTACHED, "The requested surface is not attached.")
    CHK_ERR(DDERR_TOOBIGHEIGHT, "Height requested by DirectDraw is too large.")
    CHK_ERR(DDERR_TOOBIGSIZE, "Size requested by DirectDraw is too large --  The individual height and width are OK.")
    CHK_ERR(DDERR_TOOBIGWIDTH, "Width requested by DirectDraw is too large.")
    //CHK_ERR(DDERR_UNSUPPORTED, "DDERR_UNSUPPORTED")
    CHK_ERR(DDERR_UNSUPPORTEDFORMAT, "Pixel format requested is unsupported by DirectDraw")
    CHK_ERR(DDERR_UNSUPPORTEDMASK, "Bitmask in the pixel format requested is unsupported by DirectDraw")
    CHK_ERR(DDERR_INVALIDSTREAM, "The specified stream contains invalid data")
    CHK_ERR(DDERR_VERTICALBLANKINPROGRESS, "vertical blank is in progress")
    CHK_ERR(DDERR_WASSTILLDRAWING, "Was still drawing")
    CHK_ERR(DDERR_DDSCAPSCOMPLEXREQUIRED, "The specified surface type requires specification of the COMPLEX flag")
    CHK_ERR(DDERR_XALIGN, "Rectangle provided was not horizontally aligned on reqd. boundary")
    CHK_ERR(DDERR_INVALIDDIRECTDRAWGUID, "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.")
    CHK_ERR(DDERR_DIRECTDRAWALREADYCREATED, "A DirectDraw object representing this driver has already been created for this process.")
    CHK_ERR(DDERR_NODIRECTDRAWHW, "A hardware only DirectDraw object creation was attempted but the driver did not support any hardware.")
    CHK_ERR(DDERR_PRIMARYSURFACEALREADYEXISTS, "this process already has created a primary surface")
    CHK_ERR(DDERR_NOEMULATION, "software emulation not available.")
    CHK_ERR(DDERR_REGIONTOOSMALL, "region passed to Clipper::GetClipList is too small.")
    CHK_ERR(DDERR_CLIPPERISUSINGHWND, "an attempt was made to set a clip list for a clipper objec that is already monitoring an hwnd.")
    CHK_ERR(DDERR_NOCLIPPERATTACHED, "No clipper object attached to surface object")
    CHK_ERR(DDERR_NOHWND, "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.")
    CHK_ERR(DDERR_HWNDSUBCLASSED, "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.")
    CHK_ERR(DDERR_HWNDALREADYSET, "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.")
    CHK_ERR(DDERR_NOPALETTEATTACHED, "No palette object attached to this surface.")
    CHK_ERR(DDERR_NOPALETTEHW, "No hardware support for 16 or 256 color palettes.")
    CHK_ERR(DDERR_BLTFASTCANTCLIP, "If a clipper object is attached to the source surface passed into a BltFast call.")
    CHK_ERR(DDERR_NOBLTHW, "No blter.")
    CHK_ERR(DDERR_NODDROPSHW, "No DirectDraw ROP hardware.")
    CHK_ERR(DDERR_OVERLAYNOTVISIBLE, "returned when GetOverlayPosition is called on a hidden overlay")
    CHK_ERR(DDERR_NOOVERLAYDEST, "returned when GetOverlayPosition is called on a overlay that UpdateOverlay has never been called on to establish a destionation.")
    CHK_ERR(DDERR_INVALIDPOSITION, "returned when the position of the overlay on the destionation is no longer legal for that destionation.")
    CHK_ERR(DDERR_NOTAOVERLAYSURFACE, "returned when an overlay member is called for a non-overlay surface")
    CHK_ERR(DDERR_EXCLUSIVEMODEALREADYSET, "An attempt was made to set the cooperative level when it was already set to exclusive.")
    CHK_ERR(DDERR_NOTFLIPPABLE, "An attempt has been made to flip a surface that is not flippable.")
    CHK_ERR(DDERR_CANTDUPLICATE, "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.")
    CHK_ERR(DDERR_NOTLOCKED, "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.")
    CHK_ERR(DDERR_CANTCREATEDC, "Windows can not create any more DCs, or a DC was requested for a paltte-indexed surface when the surface had no palette AND the display mode was not palette-indexed (in this case DirectDraw cannot select a proper palette into the DC)")
    CHK_ERR(DDERR_NODC, "No DC was ever created for this surface.")
    CHK_ERR(DDERR_WRONGMODE, "This surface can not be restored because it was created in a different mode.")
    CHK_ERR(DDERR_IMPLICITLYCREATED, "This surface can not be restored because it is an implicitly created surface.")
    CHK_ERR(DDERR_NOTPALETTIZED, "The surface being used is not a palette-based surface")
    CHK_ERR(DDERR_UNSUPPORTEDMODE, "The display is currently in an unsupported mode")
    CHK_ERR(DDERR_NOMIPMAPHW, "Operation could not be carried out because there is no mip-map texture mapping hardware present or available.")
    CHK_ERR(DDERR_INVALIDSURFACETYPE, "The requested action could not be performed because the surface was of the wrong type.")
    CHK_ERR(DDERR_NOOPTIMIZEHW, "Device does not support optimized surfaces, therefore no video memory optimized surfaces")
    CHK_ERR(DDERR_NOTLOADED, "Surface is an optimized surface, but has not yet been allocated any memory")
    CHK_ERR(DDERR_NOFOCUSWINDOW, "Attempt was made to create or set a device window without first setting the focus window")
    CHK_ERR(DDERR_NOTONMIPMAPSUBLEVEL, "Attempt was made to set a palette on a mipmap sublevel")
    CHK_ERR(DDERR_DCALREADYCREATED, "A DC has already been returned for this surface. Only one DC can be retrieved per surface.")
    CHK_ERR(DDERR_NONONLOCALVIDMEM, "An attempt was made to allocate non-local video memory from a device that does not support non-local video memory.")
    CHK_ERR(DDERR_CANTPAGELOCK, "The attempt to page lock a surface failed.")
    CHK_ERR(DDERR_CANTPAGEUNLOCK, "The attempt to page unlock a surface failed.")
    CHK_ERR(DDERR_NOTPAGELOCKED, "An attempt was made to page unlock a surface with no outstanding page locks.")
    CHK_ERR(DDERR_MOREDATA, "There is more data available than the specified buffer size could hold")
    CHK_ERR(DDERR_EXPIRED, "The data has expired and is therefore no longer valid.")
    CHK_ERR(DDERR_TESTFINISHED, "The mode test has finished executing.")
    CHK_ERR(DDERR_NEWMODE, "The mode test has switched to a new mode.")
    CHK_ERR(DDERR_D3DNOTINITIALIZED, "D3D has not yet been initialized.")
    CHK_ERR(DDERR_VIDEONOTACTIVE, "The video port is not active")
    CHK_ERR(DDERR_NOMONITORINFORMATION, "The monitor does not have EDID data.")
    CHK_ERR(DDERR_NODRIVERSUPPORT, "The driver does not enumerate display mode refresh rates.")
    CHK_ERR(DDERR_DEVICEDOESNTOWNSURFACE, "Surfaces created by one direct draw device cannot be used directly by another direct draw device.")

    // -------------------------------------------------------------
    // d3d9.h error codes
    // -------------------------------------------------------------
    //CHK_ERR(D3D_OK, "Ok")
    CHK_ERR(D3DERR_WRONGTEXTUREFORMAT, "Wrong texture format")
    CHK_ERR(D3DERR_UNSUPPORTEDCOLOROPERATION, "Unsupported color operation")
    CHK_ERR(D3DERR_UNSUPPORTEDCOLORARG, "Unsupported color arg")
    CHK_ERR(D3DERR_UNSUPPORTEDALPHAOPERATION, "Unsupported alpha operation")
    CHK_ERR(D3DERR_UNSUPPORTEDALPHAARG, "Unsupported alpha arg")
    CHK_ERR(D3DERR_TOOMANYOPERATIONS, "Too many operations")
    CHK_ERR(D3DERR_CONFLICTINGTEXTUREFILTER, "Conflicting texture filter")
    CHK_ERR(D3DERR_UNSUPPORTEDFACTORVALUE, "Unsupported factor value")
    CHK_ERR(D3DERR_CONFLICTINGRENDERSTATE, "Conflicting render state")
    CHK_ERR(D3DERR_UNSUPPORTEDTEXTUREFILTER, "Unsupported texture filter")
    CHK_ERR(D3DERR_CONFLICTINGTEXTUREPALETTE, "Conflicting texture palette")
    CHK_ERR(D3DERR_DRIVERINTERNALERROR, "Driver internal error")
    CHK_ERR(D3DERR_NOTFOUND, "Not found")
    CHK_ERR(D3DERR_MOREDATA, "More data")
    CHK_ERR(D3DERR_DEVICELOST, "Device lost")
    CHK_ERR(D3DERR_DEVICENOTRESET, "Device not reset")
    CHK_ERR(D3DERR_NOTAVAILABLE, "Not available")
    CHK_ERR(D3DERR_OUTOFVIDEOMEMORY, "Out of video memory")
    CHK_ERR(D3DERR_INVALIDDEVICE, "Invalid device")
    CHK_ERR(D3DERR_INVALIDCALL, "Invalid call")
    CHK_ERR(D3DERR_DRIVERINVALIDCALL, "Driver invalid call")
    //CHK_ERR(D3DERR_WASSTILLDRAWING, "Was Still Drawing")
    CHK_ERR(D3DOK_NOAUTOGEN, "The call succeeded but there won't be any mipmaps generated")

    // Extended for Windows Vista
    CHK_ERR(D3DERR_DEVICEREMOVED, "Hardware device was removed")
    CHK_ERR(S_NOT_RESIDENT, "Resource not resident in memory")
    CHK_ERR(S_RESIDENT_IN_SHARED_MEMORY, "Resource resident in shared memory")
    CHK_ERR(S_PRESENT_MODE_CHANGED, "Desktop display mode has changed")
    CHK_ERR(S_PRESENT_OCCLUDED, "Client window is occluded (minimized or other fullscreen)")
    CHK_ERR(D3DERR_DEVICEHUNG, "Hardware adapter reset by OS")

    // Extended for Windows 7
    CHK_ERR(D3DERR_UNSUPPORTEDOVERLAY, "Overlay is not supported" )
    CHK_ERR(D3DERR_UNSUPPORTEDOVERLAYFORMAT, "Overlay format is not supported" )
    CHK_ERR(D3DERR_CANNOTPROTECTCONTENT, "Contect protection not available" )
    CHK_ERR(D3DERR_UNSUPPORTEDCRYPTO, "Unsupported cryptographic system" )
    CHK_ERR(D3DERR_PRESENT_STATISTICS_DISJOINT, "Presentation statistics are disjoint" )

    // -------------------------------------------------------------
    // dsound.h error codes
    // -------------------------------------------------------------
    //CHK_ERR(DS_OK, "")
    CHK_ERR(DS_NO_VIRTUALIZATION, "The call succeeded, but we had to substitute the 3D algorithm")
    CHK_ERR(DSERR_ALLOCATED, "The call failed because resources (such as a priority level) were already being used by another caller")
    CHK_ERR(DSERR_CONTROLUNAVAIL, "The control (vol, pan, etc.) requested by the caller is not available")
    //CHK_ERR(DSERR_INVALIDPARAM, "DSERR_INVALIDPARAM")
    CHK_ERR(DSERR_INVALIDCALL, "This call is not valid for the current state of this object")
    //CHK_ERR(DSERR_GENERIC, "DSERR_GENERIC")
    CHK_ERR(DSERR_PRIOLEVELNEEDED, "The caller does not have the priority level required for the function to succeed")
    //CHK_ERR(DSERR_OUTOFMEMORY, "Not enough free memory is available to complete the operation")
    CHK_ERR(DSERR_BADFORMAT, "The specified WAVE format is not supported")
    //CHK_ERR(DSERR_UNSUPPORTED, "DSERR_UNSUPPORTED")
    CHK_ERR(DSERR_NODRIVER, "No sound driver is available for use")
    CHK_ERR(DSERR_ALREADYINITIALIZED, "This object is already initialized")
    //CHK_ERR(DSERR_NOAGGREGATION, "DSERR_NOAGGREGATION")
    CHK_ERR(DSERR_BUFFERLOST, "The buffer memory has been lost, and must be restored")
    CHK_ERR(DSERR_OTHERAPPHASPRIO, "Another app has a higher priority level, preventing this call from succeeding")
    CHK_ERR(DSERR_UNINITIALIZED, "This object has not been initialized")
    //CHK_ERR(DSERR_NOINTERFACE, "DSERR_NOINTERFACE")
    //CHK_ERR(DSERR_ACCESSDENIED, "DSERR_ACCESSDENIED")
    CHK_ERR(DSERR_BUFFERTOOSMALL, "Tried to create a DSBCAPS_CTRLFX buffer shorter than DSBSIZE_FX_MIN milliseconds")
    CHK_ERR(DSERR_DS8_REQUIRED, "Attempt to use DirectSound 8 functionality on an older DirectSound object")
    CHK_ERR(DSERR_SENDLOOP, "A circular loop of send effects was detected")
    CHK_ERR(DSERR_BADSENDBUFFERGUID, "The GUID specified in an audiopath file does not match a valid MIXIN buffer")
    CHK_ERR(DSERR_OBJECTNOTFOUND, "The object requested was not found (numerically equal to DMUS_E_NOT_FOUND)")

    CHK_ERR(DSERR_FXUNAVAILABLE, "Requested effects are not available")

#endif // !WINAPI_FAMILY || WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP

    // -------------------------------------------------------------
    // d3d10.h error codes
    // -------------------------------------------------------------
    CHK_ERR(D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS, "There are too many unique state objects.")
    CHK_ERR(D3D10_ERROR_FILE_NOT_FOUND, "File not found")

    // -------------------------------------------------------------
    // dxgi.h error codes
    // -------------------------------------------------------------
    CHK_ERR(DXGI_STATUS_OCCLUDED, "The target window or output has been occluded. The application should suspend rendering operations if possible.")
    CHK_ERR(DXGI_STATUS_CLIPPED, "Target window is clipped.")
    CHK_ERR(DXGI_STATUS_NO_REDIRECTION, "")
    CHK_ERR(DXGI_STATUS_NO_DESKTOP_ACCESS, "No access to desktop.")
    CHK_ERR(DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, "")
    CHK_ERR(DXGI_STATUS_MODE_CHANGED, "Display mode has changed")
    CHK_ERR(DXGI_STATUS_MODE_CHANGE_IN_PROGRESS, "Display mode is changing")
    CHK_ERR(DXGI_ERROR_INVALID_CALL, "The application has made an erroneous API call that it had enough information to avoid. This error is intended to denote that the application should be altered to avoid the error. Use of the debug version of the DXGI.DLL will provide run-time debug output with further information.")
    CHK_ERR(DXGI_ERROR_NOT_FOUND, "The item requested was not found. For GetPrivateData calls, this means that the specified GUID had not been previously associated with the object.")
    CHK_ERR(DXGI_ERROR_MORE_DATA, "The specified size of the destination buffer is too small to hold the requested data.")
    CHK_ERR(DXGI_ERROR_UNSUPPORTED, "Unsupported.")
    CHK_ERR(DXGI_ERROR_DEVICE_REMOVED, "Hardware device removed.")
    CHK_ERR(DXGI_ERROR_DEVICE_HUNG, "Device hung due to badly formed commands.")
    CHK_ERR(DXGI_ERROR_DEVICE_RESET, "Device reset due to a badly formed commant.")
    CHK_ERR(DXGI_ERROR_WAS_STILL_DRAWING, "Was still drawing.")
    CHK_ERR(DXGI_ERROR_FRAME_STATISTICS_DISJOINT, "The requested functionality is not supported by the device or the driver.")
    CHK_ERR(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE, "The requested functionality is not supported by the device or the driver.")
    CHK_ERR(DXGI_ERROR_DRIVER_INTERNAL_ERROR, "An internal driver error occurred.")
    CHK_ERR(DXGI_ERROR_NONEXCLUSIVE, "The application attempted to perform an operation on an DXGI output that is only legal after the output has been claimed for exclusive owenership.")
    CHK_ERR(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "The requested functionality is not supported by the device or the driver.")
    CHK_ERR(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED, "Remote desktop client disconnected.")
    CHK_ERR(DXGI_ERROR_REMOTE_OUTOFMEMORY, "Remote desktop client is out of memory.")

    // -------------------------------------------------------------
    // d3d11.h error codes
    // -------------------------------------------------------------
    CHK_ERR(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS, "There are too many unique state objects.")
    CHK_ERR(D3D11_ERROR_FILE_NOT_FOUND, "File not found")
    CHK_ERR(D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS, "Therea are too many unique view objects.")
    CHK_ERR(D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD, "Deferred context requires Map-Discard usage pattern")

#if !defined(WINAPI_FAMILY) || WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP

    // -------------------------------------------------------------
    // Direct2D error codes
    // -------------------------------------------------------------
    //CHK_ERR(D2DERR_UNSUPPORTED_PIXEL_FORMAT, "The pixel format is not supported.")
    //CHK_ERR(D2DERR_INSUFFICIENT_BUFFER, "The supplied buffer was too small to accomodate the data.")
    CHK_ERR(D2DERR_WRONG_STATE, "The object was not in the correct state to process the method.")
    CHK_ERR(D2DERR_NOT_INITIALIZED, "The object has not yet been initialized.")
    CHK_ERR(D2DERR_UNSUPPORTED_OPERATION, "The requested opertion is not supported.")
    CHK_ERR(D2DERR_SCANNER_FAILED, "The geomery scanner failed to process the data.")
    CHK_ERR(D2DERR_SCREEN_ACCESS_DENIED, "D2D could not access the screen.")
    CHK_ERR(D2DERR_DISPLAY_STATE_INVALID, "A valid display state could not be determined.")
    CHK_ERR(D2DERR_ZERO_VECTOR, "The supplied vector is zero.")
    CHK_ERR(D2DERR_INTERNAL_ERROR, "An internal error (D2D bug) occurred. On checked builds, we would assert.")
    CHK_ERR(D2DERR_DISPLAY_FORMAT_NOT_SUPPORTED, "The display format we need to render is not supported by the hardware device.")
    CHK_ERR(D2DERR_INVALID_CALL, "A call to this method is invalid.")
    CHK_ERR(D2DERR_NO_HARDWARE_DEVICE, "No HW rendering device is available for this operation.")
    CHK_ERR(D2DERR_RECREATE_TARGET, "here has been a presentation error that may be recoverable. The caller needs to recreate, rerender the entire frame, and reattempt present.")
    CHK_ERR(D2DERR_TOO_MANY_SHADER_ELEMENTS, "Shader construction failed because it was too complex.")
    CHK_ERR(D2DERR_SHADER_COMPILE_FAILED, "Shader compilation failed.")
    CHK_ERR(D2DERR_MAX_TEXTURE_SIZE_EXCEEDED, "Requested DX surface size exceeded maximum texture size.")
    CHK_ERR(D2DERR_UNSUPPORTED_VERSION, "The requested D2D version is not supported.")
    CHK_ERR(D2DERR_BAD_NUMBER, "Invalid number.")
    CHK_ERR(D2DERR_WRONG_FACTORY, "Objects used together must be created from the same factory instance.")
    CHK_ERR(D2DERR_LAYER_ALREADY_IN_USE, "A layer resource can only be in use once at any point in time.")
    CHK_ERR(D2DERR_POP_CALL_DID_NOT_MATCH_PUSH, "The pop call did not match the corresponding push call")
    //CHK_ERR(D2DERR_WRONG_RESOURCE_DOMAIN, "The resource was realized on the wrong render target")
    CHK_ERR(D2DERR_PUSH_POP_UNBALANCED, "The push and pop calls were unbalanced")
    CHK_ERR(D2DERR_RENDER_TARGET_HAS_LAYER_OR_CLIPRECT, "Attempt to copy from a render target while a layer or clip rect is applied")
    CHK_ERR(D2DERR_INCOMPATIBLE_BRUSH_TYPES, "The brush types are incompatible for the call.")
    CHK_ERR(D2DERR_WIN32_ERROR, "An unknown win32 failure occurred.")
    CHK_ERR(D2DERR_TARGET_NOT_GDI_COMPATIBLE, "The render target is not compatible with GDI")
    CHK_ERR(D2DERR_TEXT_EFFECT_IS_WRONG_TYPE, "A text client drawing effect object is of the wrong type")
    CHK_ERR(D2DERR_TEXT_RENDERER_NOT_RELEASED, "The application is holding a reference to the IDWriteTextRenderer interface after the corresponding DrawText or DrawTextLayout call has returned. The IDWriteTextRenderer instance will be zombied.")
    //CHK_ERR(D2DERR_EXCEEDS_MAX_BITMAP_SIZE, "The requested size is larger than the guaranteed supported texture size.")

    // -------------------------------------------------------------
    // DirectWrite error codes
    // -------------------------------------------------------------
    CHK_ERR(DWRITE_E_FILEFORMAT, "Indicates an error in an input file such as a font file.")
    CHK_ERR(DWRITE_E_UNEXPECTED, "Indicates an error originating in DirectWrite code, which is not expected to occur but is safe to recover from.")
    CHK_ERR(DWRITE_E_NOFONT, "Indicates the specified font does not exist.")
    CHK_ERR(DWRITE_E_FILENOTFOUND, "A font file could not be opened because the file, directory, network location, drive, or other storage location does not exist or is unavailable.")
    CHK_ERR(DWRITE_E_FILEACCESS, "A font file exists but could not be opened due to access denied, sharing violation, or similar error.")
    CHK_ERR(DWRITE_E_FONTCOLLECTIONOBSOLETE, "A font collection is obsolete due to changes in the system.")
    CHK_ERR(DWRITE_E_ALREADYREGISTERED, "The given interface is already registered.")

    // -------------------------------------------------------------
    // WIC error codes
    // -------------------------------------------------------------
    CHK_ERR(WINCODEC_ERR_WRONGSTATE, "WIC object in incorrect state.")
    CHK_ERR(WINCODEC_ERR_VALUEOUTOFRANGE, "WIC Value out of range.")
    CHK_ERR(WINCODEC_ERR_UNKNOWNIMAGEFORMAT, "Encountered unexpected value or setting in WIC image format.")
    CHK_ERR(WINCODEC_ERR_UNSUPPORTEDVERSION, "Unsupported WINCODEC_SD_VERSION passed to WIC factory.")
    CHK_ERR(WINCODEC_ERR_NOTINITIALIZED, "WIC component not initialized.")
    CHK_ERR(WINCODEC_ERR_ALREADYLOCKED, "WIC bitmap object already locked.")
    CHK_ERR(WINCODEC_ERR_PROPERTYNOTFOUND, "WIC property not found.")
    CHK_ERR(WINCODEC_ERR_PROPERTYNOTSUPPORTED, "WIC property not supported.")
    CHK_ERR(WINCODEC_ERR_PROPERTYSIZE, "Invalid property size")
    CHK_ERRA(WINCODEC_ERR_CODECPRESENT) // not currently used by WIC
    CHK_ERRA(WINCODEC_ERR_CODECNOTHUMBNAIL) // not currently used by WIC
    CHK_ERR(WINCODEC_ERR_PALETTEUNAVAILABLE, "Required palette data not available.")
    CHK_ERR(WINCODEC_ERR_CODECTOOMANYSCANLINES, "More scanlines requested than are available in WIC bitmap.")
    CHK_ERR(WINCODEC_ERR_INTERNALERROR, "Unexpected internal error in WIC.")
    CHK_ERR(WINCODEC_ERR_SOURCERECTDOESNOTMATCHDIMENSIONS, "Source WIC rectangle does not match bitmap dimensions.")
    CHK_ERR(WINCODEC_ERR_COMPONENTNOTFOUND, "WIC component not found.")
    CHK_ERR(WINCODEC_ERR_IMAGESIZEOUTOFRANGE, "Image size beyond expected boundaries for WIC codec." )
    CHK_ERR(WINCODEC_ERR_TOOMUCHMETADATA, "Image metadata size beyond expected boundaries for WIC codec.")
    CHK_ERR(WINCODEC_ERR_BADIMAGE, "WIC image is corrupted.")
    CHK_ERR(WINCODEC_ERR_BADHEADER, "Invalid header found in WIC image.")
    CHK_ERR(WINCODEC_ERR_FRAMEMISSING, "Expected bitmap frame data not found in WIC image." )
    CHK_ERR(WINCODEC_ERR_BADMETADATAHEADER, "Invalid metadata header found in WIC image.")
    CHK_ERR(WINCODEC_ERR_BADSTREAMDATA, "Invalid stream data found in WIC image.")
    CHK_ERR(WINCODEC_ERR_STREAMWRITE, "WIC operation on write stream failed.")
    CHK_ERR(WINCODEC_ERR_STREAMREAD, "WIC operation on read stream failed.")
    CHK_ERR(WINCODEC_ERR_STREAMNOTAVAILABLE, "Required stream is not available." )
    CHK_ERR(WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT, "The pixel format is not supported.")
    CHK_ERR(WINCODEC_ERR_UNSUPPORTEDOPERATION, "This operation is not supported by WIC." )
    CHK_ERR(WINCODEC_ERR_INVALIDREGISTRATION, "Error occurred reading WIC codec registry keys.")
    CHK_ERR(WINCODEC_ERR_COMPONENTINITIALIZEFAILURE, "Failed initializing WIC codec.")
    CHK_ERR(WINCODEC_ERR_INSUFFICIENTBUFFER, "Not enough buffer space available for WIC operation.")
    CHK_ERR(WINCODEC_ERR_DUPLICATEMETADATAPRESENT, "Duplicate metadata detected in WIC image.")
    CHK_ERR(WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE, "Unexpected property type in WIC image.")
    CHK_ERR(WINCODEC_ERR_UNEXPECTEDSIZE, "Unexpected value size in WIC metadata.")
    CHK_ERR(WINCODEC_ERR_INVALIDQUERYREQUEST, "Invalid metadata query.")
    CHK_ERR(WINCODEC_ERR_UNEXPECTEDMETADATATYPE, "Unexpected metadata type encountered in WIC image.")
    CHK_ERR(WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT, "Operation only valid on meatadata root.")
    CHK_ERR(WINCODEC_ERR_INVALIDQUERYCHARACTER, "Invalid character in WIC metadata query.")
    CHK_ERR(WINCODEC_ERR_WIN32ERROR, "General Win32 error encountered during WIC operation.")
    CHK_ERR(WINCODEC_ERR_INVALIDPROGRESSIVELEVEL, "Invalid level for progressive WIC image decode.")

#endif // !WINAPI_FAMILY || WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP

    // -------------------------------------------------------------
    // DXUT error codes
    // -------------------------------------------------------------
    CHK_ERR(DXUTERR_NODIRECT3D, "Could not initialize Direct3D.")
    CHK_ERR(DXUTERR_NOCOMPATIBLEDEVICES, "No device could be found with the specified device settings.")
    CHK_ERR(DXUTERR_MEDIANOTFOUND, "A media file could not be found.")
    CHK_ERR(DXUTERR_NONZEROREFCOUNT, "The device interface has a non-zero reference count, meaning that some objects were not released.")
    CHK_ERR(DXUTERR_CREATINGDEVICE, "An error occurred when attempting to create a device.")
    CHK_ERR(DXUTERR_RESETTINGDEVICE, "An error occurred when attempting to reset a device.")
    CHK_ERR(DXUTERR_CREATINGDEVICEOBJECTS, "An error occurred in the device create callback function.")
    CHK_ERR(DXUTERR_RESETTINGDEVICEOBJECTS, "An error occurred in the device reset callback function.")
    CHK_ERR(DXUTERR_INCORRECTVERSION, "Incorrect version of Direct3D or D3DX.")
    CHK_ERR(DXUTERR_DEVICEREMOVED, "The device was removed.")

    // -------------------------------------------------------------
    // xaudio2.h error codes
    // -------------------------------------------------------------
    CHK_ERR(XAUDIO2_E_INVALID_CALL, "Invalid XAudio2 API call or arguments")
    CHK_ERR(XAUDIO2_E_XMA_DECODER_ERROR, "Hardware XMA decoder error")
    CHK_ERR(XAUDIO2_E_XAPO_CREATION_FAILED, "Failed to create an audio effect")
    CHK_ERR(XAUDIO2_E_DEVICE_INVALIDATED, "Device invalidated (unplugged, disabled, etc)")

    // -------------------------------------------------------------
    // xapo.h error codes
    // -------------------------------------------------------------
    CHK_ERR(XAPO_E_FORMAT_UNSUPPORTED, "Requested audio format unsupported.")
}
#pragma warning(pop)
