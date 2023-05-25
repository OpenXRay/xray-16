#pragma once
template <class C, bool FolderAsItem = false>
class FolderHelper
{
public:
	enum EFolderNodeType
	{
		FNT_Root,
		FNT_Folder,
		FNT_Object
	};
	struct Node
	{
		Node() : Object(nullptr), Type(FNT_Root), Selected(false) {}
		~Node() {}
		bool Selected;
		EFolderNodeType Type;
		shared_str Name;
		shared_str Path;
		xr_vector<Node> Nodes;
		mutable C *Object;
		IC bool IsObject() { return Type == FNT_Object; }
		IC bool IsFolder() { return Type == FNT_Folder || Type == FNT_Root; }
	};
	FolderHelper() {}
	inline ~FolderHelper() {}
	inline Node *SelectObject(Node *N, const char *path)
	{
		VERIFY(N);
		VERIFY(N->IsFolder());
		if (strchr(path, '\\'))
		{
			string_path name;
			xr_strcpy(name, path);
			strchr(name, '\\')[0] = 0;
			N = FindFolder(N, name);
			VERIFY(N);
			N->Selected = true;
			return SelectObject(N, strchr(path, '\\') + 1);
		}
		for (Node &node : N->Nodes)
		{
			if (FolderAsItem)
			{
				if (node.Name == path && node.Object)
				{
					node.Selected = true;
					return &node;
				}
			}
			else
			{
				if (node.Name == path && node.IsObject())
				{
					node.Selected = true;
					return &node;
				}
			}
		}
		VERIFY(false);
		return nullptr;
	}
	inline Node *FindObject(Node *N, const char *path)
	{
		if (N == nullptr)
			return nullptr;
		if (!N->IsFolder())
			return nullptr;
		if (strchr(path, '\\'))
		{
			string_path name;
			xr_strcpy(name, path);
			strrchr(name, '\\')[0] = 0;
			path = strrchr(path, '\\') + 1;
			N = FindFolder(N, name);
			if (N == nullptr)
				return nullptr;
		}
		for (Node &node : N->Nodes)
		{
			if (node.Name == path && node.IsObject())
				return &node;
		}
		return nullptr;
	}

	inline Node *Find(Node *N, const char *path)
	{
		if (N == nullptr)
			return nullptr;

		if (!N->IsFolder())
			return nullptr;

		if (strchr(path, '\\'))
		{
			string_path name;
			xr_strcpy(name, path);
			strrchr(name, '\\')[0] = 0;
			path = strrchr(path, '\\') + 1;
			N = FindFolder(N, name);

			if (!N)
				return nullptr;
		}

		for (Node &node : N->Nodes)
		{
			if (node.Name == path)
				return &node;
		}

		return nullptr;
	}

	inline Node *FindFolder(Node *N, const char *path)
	{
		if (!N)
			return nullptr;

		if (!N->IsFolder())
			return nullptr;

		if (strchr(path, '\\'))
		{
			string_path name;
			xr_strcpy(name, path);
			strchr(name, '\\')[0] = 0;
			return FindFolder(FindFolder(N, name), strchr(path, '\\') + 1);
		}

		for (Node &node : N->Nodes)
		{
			if (node.Name == path && node.IsFolder())
				return &node;
		}

		return nullptr;
	}

	inline Node *AppendFolder(Node *N, const char *path)
	{
		if (!N)
			return nullptr;

		if (strchr(path, '\\'))
		{
			string_path name;
			xr_strcpy(name, path);
			strchr(name, '\\')[0] = 0;

			Node *NextNode = FindFolder(N, name);
			if (NextNode)
				return AppendFolder(NextNode, strchr(path, '\\') + 1);

			return AppendFolder(AppendFolder(N, name), strchr(path, '\\') + 1);
		}

		for (Node &node : N->Nodes)
		{
			if (node.Name == path)
			{
				if constexpr (FolderAsItem)
				{
					node.Type = FNT_Folder;
					return &node;
				}
				else
				{
					if (node.IsFolder())
						return &node;
					// return nullptr;
				}
			}
		}
		Node NewNode;
		NewNode.Type = FNT_Folder;
		NewNode.Name = path;
		if (N->Path.size() == 0)
		{
			if (N->Name.size())
				NewNode.Path = N->Name;
			else
				NewNode.Path = "";
		}
		else
			NewNode.Path.printf("%s\\%s", N->Path.c_str(), N->Name.c_str());

		N->Nodes.push_back(NewNode);
		return &N->Nodes.back();
	}

	inline Node *AppendObject(Node *N, const char *path)
	{
		if (!N)
			return nullptr;

		if (strchr(path, '\\'))
		{
			string_path name;
			xr_strcpy(name, path);

			char *slash = strchr(name, '\\');
			*slash = '\0';

			Node *NextNode = FindFolder(N, name);
			if (NextNode)
				return AppendObject(NextNode, strchr(path, '\\') + 1);

			return AppendObject(AppendFolder(N, name), strchr(path, '\\') + 1);
		}

		for (Node &node : N->Nodes)
		{
			if (node.Name == path)
			{
				if constexpr (FolderAsItem)
				{
					if (node.IsObject())
						return &node;
					return nullptr;
				}
				else
					return &node;
			}
		}

		Node NewNode;
		NewNode.Type = FNT_Object;
		NewNode.Name = path;

		if (N->Path.size() == 0)
		{
			if (N->Name.size())
				NewNode.Path = N->Name;
			else
				NewNode.Path = "";
		}
		else
			NewNode.Path.printf("%s\\%s", N->Path.c_str(), N->Name.c_str());

		N->Nodes.push_back(NewNode);
		return &N->Nodes.back();
	}

