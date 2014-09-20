#pragma once

#ifdef DEBUG

// Lain: added
namespace debug { class text_tree; }

class CLevelDebug {
public:

	// Lain: added
	debug::text_tree* m_p_texttree;
	int                  m_texttree_offs;


	template <typename T>
	class CItemBase {

		xr_vector<T>		m_data;	
		typedef typename xr_vector<T>::iterator		ITEM_STORAGE_VEC_IT;

		struct remove_text_pred {
			LPCSTR text;
			remove_text_pred(LPCSTR t) : text(t) {}
			bool operator () (const T &item) {
				return (item.text == text);
			}
		};

		struct remove_id_pred {
			u32 id;
			remove_id_pred(u32 i) : id(i) {}
			bool operator () (const T &item) {
				return (item.id == id);
			}
		};

		struct sort_id_pred {
			bool operator() (const T &item1, const T &item2) {
				return (item1.id < item2.id);
			}
		};
	
	public:
		IC	void	add_item		(T data) {
			m_data.push_back	(data);	
			std::sort			(m_data.begin(), m_data.end(), sort_id_pred());
		}

		IC	void	remove_item		(LPCSTR text) {
			ITEM_STORAGE_VEC_IT it = std::remove_if(m_data.begin(), m_data.end(), remove_text_pred(text));
			m_data.erase(it, m_data.end());

			std::sort(m_data.begin(), m_data.end(), sort_id_pred());
		}
		IC	void	remove_item		(u32 id) {
			ITEM_STORAGE_VEC_IT it = std::remove_if(m_data.begin(), m_data.end(), remove_id_pred(id));
			m_data.erase(it, m_data.end());

			std::sort(m_data.begin(), m_data.end(), sort_id_pred());
		}
		IC	void	clear			() {m_data.clear	();}

		template<class T>
		IC	void	process			(T &process_pred) {
			for (ITEM_STORAGE_VEC_IT it=m_data.begin(); it != m_data.end(); ++it) {
				process_pred(*it);
			}
		}
	};

	//////////////////////////////////////////////////////////////////////////

	struct SInfoItem {
		shared_str	text;
		u32			color;
		u32			id;

		SInfoItem	(LPCSTR str, u32 col, u32 i) : text(str), color(col), id(i) {}
	};
	
	class CObjectInfo : public CItemBase<SInfoItem> {

		typedef CItemBase<SInfoItem> inherited;
		
		#define	DELTA_HEIGHT_DEFAULT	16.f
		#define	SHIFT_POS_DEFAULT		Fvector().set(0.f,2.f,0.f)

		Fvector			m_shift_pos;
		float			m_delta_height;

	public: 

					CObjectInfo		() {setup();}

			void	add_item		(LPCSTR text, u32 color, u32 id = u32(-1));
			
			void	draw_info		(float x, float &y);
		IC	void	setup			(const Fvector &shift = SHIFT_POS_DEFAULT, float delta = DELTA_HEIGHT_DEFAULT) {m_shift_pos.set(shift); m_delta_height = delta;}

		IC	Fvector &get_shift_pos	() {return m_shift_pos;}
	};

	//////////////////////////////////////////////////////////////////////////

	struct STextItem {
		shared_str	text;
		
		float		x;
		float		y;

		u32			color;
		u32			id;

		STextItem	(LPCSTR str, float coord_x, float coord_y, u32 col, u32 i) : text(str), x(coord_x), y(coord_y), color(col), id(i) {}
	};

	class CTextInfo : public CItemBase<STextItem> {
		typedef CItemBase<STextItem> inherited;

	public: 
			void	add_item		(LPCSTR text, float x, float y, u32 color, u32 id = u32(-1));
			void	draw_text		();
	};

	//////////////////////////////////////////////////////////////////////////

	struct SLevelItem {
		Fvector		position1;
		Fvector		position2;
		float		radius;

		enum {
			ePoint	= u32(0),
			eLine,
			eBox
		} ptype;

		u32			color;
		u32			id;

		SLevelItem		(const Fvector &p, u32 col, u32 i) {
			set			(p, col, i);
			ptype		= ePoint;
		}
		
		SLevelItem		(const Fvector &p, const Fvector &p2, u32 col, u32 i) {
			set			(p, col, i);
			ptype		= eLine;
			position2	= p2;
		}

		SLevelItem		(const Fvector &p, float r, u32 col, u32 i) {
			set			(p, col, i);
			ptype		= eBox;
			radius		= r;
		}

		void	set		(const Fvector &p, u32 col, u32 i) {
			position1	= p;
			color		= col;
			id			= i;
		}
	};

	class CLevelInfo : public CItemBase<SLevelItem> {
		typedef CItemBase<SLevelItem> inherited;
	public:
		void	add_item		(const Fvector &pos, u32 color, u32 id = u32(-1));
		void	add_item		(const Fvector &pos1, const Fvector &pos2, u32 color, u32 id = u32(-1));
		void	add_item		(const Fvector &pos, float radius, u32 color, u32 id = u32(-1));
		void	draw_info		();
	};


public:
				CLevelDebug			();
				~CLevelDebug		();

	template<class T>
	CObjectInfo &object_info		(CObject *obj, T typed_class) {
		return object_info(obj, typeid((*typed_class)).name());	
	}

	template<class T>
	CObjectInfo &object_info		(T typed_class) {
		return object_info(typed_class, typeid((*typed_class)).name());	
	}

	template<class T>
	CTextInfo &text(T typed_class) {
		return text(typed_class, typeid((*typed_class)).name());	
	}

	template<class T>
	CLevelInfo &level_info(T typed_class) {
		return level_info(typed_class, typeid((*typed_class)).name());	
	}


	void		draw_object_info	();
	void		draw_text			();
	void		draw_level_info		();

	// Lain: added
	void        draw_debug_text     ();
	void        log_debug_info      ();
	void        debug_info_up       ();
	void        debug_info_down     ();

	debug::text_tree&  get_text_tree ();

	void		on_destroy_object	(CObject *obj);

private:
	void		free_mem			();
	
	CObjectInfo &object_info		(CObject *obj, LPCSTR class_name);
	CTextInfo	&text				(void *class_ptr, LPCSTR class_name);
	CLevelInfo	&level_info			(void *class_ptr, LPCSTR class_name);

private:
	
	struct SKey {
		void	*class_ptr;
		LPCSTR	class_name;

				SKey		(void *ptr, LPCSTR name) {class_ptr = ptr; class_name = name;}

		bool	operator <	(const SKey &val) const {
			return (class_ptr < val.class_ptr);
		}

	};

	DEFINE_MAP			(LPCSTR,	CObjectInfo*,	CLASS_INFO_MAP,		CLASS_INFO_MAP_IT);	
	DEFINE_MAP			(CObject*,	CLASS_INFO_MAP,	OBJECT_INFO_MAP,	OBJECT_INFO_MAP_IT);
	DEFINE_MAP			(SKey,		CTextInfo*,		TEXT_INFO_MAP,		TEXT_INFO_MAP_IT);
	DEFINE_MAP			(SKey,		CLevelInfo*,	LEVEL_INFO_MAP,		LEVEL_INFO_MAP_IT);
	
	OBJECT_INFO_MAP		m_objects_info;
	TEXT_INFO_MAP		m_text_info;
	LEVEL_INFO_MAP		m_level_info;
};

#endif
