////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_manager_space.h
//	Created 	: 25.05.2004
//  Modified 	: 25.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Agent manager space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace AgentManager {
	enum EProperties {
		ePropertyOrders			= u32(0),
		ePropertyItem,
		ePropertyEnemy,
		ePropertyDanger,

		ePropertyScript,
		ePropertyDummy			= u32(-1),
	};

	enum EOperators {
		eOperatorNoOrders		= u32(0),
		eOperatorGatherItem,
		eOperatorKillEnemy,
		eOperatorReactOnDanger,

		eOperatorScript,
		eOperatorDummy			= u32(-1),
	};
};