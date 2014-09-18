template<typename TypeListElement>
void game_state_accumulator::init_acpv_list()
{
	STATIC_CHECK(Loki::TL::is_Typelist<TypeListElement>::value,
		Type_Must_Have_a_Loki_Type_List_type_use__ADD_ACCUMULATIVE_STATE__macro_define);
	
	init_acpv_list<TypeListElement::Tail>();

	player_state_param*	tmp_obj_inst = xr_new<typename TypeListElement::Head::value_type>(this);
	
	m_accumulative_values.insert(
		std::make_pair(
			TypeListElement::Head::value_id,
			tmp_obj_inst
		)
	);
}