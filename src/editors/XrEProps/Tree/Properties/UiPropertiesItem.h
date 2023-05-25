#pragma once

class XREPROPS_API UIPropertiesItem : public UITreeItem
{
public:
	UIPropertiesItem(shared_str Name, UIPropertiesForm* PropertiesFrom);
	virtual ~UIPropertiesItem() = default;
	PropItem* PItem;
	UIPropertiesForm* PropertiesFrom;
	void Draw();
	void DrawRoot();
	void DrawItem();
	void DrawProp();

protected:
	virtual UITreeItem* CreateItem(shared_str Name);

private:
	void RemoveMixed();
};
