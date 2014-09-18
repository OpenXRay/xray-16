#ifndef __NodeMonitor_h__
#define __NodeMonitor_h__

//
//
// DESCRIPTION:  This class monitors a given node.
//
// AUTHOR: Christian Laforte
//
//

#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MString.h>
#include <maya/MNodeMessage.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

MObject getObjFromName(MString name, MStatus& stat);

// If a problem occurs, this function returns an empty string.
MString getNameFromObj(MObject obj);

// Classes that implement the NodeMonitorManager interfaces
// can be called back when a node is renamed.
class NodeMonitorManager
{
public:
	virtual void onNodeRenamed(MObject& node, MString oldName, MString newName) = 0;
};

class NodeMonitor
{
public:
	NodeMonitor(NodeMonitorManager* manager = NULL);

	~NodeMonitor();

	bool watch(MString nodeName);
	bool watch(MObject nodeObj);

	void stopWatching();

	bool dirty();

	void cleanIt();

	void setManager(NodeMonitorManager* manager) { fManager = manager; }

private:
	bool attachCallbacks();

	void detachCallbacks();

	void callbackOccured();

	// Callback functions. Those are called, respectively, when a node is dirty (has changed substantially),
	// or when a node is renamed.
	static void watchedObjectDirtyCallback(void* clientData);

	static void watchedObjectRenamedCallback(MObject & node, void* clientData);


private:
	MString fNodeName;
	bool fIsDirty;

	MCallbackId fRenamedCallbackId;
	MCallbackId fDirtyCallbackId;

	NodeMonitorManager* fManager;
};


#endif // MAYA_ShadingConnection