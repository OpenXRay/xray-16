#ifndef PH_ITEM_LIST_H
#define PH_ITEM_LIST_H
/*
#define DECLARE_PHLIST_ITEM(class_name)			public:\
												class CPHListItem\
												{\
													friend class CPHItemList<class_name>;\
													friend class CPHItemList<class_name>::iterator;\
													class_name* next;\
													class_name** tome;\
												};\
												private:
*/
#define DECLARE_PHLIST_ITEM(class_name)			friend class CPHItemList<class_name>;\
												friend class CPHItemList<class_name>::iterator;\
												class_name* next;\
												class_name** tome;
#define DECLARE_PHSTACK_ITEM(class_name)		DECLARE_PHLIST_ITEM(class_name)\
												friend class CPHItemStack<class_name>;\
												u16 stack_pos;
						
//#define TPI(item)								((T::CPHListItem*)item)	

template<class T>
		class CPHItemList
		{
			T				*first_next							;
			T			   **last_tome							;
		protected:
			u16				size								;
	
		public:
		
		class iterator;
		typedef  class iterator
			{
				
						T		*my_ptr;
					public:
								iterator(){my_ptr=0;}
								iterator(T* i){ my_ptr=i; }
					iterator	operator ++ ()	{return my_ptr=((my_ptr)->next);}
					T*			operator *	()	{return	my_ptr;}
					bool		operator !=	 (iterator right){return my_ptr!=right.my_ptr;}
			};
							CPHItemList		()		{  empty();}
			u16				count			()		{return size;}
			void			push_back		(T* item)	
			{
				*(last_tome)=item;
				item->tome= last_tome;
				last_tome=&((item)->next);
				item->next=0;
				size++;
			}
			void			move_items		(CPHItemList<T>& sourse_list)
			{
				if(!sourse_list.first_next) return;
				*(last_tome)=sourse_list.first_next;
				sourse_list.first_next->tome= last_tome;
				last_tome=sourse_list.last_tome;
				size=size+sourse_list.size;
				sourse_list.empty();
			}
			void			erase			(iterator i)
			{
				T* item=*i;
				T* next=item->next;
				*(item->tome)=next;
				if(next)next->tome=item->tome;
				else last_tome=item->tome;
				size--;
			}
			void			empty			()
			{
				last_tome=&first_next;
				first_next=0;
				size=0;
			}
			iterator			begin()
			{
				return iterator(first_next);
			}
			iterator			end()
			{
				return iterator(0);
			}
	};

template<class T>
	class CPHItemStack : 
		public CPHItemList<T>
	{
	public:	
		void			push_back		(T* item)	
		{
			item->stack_pos=size;
			CPHItemList<T>::push_back(item);
		}
	};
#define DEFINE_PHITEM_LIST(T,N,I)		typedef CPHItemList<T>	N; typedef CPHItemList<T>::iterator I;
#define DEFINE_PHITEM_STACK(T,N,I)		typedef CPHItemStack<T>	N; typedef CPHItemStack<T>::iterator I;
#endif
