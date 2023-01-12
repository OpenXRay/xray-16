#include "stdafx.h"

#include "embedded_editor_weather.h"

#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/thunderbolt.h"
#include "xrEngine/xr_efflensflare.h"

#include "xrScriptEngine/script_engine.hpp"

#include "GamePersistent.h"
#include "Level.h"
#include "ai_space.h"

#include <imgui.h>

float editor_longitude = 0.0;
float editor_altitude = 0.0;

Fvector convert(const Fvector& v)
{
    Fvector result;
    result.set(v.z, v.y, v.x);
    return result;
}

Fvector4 convert(const Fvector4& v)
{
    Fvector4 result;
    result.set(v.z, v.y, v.x, v.w);
    return result;
}

bool enumCycle(void* data, int idx, const char** item)
{
    xr_vector<shared_str>* cycles = (xr_vector<shared_str>*)data;
    *item = (*cycles)[idx].c_str();
    return true;
}

bool enumWeather(void* data, int idx, const char** item)
{
    xr_vector<CEnvDescriptor*>* envs = (xr_vector<CEnvDescriptor*>*)data;
    *item = (*envs)[idx]->m_identifier.c_str();
    return true;
}

const char* empty = "";
bool enumIniWithEmpty(void* data, int idx, const char** item)
{
	if (idx == 0)
		*item = empty;
	else {
		CInifile* ini = (CInifile*)data;
		*item = ini->sections()[idx - 1]->Name.c_str();
	}
	return true;
}

bool enumIni(void* data, int idx, const char** item)
{
	CInifile* ini = (CInifile*)data;
	*item = ini->sections()[idx]->Name.c_str();
	return true;
}

/*bool getScriptWeather()
{
    luabind::object benchmark = ai().script_engine().name_space("benchmark");
    return benchmark["weather"].type() == LUA_TBOOLEAN ? !luabind::object_cast<bool>(benchmark["weather"]) : true;
}

void setScriptWeather(bool b)
{
    luabind::object benchmark = ai().script_engine().name_space("benchmark");
    benchmark["weather"] = !b;
}*/

xr_set<shared_str> modifiedWeathers;

void saveWeather(shared_str name, const xr_vector<CEnvDescriptor*>& env)
{
	CInifile f(nullptr, FALSE, FALSE, FALSE);
	for (auto el : env) {
		if (el->env_ambient)
			f.w_string(el->m_identifier.c_str(), "ambient", el->env_ambient->name().c_str());
		f.w_fvector3(el->m_identifier.c_str(), "ambient_color", el->ambient);
		f.w_fvector4(el->m_identifier.c_str(), "clouds_color", el->clouds_color);
		f.w_string(el->m_identifier.c_str(), "clouds_texture", el->clouds_texture_name.c_str());
		f.w_float(el->m_identifier.c_str(), "far_plane", el->far_plane);
		f.w_float(el->m_identifier.c_str(), "fog_distance", el->fog_distance);
		f.w_float(el->m_identifier.c_str(), "fog_density", el->fog_density);
		f.w_fvector3(el->m_identifier.c_str(), "fog_color", el->fog_color);
		f.w_fvector3(el->m_identifier.c_str(), "rain_color", el->rain_color);
		f.w_float(el->m_identifier.c_str(), "rain_density", el->rain_density);
		f.w_fvector3(el->m_identifier.c_str(), "sky_color", el->sky_color);
		f.w_float(el->m_identifier.c_str(), "sky_rotation", rad2deg(el->sky_rotation));
		f.w_string(el->m_identifier.c_str(), "sky_texture", el->sky_texture_name.c_str());
		f.w_fvector3(el->m_identifier.c_str(), "sun_color", el->sun_color);
		f.w_float(el->m_identifier.c_str(), "sun_shafts_intensity", el->m_fSunShaftsIntensity);
		f.w_string(el->m_identifier.c_str(), "sun", el->lens_flare_id.c_str());
		f.w_string(el->m_identifier.c_str(), "thunderbolt_collection", el->tb_id.c_str());
		f.w_float(el->m_identifier.c_str(), "thunderbolt_duration", el->bolt_duration);
		f.w_float(el->m_identifier.c_str(), "thunderbolt_period", el->bolt_period);
		f.w_float(el->m_identifier.c_str(), "water_intensity", el->m_fWaterIntensity);
		f.w_float(el->m_identifier.c_str(), "wind_direction", rad2deg(el->wind_direction));
		f.w_float(el->m_identifier.c_str(), "wind_velocity", el->wind_velocity);
		f.w_fvector4(el->m_identifier.c_str(), "hemisphere_color", el->hemi_color);
		f.w_float(el->m_identifier.c_str(), "sun_altitude", rad2deg(el->sun_dir.getH()));
		f.w_float(el->m_identifier.c_str(), "sun_longitude", rad2deg(el->sun_dir.getP()));
		f.w_float(el->m_identifier.c_str(), "tree_amplitude_intensity", el->m_fTreeAmplitudeIntensity);
	}
	string_path fileName;
	FS.update_path(fileName, "$game_weathers$", name.c_str());
	strconcat(sizeof(fileName), fileName, fileName, ".ltx");
	f.save_as(fileName);
}

