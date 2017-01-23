///////////////////////////////////////////////////////////////////
// DESCRIPTION: This class monitors a given node. 
//
// TODO: Take care of renamed nodes.
//
// AUTHOR: Christian Laforte
//
///////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "NodeMonitor.h"

MObject getObjFromName(MString name, MStatus& stat)
{
	MObject obj;
	
	MSelectionList list;
	
	// Attempt to add the given name to the selection list,
	// then get the corresponding dependency node handle.
	if (!list.add(name) ||
		!list.getDependNode(0, obj))
	{
		// Failed.
		stat = MStatus::kInvalidParameter;
		return obj;
	}

	// Successful.
	stat = MStatus::kSuccess;
	return obj;
}

// If a problem occurs, this function returns an empty string.
MString getNameFromObj(MObject obj)
{
	MString nodeName;

	// If this object is a MFnDagNode, we should store the dag name.
	// Otherwise, use the MFnDependencyNode name.
	if (obj.hasFn(MFn::kDagNode))
	{
		MFnDagNode dagNode(obj);
		nodeName = dagNode.fullPathName();
	}
	else if (obj.hasFn(MFn::kDependencyNode))
	{
		MFnDependencyNode node(obj);
		nodeName = node.name();
	}

	return nodeName;
}


//-----------------------------------------------------------
// Public interface:
//-----------------------------------------------------------

NodeMonitor::NodeMonitor(NodeMonitorManager* manager /* = NULL */)
{
	fNodeName = "";
	fIsDirty = false;
	
	fRenamedCallbackId = 0;
	fDirtyCallbackId = 0;

	fManager = manager;
}

NodeMonitor::~NodeMonitor()
{
	stopWatching();
}

bool NodeMonitor::watch(MString nodeName)
{
	// if already watching another object, release the callbacks.
	stopWatching();

	fNodeName = nodeName;
	return attachCallbacks();
}

bool NodeMonitor::watch(MObject nodeObj)
{
	MString newNodeName = getNameFromObj(nodeObj);

	// If already watching another object, release the callbacks.
	stopWatching();

	// Get the name of the given object... since an MObject is not
	// persistent. We'll use the name of the object to attach/re-attach
	// callbacks when necessary.
	if (newNodeName == "")
	{
		// This is bad. We've been given an invalid node object.
		// return false to indicate that an error occured.

		return false;
	}

	fNodeName = newNodeName;
	return attachCallbacks();	
}

void NodeMonitor::stopWatching()
{
	if (fNodeName != "")
	{
		// Clean up the callbacks
		detachCallbacks();

		fNodeName = "";
		fIsDirty = false;
	}
}

bool NodeMonitor::dirty()
{
	return fIsDirty;
}

void NodeMonitor::cleanIt()
{
	if (dirty())
	{
		// We have to re-attach the callback.
		attachCallbacks();
	}

	fIsDirty = false;
}


//----------------------------------------------------------------
// Private interface:
//----------------------------------------------------------------

bool NodeMonitor::attachCallbacks()
{
	// Make sure that there are no callbacks currently enabled.
	detachCallbacks();

	MStatus stat;
	MObject node = getObjFromName(fNodeName, stat);
	if (!stat)
	{
		detachCallbacks();
		return false;
	}

	fIsDirty = false;

	fDirtyCallbackId = MNodeMessage::addNodeDirtyCallback(node, watchedObjectDirtyCallback, this, &stat );
	if (stat)
	{
		fRenamedCallbackId = MNodeMessage::addNameChangedCallback (node, watchedObjectRenamedCallback, this, &stat); 
	}

	// If an error occured, detach any valid callback.
	if (!stat)
	{
		detachCallbacks();
		return false;
	}

	return true;
}

void NodeMonitor::detachCallbacks()
{
	MStatus stat;

	if (fDirtyCallbackId)
	{
		stat = MMessage::removeCallback(fDirtyCallbackId);
		assert(stat);
	}

	if (fRenamedCallbackId)
	{
		stat = MMessage::removeCallback(fRenamedCallbackId);
		assert(stat);
	}

	fRenamedCallbackId = 0;
	fDirtyCallbackId = 0;
}

void NodeMonitor::callbackOccured()
{
	fIsDirty = true;

	// Detach the callback so that we don't get called needlessly. We'll
	// re-attach when the dirty bit is read back.
	detachCallbacks();
}

// Callback functions. Those are called, respectively, when a node is dirty (has changed substantially),
// or when a node is renamed.

/*static*/ void NodeMonitor::watchedObjectDirtyCallback(void* clientData)
{
	NodeMonitor* pMon = (NodeMonitor*) clientData;

	pMon->callbackOccured();
}

/*static*/ void NodeMonitor::watchedObjectRenamedCallback(MObject & node, void* clientData)
{
	NodeMonitor* pMon = (NodeMonitor*) clientData;

	// Get the new name of the node, and use it from now on.
	MString oldName = pMon->fNodeName;
	pMon->fNodeName = getNameFromObj(node);
	
	// Call the manager, if there's one.
	if (pMon->fManager != NULL)
		pMon->fManager->onNodeRenamed(node, oldName, pMon->fNodeName);

	pMon->callbackOccured();
}
