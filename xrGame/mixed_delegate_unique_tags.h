#ifndef MIXED_DELEGATE_UNIQUE_TAGS
#define MIXED_DELEGATE_UNIQUE_TAGS

enum enum_mixed_delegate_unique_tags
{
	mdut_no_unique_tag				= 0x00,	//in this case you can have c2084 compile error
	mdut_login_operation_cb_tag,
	account_operation_cb_tag,
	suggest_nicks_cb_tag,
	account_profiles_cb_tag,
	found_emails_cb_tag,
	store_operation_cb_tag
}; //enum enum_mixed_delegate_unique_tags

#endif //#ifndef MIXED_DELEGATE_UNIQUE_TAGS