void nextTexture(char* tex, int texSize, int offset)
{
	string_path dir, fn;
	_splitpath(tex, nullptr, dir, fn, nullptr);
	strconcat(sizeof(fn), fn, fn, ".dds");
	xr_vector<LPSTR>* files = FS.file_list_open("$game_textures$", dir, FS_ListFiles);
	if (!files)
		return;
	size_t index = 0;
	for (size_t i = 0; i != files->size(); i++)
		if (strcmp((*files)[i], fn) == 0) {
			index = i;
			break;
		}
	size_t newIndex = index;
	while (true) {
		newIndex = (newIndex + offset + files->size()) % files->size();
		if (strstr((*files)[newIndex], "#small") == nullptr && strstr((*files)[newIndex], ".thm") == nullptr)
			break;
	}
	string_path newFn;
	_splitpath((*files)[newIndex], nullptr, nullptr, newFn, nullptr);
	strconcat(texSize, tex, dir, newFn);
	FS.file_list_close(files);
}

bool ImGui_ListBox(const char* label, int* current_item, bool(*items_getter)(void*, int, const char**), void* data,
	int items_count, const ImVec2& size_arg = ImVec2(0, 0));

////////////////////////////////////////////////////////////////////////////////////////////////////
// from https://www.strchr.com/natural_sorting
////////////////////////////////////////////////////////////////////////////////////////////////////

int count_digits(const char* s)
{
	const char* p = s;
	while (isdigit(*p))
		p++;
	return (int)(p - s);
}

