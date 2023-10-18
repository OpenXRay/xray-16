#pragma once

pcstr GetLoadingScreenXML();
pcstr GetLoadingScreenTexturesDescr();

constexpr pcstr LoadingScreenXML =
R"(<w>
	<background width="1024" height="768">
        <auto_static width="1024" height="768">
            <texture>ui_mm_loading_screen</texture>
        </auto_static>
	</background>
    <loading_logo x="0" y="173" width="1024" height="399">
    </loading_logo>
    <loading_progress x="260" y="599" width="506" height="4" horz="1" min="0" max="100" pos="0" inertion="5.0">
        <progress>
            <texture>ui_mm_loading_progress_bar</texture>
        </progress>
        <background stretch="1">
            <texture width="506" height="4" r="0" g="0" b="0" a="255">ui_mm_loading_progress_bar</texture>
        </background>
    </loading_progress>
    <loading_stage x="260" y="530" width="506" height="20">
        <text align="c" r="170" g="170" b="170" font="letterica18"/>
    </loading_stage>
    <loading_header x="260" y="622" width="506" height="20">
        <text align="c" r="103" g="103" b="103" font="letterica18"/>
    </loading_header>
    <loading_tip_number x="260" y="658" width="506" height="20">
        <text align="c" r="103" g="103" b="103" font="letterica18"/>
    </loading_tip_number>
    <loading_tip x="163" y="676" width="700" height="80" complex_mode="1">
        <text alignment="c" align="c" r="103" g="103" b="103" font="letterica18"/>
    </loading_tip>
</w>)";

constexpr pcstr LoadingScreenXML16x9 =
R"(<w>
    <background width="1024" height="768">
        <auto_static x="102" y="0" width="819" height="768" stretch="1">
            <texture>ui_mm_loading_screen</texture>
        </auto_static>
        <auto_static x="0" y="0" width="104" height="768" stretch="1">
            <texture>ui_mm_loading_left_widepanel</texture>
        </auto_static>
        <auto_static x="920" y="0" width="104" height="768" stretch="1">
            <texture>ui_mm_loading_right_widepanel</texture>
        </auto_static>
    </background>
    <loading_logo x="102" y="173" width="819" height="512" stretch="1"/>
    <loading_progress x="310" y="599" width="405" height="5" horz="1" min="0" max="100" pos="0" inertion="5.0">
        <progress>
            <texture>ui_mm_loading_progress_bar</texture>
        </progress>
        <background stretch="1">
            <texture width="405" height="5" r="0" g="0" b="0" a="255">ui_mm_loading_progress_bar</texture>
        </background>
    </loading_progress>
    <loading_stage x="310" y="530" width="405" height="20" stretch="1">
        <text align="c" r="170" g="170" b="170" font="letterica18"/>
    </loading_stage>
    <loading_header x="310" y="622" width="405" height="20" stretch="1">
        <text align="c" r="103" g="103" b="103" font="letterica18"/>
    </loading_header>
    <loading_tip_number x="310" y="658" width="405" height="20" stretch="1">
        <text align="c" r="103" g="103" b="103" font="letterica18"/>
    </loading_tip_number>
    <loading_tip x="273" y="676" width="480" height="80" complex_mode="1" stretch="1">
        <text alignment="c" align="c" r="103" g="103" b="103" font="letterica18"/>
    </loading_tip>
</w>)";

constexpr pcstr LoadingScreenXMLClearSky =
R"(<w>
	<background width="1024" height="768">
        <auto_static width="1024" height="768">
            <texture>ui_mm_loading_screen</texture>
        </auto_static>
	</background>
    <background_rus width="1024" height="768">
        <auto_static width="1024" height="768">
            <texture>ui_mm_loading_screen_ru</texture>
        </auto_static>
    </background_rus>
    <background_eng width="1024" height="768">
        <auto_static width="1024" height="768">
            <texture>ui_mm_loading_screen_en</texture>
        </auto_static>
    </background_eng>
    <loading_logo x="257" y="369" width="512" height="256" stretch="1"/>
    <loading_progress x="379" y="726" width="268" height="37" mode="from_center" min="0" max="100" pos="0" inertion="5.0" under_background="0">
        <progress>
            <texture>ui_mm_loading_progress_bar</texture>
        </progress>
    </loading_progress>
    <loading_stage x="310" y="698" width="405" height="20" stretch="1" always_show="1">
        <text align="c" r="157" g="140" b="120" font="graffiti22"/>
    </loading_stage>
</w>)";

