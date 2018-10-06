#pragma once

constexpr pcstr LoadingScreenXML =
"<w>\
	<background width=\"1024\" height=\"768\">\
        <auto_static width=\"1024\" height=\"768\">\
            <texture>ui_mm_loading_screen</texture>\
        </auto_static>\
	</background>\
    <loading_logo x=\"0\" y=\"173\" width=\"1024\" height=\"399\">\
    </loading_logo>\
    <loading_progress_background x=\"260\" y=\"599\" width=\"506\" height=\"4\">\
        <texture color=\"black\" a=\"255\">ui_mm_loading_progress_bar</texture>\
    </loading_progress_background>\
    <loading_progress x=\"260\" y=\"599\" width=\"506\" height=\"4\" horz=\"1\" min=\"0\" max=\"100\" pos=\"0\" inertion=\"5.0\">\
        <progress>\
            <texture>ui_mm_loading_progress_bar</texture>\
        </progress>\
        <min_color r=\"30\" g=\"88\" b=\"126\"/>\
		<middle_color r=\"60\" g=\"118\" b=\"156\"/>\
		<max_color r=\"90\" g=\"148\" b=\"186\"/>\
    </loading_progress>\
    <loading_progress_percent x=\"260\" y=\"574\" width=\"506\" height=\"20\">\
        <text align=\"c\" r=\"170\" g=\"170\" b=\"170\" a=\"0\" font=\"letterica18\"/>\
    </loading_progress_percent>\
    <loading_stage x=\"260\" y=\"530\" width=\"506\" height=\"20\">\
        <text align=\"c\" r=\"170\" g=\"170\" b=\"170\" font=\"letterica18\"/>\
    </loading_stage>\
    <loading_header x=\"260\" y=\"622\" width=\"506\" height=\"20\">\
        <text align=\"c\" r=\"103\" g=\"103\" b=\"103\" font=\"letterica18\"/>\
    </loading_header>\
    <loading_tip_number x=\"260\" y=\"658\" width=\"506\" height=\"20\">\
        <text align=\"c\" r=\"103\" g=\"103\" b=\"103\" font=\"letterica18\"/>\
    </loading_tip_number>\
    <loading_tip x=\"163\" y=\"676\" width=\"700\" height=\"80\" complex_mode=\"1\">\
        <text alignment=\"c\" align=\"c\" r=\"103\" g=\"103\" b=\"103\" font=\"letterica18\"/>\
    </loading_tip>\
</w>";

constexpr pcstr LoadingScreenXML16x9 =
"<w>\
    <background width=\"1024\" height=\"768\">\
        <auto_static x=\"102\" y=\"0\" width=\"819\" height=\"768\" stretch=\"1\">\
            <texture>ui_mm_loading_screen</texture>\
        </auto_static>\
        <auto_static x=\"0\" y=\"0\" width=\"104\" height=\"768\" stretch=\"1\">\
            <texture>ui_mm_loading_left_widepanel</texture>\
        </auto_static>\
        <auto_static x=\"920\" y=\"0\" width=\"104\" height=\"768\" stretch=\"1\">\
            <texture>ui_mm_loading_right_widepanel</texture>\
        </auto_static>\
    </background>\
    <loading_logo x=\"102\" y=\"173\" width=\"819\" height=\"512\" stretch=\"1\"/>\
    <loading_progress_background x=\"310\" y=\"599\" width=\"405\" height=\"5\" stretch=\"1\">\
        <texture width=\"405\" height=\"4\" color=\"black\" a=\"255\" stretch=\"1\">ui_mm_loading_progress_bar</texture>\
    </loading_progress_background>\
    <loading_progress x=\"310\" y=\"599\" width=\"405\" height=\"5\" horz=\"1\" min=\"0\" max=\"100\" pos=\"0\" inertion=\"5.0\">\
        <progress>\
            <texture>ui_mm_loading_progress_bar</texture>\
        </progress>\
        <min_color r=\"30\" g=\"88\" b=\"126\"/>\
		<middle_color r=\"60\" g=\"118\" b=\"156\"/>\
		<max_color r=\"90\" g=\"148\" b=\"186\"/>\
    </loading_progress>\
    <loading_progress_percent x=\"310\" y=\"574\" width=\"405\" height=\"20\" stretch=\"1\">\
        <text align=\"c\" r=\"170\" g=\"170\" b=\"170\" a=\"0\" font=\"letterica18\"/>\
    </loading_progress_percent>\
    <loading_stage x=\"310\" y=\"530\" width=\"405\" height=\"20\" stretch=\"1\">\
        <text align=\"c\" r=\"170\" g=\"170\" b=\"170\" font=\"letterica18\"/>\
    </loading_stage>\
    <loading_header x=\"310\" y=\"622\" width=\"405\" height=\"20\" stretch=\"1\">\
        <text align=\"c\" r=\"103\" g=\"103\" b=\"103\" font=\"letterica18\"/>\
    </loading_header>\
    <loading_tip_number x=\"310\" y=\"658\" width=\"405\" height=\"20\" stretch=\"1\">\
        <text align=\"c\" r=\"103\" g=\"103\" b=\"103\" font=\"letterica18\"/>\
    </loading_tip_number>\
    <loading_tip x=\"273\" y=\"676\" width=\"480\" height=\"80\" complex_mode=\"1\" stretch=\"1\">\
        <text alignment=\"c\" align=\"c\" r=\"103\" g=\"103\" b=\"103\" font=\"letterica18\"/>\
    </loading_tip>\
</w>";
