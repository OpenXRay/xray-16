#ifndef CAMERA_EXPORTER_H_INCLUDED
#define CAMERA_EXPORTER_H_INCLUDED

class CXRayCameraExport : public MPxFileTranslator
{
public:
    CXRayCameraExport(){};
    virtual ~CXRayCameraExport(){};
    static void* creator();

    MStatus reader(const MFileObject& file, const MString& optionsString, FileAccessMode mode);
    MStatus writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode);

    bool haveReadMethod() const;
    bool haveWriteMethod() const;

    virtual MString filter() const;
    MFileKind identifyFile(const MFileObject& fileName, const char* buffer, short size) const;

private:
    MStatus ExportCamera(const MFileObject& file);
};

#endif // CAMERA_EXPORTER_H_INCLUDED
