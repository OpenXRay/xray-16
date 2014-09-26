#include "stdafx.h"
#pragma hdrstop

#include "MeshExpUtility.h"
#include "notetrck.h"

#define BIP_BONE_CLASS_ID	0x00009125

#define OBTYPE_MESH 0
#define OBTYPE_CAMERA 1
#define OBTYPE_OMNILIGHT 2
#define OBTYPE_SPOTLIGHT 3
#define OBTYPE_DUMMY 5
#define OBTYPE_CTARGET 6
#define OBTYPE_LTARGET 7
#define OBTYPE_BONE 8

class SceneEntry {
public:
	TSTR name;
	INode *node,*tnode;
	Object *obj;
	int type;		// See above
	int id;
	SceneEntry *next;
	SceneEntry(INode *n, Object *o, int t);
	void SetID(int id) { this->id = id; }
};

SceneEntry::SceneEntry(INode *n, Object *o, int t) { 
	node = n; obj = o; type = t; next = NULL; 
	tnode = n->GetTarget();
}
//-------------------------------------------------------------------------------
class SceneEnumProc : public ITreeEnumProc {
public:
	SceneEntry*	head;
	SceneEntry*	tail;
	int			count;
	TimeValue	time;
				SceneEnumProc(INode *root_node, TimeValue t, Interface *i);
				~SceneEnumProc();
	int			Count() { return count; }
	void		Append(INode *node, Object *obj, int type);
	int			callback( INode *node );
	void		BuildNames();
};

void NodeEnum(SceneEnumProc* se, INode* node){
	// For each child of this node, we recurse into ourselves
	// until no more children are found.
	for (int c = 0; c < node->NumberOfChildren(); c++){
		se->callback(node->GetChildNode(c));
		NodeEnum(se, node->GetChildNode(c));
	}
}

SceneEnumProc::SceneEnumProc(INode *root_node, TimeValue t, Interface *ip) {
	time = t;
	count = 0;
	head = tail = NULL;
	for (int idx = 0; idx < root_node->NumberOfChildren(); idx++){
		this->callback(root_node->GetChildNode(idx));
		NodeEnum(this, root_node->GetChildNode(idx));
	}
}

SceneEnumProc::~SceneEnumProc() 
{
	while(head) {
		SceneEntry *next = head->next;
		xr_delete(head);
		head = next;
	}
	head = tail = NULL;
	count = 0;	
}

int SceneEnumProc::callback(INode *node)
{
	char line[1024];
	Object *obj = node->EvalWorldState(time).obj;
	
	strcpy(line, node->GetName());
	//if(strstr(line, "Bip") != NULL)
	{
		sprintf(line, "%08X %08X", obj->SuperClassID(), obj->ClassID());
		//		MessageBox(NULL, line, node->GetName(), MB_OK);
	}
	
	if( (obj->SuperClassID() == GEOMOBJECT_CLASS_ID) && 
		(obj->ClassID() == Class_ID(BIP_BONE_CLASS_ID, 0)) )
	{
		Append(node, obj, OBTYPE_BONE);
		return TREE_CONTINUE;
	}
	
	if (obj->CanConvertToType(triObjectClassID))
	{
		Append(node, obj, OBTYPE_MESH);
		return TREE_CONTINUE;
	}
	
	if (node->IsTarget()) 
	{
		INode* ln = node->GetLookatNode();
		if (ln) 
		{
			Object *lobj = ln->EvalWorldState(time).obj;
			switch(lobj->SuperClassID())
			{
			case LIGHT_CLASS_ID:  Append(node, obj, OBTYPE_LTARGET); break;
			case CAMERA_CLASS_ID: Append(node, obj, OBTYPE_CTARGET); break;
			}
		}
		return TREE_CONTINUE;
	}
	switch (obj->SuperClassID()) 
	{ 
	case HELPER_CLASS_ID:
		if ( obj->ClassID()==Class_ID(DUMMY_CLASS_ID,0)) 
			Append(node, obj, OBTYPE_DUMMY);
		if(obj->ClassID() == Class_ID(BONE_CLASS_ID,0))
			Append(node, obj, OBTYPE_BONE);
		break;
	case LIGHT_CLASS_ID:
		if (obj->ClassID()==Class_ID(OMNI_LIGHT_CLASS_ID,0))
			Append(node, obj, OBTYPE_OMNILIGHT);
		else 
			if (obj->ClassID()==Class_ID(SPOT_LIGHT_CLASS_ID,0)) 
				Append(node, obj, OBTYPE_SPOTLIGHT);
			//export DIR_LIGHT and FSPOT_LIGHT????
			break;
	case CAMERA_CLASS_ID:
		if (obj->ClassID()==Class_ID(LOOKAT_CAM_CLASS_ID,0))
			Append(node, obj, OBTYPE_CAMERA);
		break;
	}
	return TREE_CONTINUE;	// Keep on enumeratin'!
}


