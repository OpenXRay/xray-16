#ifndef maTranslatorH
#define maTranslatorH

#include "ExportDefines.H"
// refs
class CEditableObject;

class CXRayObjectExport : public MPxFileTranslator
{
public:
    CXRayObjectExport(){};
    virtual ~CXRayObjectExport(){};
    static void* creator();

    MStatus reader(const MFileObject& file, const MString& optionsString, FileAccessMode mode);

    MStatus writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode);
    bool haveReadMethod() const;
    bool haveWriteMethod() const;
    // virtual MString         defaultExtension	() const;
    virtual MString filter() const;
    MFileKind identifyFile(const MFileObject& fileName, const char* buffer, short size) const;

private:
    LPCSTR getMaterialName(MDagPath& mdagPath, int cid, int objectIdx);
    MStatus ExportPart(CEditableObject* O, MDagPath&, MObject&);
    MStatus set_smoth_flags(
        u32& flags, const MIntArray& tri_vert_indeces); // const MFnMesh &fnMesh, const MItMeshPolygon &meshPoly,
    MStatus ExportSelected(CEditableObject* O);
    MStatus ExportAll(CEditableObject* O);

    bool initializeSetsAndLookupTables(bool exportAll);
    void freeLookupTables();
    bool lookup(MDagPath&, int, int);
    void recFindTransformDAGNodes(MString&, MIntArray&);

    // Edge lookup methods
    //
    void buildEdgeTable(MDagPath&);
    void addEdgeInfo(int, int, bool);

public:
    SXREdgeInfoPtr findEdgeInfo(int, int);

private:
    void destroyEdgeTable();
    // smoothing groups
    bool smoothingAlgorithm(int, MFnMesh&);
    void CreateSmoothingGroups(MFnMesh&);
    void CreateSMGFacegroups(MFnMesh&);
    void CreateSMGEdgeAttrs(MFnMesh&);

private:
    // Keeps track of all sets.
    //
    int numSets;
    MObjectArray* sets;

    // Keeps track of all objects and components.
    // The Tables are used to mark which sets each
    // component belongs to.
    //
    MStringArray* objectNames;

    bool** polygonTablePtr;
    bool** vertexTablePtr;
    bool* polygonTable;
    bool* vertexTable;
    bool** objectGroupsTablePtr;

    // Used to determine if the last set(s) written out are the same
    // as the current sets to be written. We don't need to write out
    // sets unless they change between components. Same goes for
    // materials.
    //
    MIntArray* lastSets;
    MIntArray* lastMaterials;

    // We have to do 2 dag iterations so keep track of the
    // objects found in the first iteration by this index.
    //
    int objectId;
    int objectCount;

    // Edge lookup table (by vertex id) and smoothing group info
    //
    SXREdgeInfoPtr* edgeTable;
    int* polySmoothingGroups;
    int edgeTableSize;
    int nextSmoothingGroup;
    int currSmoothingGroup;
    bool newSmoothingGroup;

    // List of names of the mesh shapes that we export from maya
    MStringArray objectNodeNamesArray;

    // Used to keep track of Maya groups (transform DAG nodes) that
    // contain objects being exported
    MStringArray transformNodeNameArray;
};

#endif // maTranslatorH
