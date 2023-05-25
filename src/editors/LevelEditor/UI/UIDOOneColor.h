#pragma once
class UIDOShuffle;
class UIDOOneColor : public XrUI
{
public:
	UIDOOneColor();
	virtual ~UIDOOneColor();
	virtual void Draw();
	void RemoveObject(const xr_string &str);
	float Color[3];
	xr_vector<xr_string> list;
	int list_index;
	UIDOShuffle *DOShuffle;

private:
	void AppendItem(const xr_string &item);
};