	inline void Remove(Node *N, Node *Object, bool use_event = false)
	{
		Node *F = Object->Path.size() == 0 ? N : FindFolder(N, Object->Path.c_str());
		R_ASSERT(F);
		string_path path;
		if (use_event)
		{
			RemoveNode(Object);
			GetFullPath(Object, path);
			EventRemoveNode(Object, path);
		}
		R_ASSERT(RemoveNodeFromN(F, Object));
	}
	inline bool Move(Node *N, Node *Object, const char *new_path)
	{
		{
			Node *ObjectNew = nullptr;
			bool IsFolder = false;

			string_path old_path;
			GetFullPath(Object, old_path);

			if (Object->IsFolder())
			{
				IsFolder = true;
				ObjectNew = AppendFolder(N, new_path);
			}
			else

			{
				ObjectNew = AppendObject(N, new_path);
			}
			if (IsFolder)
			{
				Object = FindFolder(N, old_path);
			}
			else

			{
				Object = FindObject(N, old_path);
			}
			if (ObjectNew == Object)
				return false;
			if (!ObjectNew)
				return false;

			SwapData(ObjectNew, Object);
			Remove(N, Object);
			if (IsFolder)
			{
				ObjectNew = FindFolder(N, new_path);
			}
			else
			{
				ObjectNew = FindObject(N, new_path);
			}
			R_ASSERT(ObjectNew);
			EventRenameNode(ObjectNew, old_path, new_path);
			RebuildPath(ObjectNew);
			ObjectNew->Selected = true;
		}
		return true;
	}

	virtual void DrawNode(Node *N)
	{
		if (N->Type == FNT_Root)
		{
			for (Node &node : N->Nodes)
			{
				if (node.IsFolder() && IsDrawFolder(&node))
				{
					DrawNode(&node);
				}
			}
			for (Node &node : N->Nodes)
			{
				if (!node.IsFolder())
				{
					DrawNode(&node);
				}
			}
		}
		else if (N->IsFolder())
		{
			if (N->Selected)
				ImGui::SetNextItemOpen(true);

			ImGui::AlignTextToFramePadding();
			ImGuiTreeNodeFlags FolderFlags = ImGuiTreeNodeFlags_OpenOnArrow;

			if (IsFolderBullet(N))
				FolderFlags |= ImGuiTreeNodeFlags_Bullet;

			if (IsFolderSelected(N))
				FolderFlags |= ImGuiTreeNodeFlags_Selected;

			if (ImGui::TreeNodeEx(N->Name.c_str(), FolderFlags))
			{
				DrawAfterFolderNode(true, N);
				if (ImGui::IsItemClicked() && N->Object)
					IsItemClicked(N);
				for (Node &node : N->Nodes)
				{
					if (node.IsFolder() && IsDrawFolder(&node))
					{
						DrawNode(&node);
					}
				}
				for (Node &node : N->Nodes)
				{
					if (!node.IsFolder())
					{
						DrawNode(&node);
					}
				}
				ImGui::TreePop();
			}
			else
			{
				DrawAfterFolderNode(false, N);
				if (ImGui::IsItemClicked() && N->Object)
					IsItemClicked(N);
			}
			if (N->Selected)
				N->Selected = false;
		}
		else if (N->IsObject())
		{
			DrawItem(N);
			if (N->Selected)
				N->Selected = false;
		}
	}
	virtual void DrawAfterFolderNode(bool is_open = false, Node *Node = 0) {}
	virtual bool IsFolderBullet(Node *Node) { return false; }
	virtual bool IsFolderSelected(Node *Node) { return false; }
	virtual void EventRenameNode(Node *Node, const char *old_path, const char *new_path) {}
	virtual void EventRemoveNode(Node *Node, const char *path) {}
	inline void GetFullPath(Node *Object, string_path &full_path)
	{
		if (Object->Path.c_str() && Object->Path.c_str()[0])
			xr_strcpy(full_path, Object->Path.c_str());
		else
			full_path[0] = 0;

		if (full_path[0])
			xr_strcat(full_path, "\\");
		xr_strcat(full_path, Object->Name.c_str());
	}
	virtual void IsItemClicked(Node *Node) {}
	virtual bool IsDrawFolder(Node *Node) = 0;
	virtual void DrawItem(Node *Node) = 0;

private:
	inline void SwapData(Node *Dst, Node *Src)
	{
		std::swap(Dst->Type, Src->Type);
		std::swap(Dst->Object, Src->Object);
		Dst->Nodes.swap(Src->Nodes);
	}
	inline void RebuildPath(Node *N)
	{
		for (Node &n : N->Nodes)
		{

			string_path old_path, new_path;
			GetFullPath(&n, old_path);

			if (N->Path.size() == 0)
			{
				if (N->Name.size())
				{
					n.Path = N->Name;
				}
				else
				{

					n.Path = "";
				}
			}
			else
			{

				n.Path.printf("%s\\%s", N->Path.c_str(), N->Name.c_str());
			}
			GetFullPath(&n, new_path);

			EventRenameNode(&n, old_path, new_path);
			RebuildPath(&n);
		}
	}

	inline void RemoveNode(Node *Object)
	{
		for (Node &n : Object->Nodes)
		{
			string_path path;
			RemoveNode(&n);
			GetFullPath(&n, path);
			EventRemoveNode(&n, path);
		}
		Object->Nodes.clear();
	}

	inline bool RemoveNodeFromN(Node *N, Node *Object)
	{
		for (auto b = N->Nodes.begin(), e = N->Nodes.end(); b != e; b++)
		{
			if (&(*b) == Object)
			{
				N->Nodes.erase(b);
				return true;
			}
		}
		return false;
	}
};