void SceneEnumProc::Append(INode *node, Object *obj, int type) {
	SceneEntry *entry = xr_new<SceneEntry>(node, obj, type);
	
	if(tail)
		tail->next = entry;
	tail = entry;
	if(!head)
		head = entry;
	count++;	
}

// We need to maintain a list of the unique objects in the scene
class ObjectEntry {
public:
	TriObject *tri;
	SceneEntry *entry;
	ObjectEntry *next;
	ObjectEntry(SceneEntry *e) { entry = e; next = NULL;  tri = NULL; }
};

class ObjectList {
public:
	ObjectEntry *head;
	ObjectEntry *tail;
	int			count;
	ObjectList(SceneEnumProc &scene);
	~ObjectList();
	int			Count() { return count; }
	void		Append(SceneEntry *e);
	ObjectEntry *Contains(Object *obj);
	ObjectEntry *Contains(INode *node);
	ObjectEntry *FindLookatNode(INode *node);
};

ObjectList::ObjectList(SceneEnumProc &scene) {
	head = tail = NULL;
	count = 0;
	// Zip thru the object list and record all unique objects (Some may be used by more than one node)
	int scount = scene.Count();
	for(SceneEntry *se = scene.head; se!=NULL; se = se->next) {
		// can't multiple instance lights and cameras in 3DS  
		// so make them all unique--DS 4/6/96
		if ( (se->type!=OBTYPE_MESH)|| !Contains(se->obj))
			Append(se);
	}
}

ObjectList::~ObjectList() 
{
	while(head) {
		ObjectEntry *next = head->next;
		xr_delete(head);
		head = next;
	}
	head = tail = NULL;
	count = 0;	
}

ObjectEntry *ObjectList::Contains(Object *obj) {
	ObjectEntry *e;
	for (e=head; e!=NULL; e = e->next) {
		if(e->entry->obj == obj)
			return e;
	}
	return NULL;
}

class FindDepNodeEnum: public DependentEnumProc {
public:
	ReferenceTarget *targ;
	INode *depNode;
	FindDepNodeEnum() { targ = NULL; depNode = NULL; }
	// proc should return 1 when it wants enumeration to halt.
	virtual	int proc(ReferenceMaker *rmaker);
};

int FindDepNodeEnum::proc(ReferenceMaker *rmaker) {
	if (rmaker->SuperClassID()!=BASENODE_CLASS_ID) return 0;
	INode* node = (INode *)rmaker;
	if (node->GetTarget()==targ) {
		depNode = node;
		return 1;
	}
	return 0;
}

ObjectEntry *ObjectList::FindLookatNode(INode *node) {
	FindDepNodeEnum	 finder;
	ObjectEntry *e;
	for (e=head; e!=NULL; e = e->next) {
		finder.targ = node;
		e->entry->node->EnumDependents(&finder);
		if (finder.depNode) return e;
	}
	return NULL;
}


ObjectEntry *ObjectList::Contains(INode *node) {
	ObjectEntry *e;
	for (e=head; e!=NULL; e = e->next) {
		if(e->entry->node == node)
			return e;
	}
	return NULL;
}

void ObjectList::Append(SceneEntry *e) {
	ObjectEntry *entry = xr_new<ObjectEntry>(e);
	if(tail)
		tail->next = entry;
	tail = entry;
	if(!head)
		head = entry;
	count++;	
}
ObjectList *theObjects = NULL;

//-----------------------------------------------------------------------------------------------------
class ObjName {
public:
	TSTR name;
	ObjName *next;
	ObjName(TSTR n) { name = n; next = NULL; }
};

