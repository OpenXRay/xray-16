#include "stdafx.h"

UILPropertiesFrom::UILPropertiesFrom()
{
}

UILPropertiesFrom::~UILPropertiesFrom()
{
}

void UILPropertiesFrom::Draw()
{
	if (bOpen)
	{
		if (ImGui::Begin("Properties", &bOpen))
		{
			LTools->GetProperties()->Draw();
		}
		ImGui::End();
	}
}
