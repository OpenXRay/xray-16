
#ifdef __cplusplus
extern "C" {
#endif

void MakeBase32Pretty(char* pcOut, const char* pcIn);
int CleanForBase32(char* newstr, const char *oldstr, int maxoutput);
int ConvertToBase32(char* pcOut, const char* pcIn, int nInBytes);
int ConvertFromBase32(char* pcOut, const char* pcIn, int nInBytes);
#ifdef __cplusplus
}
#endif