class ObjNameList {
public:
	ObjName *head;
	ObjName *tail;
	int			count;
	ObjNameList() { head = tail = NULL; count = 0; }
	~ObjNameList();
	int			Count() { return count; }
	int			Contains(TSTR &n);
	void		Append(TSTR &n);
	void		MakeUnique(TSTR &n);
};

ObjNameList::~ObjNameList() 
{
	while(head) {
		ObjName *next = head->next;
		xr_delete(head);
		head = next;
	}
	head = tail = NULL;
	count = 0;	
}

int ObjNameList::Contains(TSTR &n) {
	ObjName *e = head;
	int index = 0;
	while(e) {
		if(e->name == n)
			return index;
		e = e->next;
		index++;
	}
	return -1;
}

void ObjNameList::Append(TSTR &n) {
	ObjName *entry = xr_new<ObjName>(n);
	if(tail)
		tail->next = entry;
	tail = entry;
	if(!head)
		head = entry;
	count++;	
}

void ObjNameList::MakeUnique(TSTR &n) {
	// First make it less than 10 chars.
	if (n.Length()>10) n.Resize(10);
	
	if(Contains(n) < 0) {
		Append(n);
		return;
	}
	// Make it unique and keep it 10 chars or less
	for(int i = 0; i < 100000; ++i) {
		char buf[12];
		sprintf(buf,"%d",i);
		TSTR num(buf);
		TSTR work = n;
		int totlen = num.Length() + work.Length();
		if(totlen > 10)
			work.Resize(10 - (totlen - 10));
		work = work + num;
		if(Contains(work) < 0) {
			Append(work);
			n = work;
			return;
		}
	}
	// Forget it!
}

ObjNameList theObjNames;
//--------------------------------------------------------------------------------------------------


void SceneEnumProc::BuildNames()
{
	ObjNameList nameList;
	SceneEntry *ptr = head;
	
	while (ptr) {
		//		if (ptr->node->IsTarget()) {
		//			ptr->name = _T("");
		//			}
		//		else {
		ptr->name = ptr->node->GetName();
		nameList.MakeUnique(ptr->name);		
		//			}
		ptr = ptr->next;
	}
}
//--------------------------------------------------------------------------------------------------


