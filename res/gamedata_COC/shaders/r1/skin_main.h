#ifndef	SKIN_MAIN_INCLUDED
#define	SKIN_MAIN_INCLUDED

//////////////////////////////////////////////////////
#ifdef	SKIN_LQ
//////////////////////////////////////////////////////

#ifdef 	SKIN_NONE
SKIN_VF	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
SKIN_VF	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); 	}
#endif

#ifdef	SKIN_1
SKIN_VF	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); 	}
#endif

#ifdef	SKIN_2
SKIN_VF	main(v_model_skinned_2 v) 	{ return _main(skinning_2lq(v)); }
#endif

#ifdef	SKIN_3
SKIN_VF	main(v_model_skinned_3 v) 	{ return _main(skinning_3lq(v)); }
#endif

#ifdef	SKIN_4
SKIN_VF	main(v_model_skinned_4 v) 	{ return _main(skinning_4lq(v)); }
#endif

//////////////////////////////////////////////////////
#else	//	SKIN_LQ
//////////////////////////////////////////////////////

#ifdef 	SKIN_NONE
SKIN_VF	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
SKIN_VF	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); 	}
#endif

#ifdef	SKIN_1
SKIN_VF	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); 	}
#endif

#ifdef	SKIN_2
SKIN_VF	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); 	}
#endif

#ifdef	SKIN_3
SKIN_VF	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); 	}
#endif

#ifdef	SKIN_4
SKIN_VF	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); 	}
#endif

//////////////////////////////////////////////////////
#endif	//	SKIN_LQ
//////////////////////////////////////////////////////

#endif	//	SKIN_MAIN_INCLUDED