int compare_naturally(const void* a_ptr, const void* b_ptr)
{
	const char* a = (const char*)a_ptr;
	const char* b = (const char*)b_ptr;

	for (;;) {
		if (isdigit(*a) && isdigit(*b)) {
			int a_count = count_digits(a);
			int diff = a_count - count_digits(b);
			if (diff)
				return diff;
			diff = memcmp(a, b, a_count);
			if (diff)
				return diff;
			a += a_count;
			b += a_count;
		}
		if (*a != *b)
			return *a - *b;
		if (*a == '\0')
			return 0;
		a++, b++;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool editTexture(const char* label, shared_str& texName)
{
	char tex[100];
	strncpy(tex, texName.empty() ? "" : texName.c_str(), 100);
	bool changed = false;
	static shared_str prevValue;
	ImGui::PushID(label);
	if (ImGui::InputText("", tex, 100)) {
		texName = tex;
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("...")) {
		ImGui::OpenPopup("Choose texture");
		prevValue = texName;
	}
	ImGui::SameLine();
	ImGui::Text(label);
	ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Choose texture", NULL, 0)) {
		string_path dir, fn;
		_splitpath(tex, nullptr, dir, fn, nullptr);
		strconcat(sizeof(fn), fn, fn, ".dds");
		static xr_map<xr_string, xr_vector<xr_string>> dirs;
		auto filtered = dirs[dir];
		if (filtered.empty()) {
			xr_vector<LPSTR>* files = FS.file_list_open("$game_textures$", dir, FS_ListFiles);
			if (files) {
				filtered.resize(files->size());
				auto e = std::copy_if(files->begin(), files->end(), filtered.begin(),
					[](auto x) { return strstr(x, "#small") == nullptr && strstr(x, ".thm") == nullptr; });
				filtered.resize(e - filtered.begin());
				std::sort(filtered.begin(), filtered.end(),
					[](auto a, auto b) { return compare_naturally(a.c_str(), b.c_str()) < 0; });
				dirs[dir] = filtered;
			}
			FS.file_list_close(files);
		}
		int cur = -1;
		for (size_t i = 0; i != filtered.size(); i++)
			if (filtered[i] == fn) {
				cur = i;
				break;
			}
		if (ImGui_ListBox("", &cur,
			[](void* data, int idx, const char** out_text) -> bool {
				xr_vector<xr_string>* textures = (xr_vector<xr_string>*)data;
				*out_text = (*textures)[idx].c_str();
				return true;
			},
			&filtered, filtered.size(), ImVec2(-1.0f, -20.0f))) {
			string_path newFn;
			_splitpath(filtered[cur].c_str(), nullptr, nullptr, newFn, nullptr);
			strconcat(100, tex, dir, newFn);
			texName = tex;
			changed = true;
		}
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			string_path newFn;
			_splitpath(prevValue.empty() ? "" : prevValue.c_str(), nullptr, nullptr, newFn, nullptr);
			strconcat(100, tex, dir, newFn);
			texName = tex;
			changed = true;
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return changed;
}

void ShowWeatherEditor(bool& show)
{
	if (!ImGui::Begin(modifiedWeathers.empty() ? "Weather###Weather" : "Weather*###Weather", &show)) {
        ImGui::End();
        return;
    }
    CEnvironment& env = GamePersistent().Environment();
    CEnvDescriptor* cur = env.Current[0];
    u64 time = Level().GetEnvironmentGameTime() / 1000;
    ImGui::Text("Time: %02d:%02d:%02d", int(time / (60 * 60) % 24), int(time / 60 % 60), int(time % 60));
    float tf = Level().GetEnvironmentTimeFactor();
    if (ImGui::SliderFloat("Time factor", &tf, 0.0f, 1000.0f, "%.3f", 2.0f))
        Level().SetEnvironmentTimeFactor(tf);
    xr_vector<shared_str> cycles;
    int iCycle = -1;
    for (const auto& el : env.WeatherCycles) {
        cycles.push_back(el.first);
        if (el.first == env.CurrentWeatherName)
            iCycle = cycles.size() - 1;
    }

	ImGui::Text(u8"Main parameters");

    if (ImGui::Combo("Weather cycle", &iCycle, enumCycle, &cycles, env.WeatherCycles.size()))
        env.SetWeather(cycles[iCycle], true);
    int sel = -1;
    for (int i = 0; i != env.CurrentWeather->size(); i++)
        if (cur->m_identifier == env.CurrentWeather->at(i)->m_identifier)
            sel = i;
    if (ImGui::Combo("Current section", &sel, enumWeather, env.CurrentWeather, env.CurrentWeather->size())) {
        env.SetGameTime(env.CurrentWeather->at(sel)->exec_time + 0.5f, tf);
        time = time / (24 * 60 * 60) * 24 * 60 * 60 * 1000;
        time += u64(env.CurrentWeather->at(sel)->exec_time * 1000 + 0.5f);
        Level().SetEnvironmentGameTimeFactor(time, tf);
        env.SetWeather(cycles[iCycle], true);
    }
    /* static bool b = getScriptWeather();
    if (ImGui::Checkbox("Script weather", &b))
        setScriptWeather(b);*/
    ImGui::Separator();
	bool changed = false;
	sel = -1;
	for (int i = 0; i != env.m_ambients_config->sections().size(); i++)
		if (cur->env_ambient->name() == env.m_ambients_config->sections()[i]->Name)
			sel = i;

	ImGui::Text(u8"Ambient light parameters");

	if (ImGui::Combo("ambient", &sel, enumIni, env.m_ambients_config, env.m_ambients_config->sections().size())) {
		cur->env_ambient = env.AppendEnvAmb(env.m_ambients_config->sections()[sel]->Name);
		changed = true;
	}
	if (ImGui::ColorEdit3("ambient_color", (float*)&cur->ambient))
		changed = true;

	ImGui::Text(u8"Clouds parameters");

	if (ImGui::ColorEdit4("clouds_color", (float*)&cur->clouds_color, ImGuiColorEditFlags_AlphaBar))
		changed = true;
	char buf[100];
	if (editTexture("clouds_texture", cur->clouds_texture_name)) 
	{
		cur->on_device_create();
		changed = true;
	}

	ImGui::Text(u8"Fog parameters");

	if (ImGui::SliderFloat("far_plane", &cur->far_plane, 0.01f, 10000.0f))
		changed = true;
	if (ImGui::SliderFloat("fog_distance", &cur->fog_distance, 0.0f, 10000.0f))
		changed = true;
	if (ImGui::SliderFloat("fog_density", &cur->fog_density, 0.0f, 10.0f))
		changed = true;
	if (ImGui::ColorEdit3("fog_color", (float*)&cur->fog_color))
		changed = true;

	ImGui::Text(u8"Hemi parameters");

	if (ImGui::ColorEdit4("hemisphere_color", (float*)&cur->hemi_color, ImGuiColorEditFlags_AlphaBar))
		changed = true;

	ImGui::Text(u8"Rain parameters");

	if (ImGui::SliderFloat("rain_density", &cur->rain_density, 0.0f, 10.0f))
		changed = true;
	if (ImGui::ColorEdit3("rain_color", (float*)&cur->rain_color))
		changed = true;

	ImGui::Text(u8"Sky parameters");

	Fvector temp;
	temp = convert(cur->sky_color);
	if (ImGui::ColorEdit3("sky_color", (float*)&temp))
		changed = true;
	cur->sky_color = convert(temp);
	if (ImGui::SliderFloat("sky_rotation", &cur->sky_rotation, 0.0f, 6.28318f))
		changed = true;
	if (editTexture("sky_texture", cur->sky_texture_name)) {
		strconcat(sizeof(buf), buf, cur->sky_texture_name.empty() ? "" : cur->sky_texture_name.c_str(), "#small");
		cur->sky_texture_env_name = buf;
		cur->on_device_create();
		changed = true;
	}

	ImGui::Text(u8"Sun parameters");

	sel = -1;
	for (int i = 0; i != env.m_suns_config->sections().size(); i++)
		if (cur->lens_flare_id == env.m_suns_config->sections()[i]->Name)
			sel = i;
	if (ImGui::Combo("sun", &sel, enumIni, env.m_suns_config, env.m_suns_config->sections().size())) {
		cur->lens_flare_id
			= env.eff_LensFlare->AppendDef(env, env.m_suns_config, env.m_suns_config->sections()[sel]->Name.c_str());
		env.eff_LensFlare->Invalidate();
		changed = true;
	}
	if (ImGui::ColorEdit3("sun_color", (float*)&cur->sun_color))
		changed = true;
	if (ImGui::SliderFloat("sun_altitude", &editor_altitude, -360.0f, 360.0f))
	{
		changed = true;
		if (changed)
			cur->sun_dir.setHP(deg2rad(editor_longitude), deg2rad(editor_altitude));
		else
			editor_altitude = cur->sun_dir.getH();
	}
	if (ImGui::SliderFloat("sun_longitude", &editor_longitude, -360.0f, 360.0f))
	{
		changed = true;
		if (changed)
			cur->sun_dir.setHP(deg2rad(editor_longitude), deg2rad(editor_altitude));
	}
	if (ImGui::SliderFloat("sun_shafts_intensity", &cur->m_fSunShaftsIntensity, 0.0f, 2.0f))
		changed = true;
	sel = 0;
	for (int i = 0; i != env.m_thunderbolt_collections_config->sections().size(); i++)
		if (cur->tb_id == env.m_thunderbolt_collections_config->sections()[i]->Name)
			sel = i + 1;

	ImGui::Text(u8"Thunder bolt parameters");

	if (ImGui::Combo("thunderbolt_collection", &sel, enumIniWithEmpty, env.m_thunderbolt_collections_config,
		env.m_thunderbolt_collections_config->sections().size() + 1)) {
		cur->tb_id = (sel == 0)
			? env.eff_Thunderbolt->AppendDef(env, env.m_thunderbolt_collections_config, env.m_thunderbolts_config, "")
			: env.eff_Thunderbolt->AppendDef(env, env.m_thunderbolt_collections_config, env.m_thunderbolts_config,
				env.m_thunderbolt_collections_config->sections()[sel - 1]->Name.c_str());
		changed = true;
	}
	if (ImGui::SliderFloat("thunderbolt_duration", &cur->bolt_duration, 0.0f, 2.0f))
		changed = true;
	if (ImGui::SliderFloat("thunderbolt_period", &cur->bolt_period, 0.0f, 10.0f))
		changed = true;

	ImGui::Text(u8"Water parameters");

	if (ImGui::SliderFloat("water_intensity", &cur->m_fWaterIntensity, 0.0f, 2.0f))
		changed = true;

	ImGui::Text(u8"Wind parameters");

	if (ImGui::SliderFloat("wind_velocity", &cur->wind_velocity, 0.0f, 100.0f))
		changed = true;
	if (ImGui::SliderFloat("wind_direction", &cur->wind_direction, 0.0f, 360.0f))
		changed = true;

	ImGui::Text(u8"Trees parameters");

	if (ImGui::SliderFloat("trees_amplitude_intensity", &cur->m_fTreeAmplitudeIntensity, 0.01f, 0.250f))
		changed = true;

	if (changed)
		modifiedWeathers.insert(env.CurrentWeatherName);
	if (ImGui::Button("Save")) {
		for (auto name : modifiedWeathers)
			saveWeather(name, env.WeatherCycles[name]);
		modifiedWeathers.clear();
	}
    ImGui::End();
} 
