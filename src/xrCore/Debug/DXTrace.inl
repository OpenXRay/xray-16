DX_CHAR strBufferLine[128];
DX_CHAR strBufferError[256];
DX_CHAR strBuffer[BUFFER_SIZE];

DX_SPRINTF_S( strBufferLine, 128, DX_STR_WRAP("%lu"), dwLine );
if( strFile )
{
    DX_SPRINTF_S( strBuffer, BUFFER_SIZE, DX_STR_WRAP(STR_FMT_SPEC "(" STR_FMT_SPEC "): "), strFile, strBufferLine );
    DX_OUTPUTDEBUGSTRING( strBuffer );
}

size_t nMsgLen = (strMsg) ? DX_STRNLEN_S( strMsg, 1024 ) : 0;
if( nMsgLen > 0 )
{
    DX_OUTPUTDEBUGSTRING( strMsg );
    DX_OUTPUTDEBUGSTRING( DX_STR_WRAP(" ") );
}

DX_SPRINTF_S( strBufferError, 256, DX_STR_WRAP(STR_FMT_SPEC " (0x%0.8x)"), DX_GETERRORSTRING(hr), hr );
DX_SPRINTF_S( strBuffer, BUFFER_SIZE, DX_STR_WRAP("hr=" STR_FMT_SPEC), strBufferError );
DX_OUTPUTDEBUGSTRING( strBuffer );

DX_OUTPUTDEBUGSTRING( DX_STR_WRAP("\n") );

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
if( bPopMsgBox )
{
    DX_CHAR strBufferFile[MAX_PATH];
    DX_STRCPY_S( strBufferFile, MAX_PATH, DX_STR_WRAP("") );
    if( strFile )
        DX_STRCPY_S( strBufferFile, MAX_PATH, strFile );

    DX_CHAR strBufferMsg[1024];
    DX_STRCPY_S( strBufferMsg, 1024, DX_STR_WRAP("") );
    if( nMsgLen > 0 )
        DX_SPRINTF_S( strBufferMsg, 1024, DX_STR_WRAP("Calling: " STR_FMT_SPEC "\n"), strMsg );

    DX_SPRINTF_S( strBuffer, BUFFER_SIZE, DX_STR_WRAP("File: " STR_FMT_SPEC "\nLine: " STR_FMT_SPEC "\nError Code: " STR_FMT_SPEC "\n" STR_FMT_SPEC "Do you want to debug the application?"),
                strBufferFile, strBufferLine, strBufferError, strBufferMsg );

    int nResult = DX_MESSAGEBOX( GetForegroundWindow(), strBuffer, DX_STR_WRAP("Unexpected error encountered"), MB_YESNO | MB_ICONERROR );
    if( nResult == IDYES )
        DebugBreak();
}
#else
UNREFERENCED_PARAMETER(bPopMsgBox);
#endif

return hr;
