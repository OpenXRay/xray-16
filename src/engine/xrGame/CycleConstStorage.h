#ifndef CYCLE_CONST_STORAGE_H
#define CYCLE_CONST_STORAGE_H

template<class T,int size> 
class CCycleConstStorage
{
	T array[size];
	int first;
	IC int position( int i ) const { return (first+i)%size; }
public:
	IC CCycleConstStorage()
	{
		first=0;
	}
	IC void fill_in(const T& val)
	{
		std::fill(array,array+size,val);
	}
	IC void push_back(T& val)
	{
		array[first]=val;
		first	=position( 1 );
	}
	IC T& operator [] (int i)
	{
		return array[ position( i ) ];
	}
	IC const T& operator [] (int i) const
	{
		return array[ position( i ) ];
	}
};
#endif