bool MeshExpUtility::SaveSkinKeys(const char* n){
	CFS_Memory F;

	int FramesPerSecond = GetFrameRate();
	int TicksPerFrame	= GetTicksPerFrame();
	int FirstTick		= ip->GetAnimRange().Start();
	int LastTick		= ip->GetAnimRange().End();

	Point3 v;
	Matrix3 tm;

	// Write signature and version
	char S[MAX_PATH];
	sprintf(S, "KEYEXP 3.0");
	F.Wstring(S);

	INode *node;
	ObjectEntry *Current;

	//-----------------------------------------------------------------------
	// Count bones and report

	int NumBones = 0;
	Current = theObjects->head;
	while(Current)
	{
		/*
		if(Current->entry->type != OBTYPE_BONE)
		{
			Current = Current->next;
			continue;
		}
		*/

		NumBones++;

		Current = Current->next;
	}

	sprintf(S, "Number of Bones = %d", NumBones);
	F.Wstring(S);
	ELog.Msg(mtInformation,S);

	//-----------------------------------------------------------------------
	// Write out necessary data for motion keys

	sprintf(S, "Key Data");
	F.Wstring(S);

	TimeValue t;
	Quat qq;
	Point3 tp, sp;
	INode* parent;
	Matrix3 tmp;

	sprintf(S, "%d %d %d", FirstTick / TicksPerFrame, LastTick / TicksPerFrame, FramesPerSecond);
	F.Wstring(S);

	Current = theObjects->head;
	while(Current){
		/*
		if(Current->entry->type != OBTYPE_BONE)
		{
			Current = Current->next;
			continue;
		}
		*/

		Matrix3 tm;

		node = Current->entry->node;

		sprintf(S, "Node: %s", node->GetName());
		F.Wstring(S);
		ELog.Msg(mtInformation,S);

		// Print notetrack info
		{
			int NumNT, n, i, j, NumNotes;
			NoteTrack* pNT;
			DefNoteTrack* pDNT;
			NoteKey* pNK;

			NumNT = node->NumNoteTracks();

			// count all of the notes on all of the notetracks
			NumNotes = 0;
			for(n=0;n<NumNT;n++){
				pNT = node->GetNoteTrack(n);
				if(pNT->ClassID() == Class_ID(NOTETRACK_CLASS_ID, 0)){
					pDNT = (DefNoteTrack*)pNT;
					j = pDNT->NumKeys();
					for(i=0;i<j;i++){
						pNK = pDNT->keys[i];
						if( (pNK->time >= FirstTick) && (pNK->time <= LastTick) )
							NumNotes++;
					}
				}
			}

			sprintf(S, "Number of Notes = %d", NumNotes);
			F.Wstring(S);

			for(n=0;n<NumNT;n++){
				pNT = node->GetNoteTrack(n);
				if(pNT->ClassID() == Class_ID(NOTETRACK_CLASS_ID, 0)){
					pDNT = (DefNoteTrack*)pNT;
					j = pDNT->NumKeys();
					for(i=0;i<j;i++){
						pNK = pDNT->keys[i];
						if( (pNK->time >= FirstTick) && (pNK->time <= LastTick) ){
							sprintf(S, "%d: %s", (pNK->time - FirstTick) / TicksPerFrame, pNK->note);
							F.Wstring(S);
						}
					}
				}
			}
		}

		for(t=FirstTick;t<=LastTick;t+=TicksPerFrame){
			tm = node->GetNodeTM(t);
			DecomposeMatrix(tm, tp, qq, sp);
			qq.MakeMatrix	(tm);
			tm.SetTrans		(tp);

			parent = node->GetParentNode();
			if(parent){
				tmp = parent->GetNodeTM(t);
				DecomposeMatrix(tmp, tp, qq, sp);
				qq.MakeMatrix(tmp);
				tmp.SetTrans(tp);
				tmp = Inverse(tmp);
				tm *= tmp;
			}

			DecomposeMatrix(tm, tp, qq, sp);
			sprintf(S,"%f %f %f %f",qq.x, qq.y, qq.z, qq.w);
			F.Wstring(S);
			sprintf(S,"%f %f %f",tp.x,tp.y,tp.z);
			F.Wstring(S);

			/*
			// Euler angles
			Point3			E;
			QuatToEuler		(Quat(tm), E);
			fprintf			(f,"%f %f %f",E.x,E.y,E.z);

			// Translate
			DecomposeMatrix	(tm, tp, qq, sp);
			fprintf			(f,"%f %f %f",tp.x,tp.y,tp.z);
			*/

//			Matrix3fprint(f, tm);
		}
		Current = Current->next;
	}

	sprintf(S, "Key Data Complete");
	F.Wstring(S);
	ELog.Msg(mtInformation,S);

	F.SaveTo(n,0);

	return true;
}

void MeshExpUtility::ExportSkinKeys(){
	bool bResult = true;

	// Make sure there are nodes we're interested in!
	// Ask the scene to enumerate all its nodes so we can determine if there are any we can use
	INode *root_node = ip->GetRootNode();
	if(!root_node){
		ELog.Msg(mtError,"Scene empty." );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return;
	}

	SceneEnumProc myScene(root_node, ip->GetTime(), ip);

	// Any useful nodes?
	if(!myScene.Count()){
		ELog.Msg(mtError,"Scene has no useful nodes." );
		ELog.Msg(mtError,"-------------------------------------------------------" );
		return;
	}

	char m_ExportName[MAX_PATH];
	m_ExportName[0]=0;
	if( !Engine.FS.GetSaveName(Engine.FS.m_GameKeys,m_ExportName,MAX_PATH,0) ){
		ELog.Msg(mtInformation,"Export cancelled" );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return;
	}

	// Construct unique names < 10 chars.
	myScene.BuildNames();

	ObjectList myObjects(myScene);

	theObjects = &myObjects;

	bResult  = SaveSkinKeys(m_ExportName);

	ELog.Msg(mtInformation,"-------------------------------------------------------" );
	if (bResult) ELog.Msg(mtInformation,"Export completed" );
	else		 ELog.Msg(mtError,"Export failed***********************" );
	ELog.Msg(mtInformation,"-------------------------------------------------------" );
}