constexpr pcstr LoadingScreenXML16x9ClearSky =
R"(<w>
    <background width="1024" height="768">
        <auto_static x="102" y="0" width="819" height="768" stretch="1">
            <texture>ui_mm_loading_screen</texture>
        </auto_static>
        <auto_static x="0" y="0" width="104" height="768" stretch="1">
            <texture>ui_mm_loading_left_widepanel</texture>
        </auto_static>
        <auto_static x="920" y="0" width="104" height="768" stretch="1">
            <texture>ui_mm_loading_right_widepanel</texture>
        </auto_static>
    </background>
    <!--background_rus width="1024" height="768">
        <auto_static x="0" y="0" width="1024" height="768">
            <texture>ui_mm_loading_screen_ru</texture>
        </auto_static>
    </background_rus>
    <background_eng width="1024" height="768">
        <auto_static x="0" y="0" width="1024" height="768">
            <texture>ui_mm_loading_screen_en</texture>
        </auto_static>
    </background_eng-->
    <loading_logo x="307" y="368.3" width="409.5" height="256" stretch="1"/>
    <!--loading_logo_rus x="256" y="368.3" width="614.5" height="256"/>
    <loading_logo_eng x="256" y="368.3" width="614.5" height="256"/-->
    <loading_progress x="379" y="726" width="268" height="37" mode="from_center" min="0" max="100" pos="0" inertion="5.0" under_background="0">
        <progress>
            <texture>ui_mm_loading_progress_bar</texture>
        </progress>
    </loading_progress>
    <loading_stage x="310" y="698" width="405" height="20" stretch="1" always_show="1">
        <text align="c" r="157" g="140" b="120" font="graffiti22"/>
    </loading_stage>
</w>)";

constexpr pcstr LoadingScreenXML16x9ShadowOfChernobyl =
R"(<w>
    <background width="1024" height="768">
        <auto_static x="0" y="0" width="1024" height="768" stretch="1">
            <texture>ui_mm_loading_screen</texture>
        </auto_static>
    </background>
    <loading_logo x="257" y="369" width="512" height="256" stretch="1"/>
    <loading_progress x="379" y="726" width="268" height="37" mode="from_center" min="0" max="100" pos="0" inertion="5.0" under_background="0">
        <progress>
            <texture>ui_mm_loading_progress_bar</texture>
        </progress>
    </loading_progress>
    <loading_stage x="310" y="698" width="405" height="20" stretch="1" always_show="1">
        <text align="c" r="157" g="140" b="120" font="graffiti22"/>
    </loading_stage>
</w>)";

constexpr cpcstr LoadingScreenXMLTexturesDescription =
R"(<w>
    <file name="ui\ui_actor_loadgame_screen">
        <texture id="ui_mm_loading_screen" x="0" y="0" width="1024" height="768" />
        <texture id="ui_mm_loading_progress_bar" x="0" y="772" width="506" height="4" />
    </file>
    <file name="ui\ui_actor_widescreen_sidepanels">
        <texture id="ui_mm_loading_left_widepanel" x="0" y="0" width="128" height="768" />
        <texture id="ui_mm_loading_right_widepanel" x="128" y="0" width="128" height="768" />
   </file>
</w>)";

// XXX: maybe apply Russian and English backgrounds (though, they don't have left and right bars for auto_statics)
constexpr cpcstr LoadingScreenXMLClearSkyTexturesDescription =
R"(<w>
    <file name="ui\ui_load">
        <texture id="ui_mm_loading_screen" x="0" y="0" width="1024" height="768" />
        <texture id="ui_mm_loading_progress_bar" x="0" y="768" width="268" height="37" />
    </file>
    <file name="ui\ui_load_ru">
        <texture id="ui_mm_loading_screen_ru" x="0" y="0" width="1024" height="768" />
    </file>
    <file name="ui\ui_load_en">
        <texture id="ui_mm_loading_screen_en" x="0" y="0" width="1024" height="768" />
    </file>
    <file name="ui\ui_ingame2_back_add2_w">
        <texture id="ui_mm_loading_left_widepanel" x="680" y="0" width="170" height="768" />
        <texture id="ui_mm_loading_right_widepanel" x="850" y="0" width="170" height="768" />
   </file>
</w>)";

inline pcstr GetLoadingScreenXML()
{
    const bool is_wide = UICore::is_widescreen();
    if (ShadowOfChernobylMode)
    {
        if (is_wide)
            return LoadingScreenXML16x9ShadowOfChernobyl;
        return LoadingScreenXMLClearSky;
    }
    if (ClearSkyMode)
    {
        if (is_wide)
            return LoadingScreenXML16x9ClearSky;
        return LoadingScreenXMLClearSky;
    }
    // Call of Pripyat
    if (is_wide)
        return LoadingScreenXML16x9;
    return LoadingScreenXML;
}

inline pcstr GetLoadingScreenTexturesDescr()
{
    if (ShadowOfChernobylMode)
    {
        return LoadingScreenXMLClearSkyTexturesDescription;
    }
    if (ClearSkyMode)
    {
        return LoadingScreenXMLClearSkyTexturesDescription;
    }
    // Call of Pripyat
    return LoadingScreenXMLTexturesDescription;
}
