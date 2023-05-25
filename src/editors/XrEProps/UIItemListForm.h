#pragma once
class XREPROPS_API UIItemListForm : public XrUI, private FolderHelper<ListItem, true>
{
	TOnILItemsFocused OnItemsFocusedEvent;
	TOnILItemFocused OnItemFocusedEvent;
	TOnItemRemove OnItemRemoveEvent;
	TOnItemRename OnItemRenameEvent;
	TOnItemCreate OnItemCreateEvent;
	TOnItemClone OnItemCloneEvent;

public:
	UIItemListForm();
	virtual ~UIItemListForm();

public:
	virtual void Draw();
	void ClearList();
	void RemoveSelectItem();
	void ClearSelected();
	void SelectItem(const char *name);
	void AssignItems(ListItemsVec &items, const char *name_selection = nullptr, bool clear_Folder = true, bool save_selected = false);
	IC const ListItemsVec &GetItems() const { return m_Items; }
	bool GetSelected(RStringVec &items) const;
	int GetSelected(LPCSTR pref, ListItemsVec &items, bool bOnlyObject);

public:
	enum
	{
		fMenuEdit = (1 << 0),
		fMultiSelect = (1 << 1),
	};
	Flags32 m_Flags;

private:
	void DrawMenuEdit();
	string4096 m_edit_name;
	string4096 m_edit_path;
	Node *m_edit_node;

public:
	IC void SetOnItemsFocusedEvent(TOnILItemsFocused e) { OnItemsFocusedEvent = e; }
	IC void SetOnItemFocusedEvent(TOnILItemFocused e) { OnItemFocusedEvent = e; }
	IC void SetOnItemRemoveEvent(TOnItemRemove e) { OnItemRemoveEvent = e; }
	IC void SetOnItemRenameEvent(TOnItemRename e) { OnItemRenameEvent = e; }
	IC void SetOnItemCreaetEvent(TOnItemCreate e) { OnItemCreateEvent = e; }
	IC void SetOnItemCloneEvent(TOnItemClone e) { OnItemCloneEvent = e; }

private:
	virtual void DrawAfterFolderNode(bool is_open, Node *Node = 0);
	virtual void DrawItem(Node *Node);
	virtual bool IsDrawFolder(Node *Node);
	virtual void IsItemClicked(Node *Node);
	virtual bool IsFolderBullet(Node *Node);
	virtual bool IsFolderSelected(Node *Node);

private:
	virtual void EventRenameNode(Node *Node, const char *old_path, const char *new_path);
	virtual void EventRemoveNode(Node *Node, const char *path);

public:
	Node m_GeneralNode;
	ListItemsVec m_Items;
	ListItemsVec m_SelectedItems;
	void ClearSelectedItems();
	bool m_UseMenuEdit;
	void ClearObject(Node *Node);
};
