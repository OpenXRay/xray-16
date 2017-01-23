#pragma once


namespace UIMapWndActionsSpace {

	enum EWorldProperties {
		ePropTargetMapShown,
		ePropMapMinimized,
		ePropMapResized,
		ePropMapIdle,
		ePropMapCentered,
		ePropDummy					= u16(-1),
	};

	enum EWorldOperators {
		eOperatorMapResize,
		eOperatorMapMinimize,
		eOperatorMapIdle,
		eOperatorMapCenter,
		eWorldOperatorDummy			= u16(-1),
	};
};
