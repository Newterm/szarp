#ifndef __UTILS_ITERATOR_H__
#define __UTILS_ITERATOR_H__

template<class Map>
class key_iterator
{
public:
    key_iterator( const key_iterator& itr ) : itr(itr.itr) {}
    key_iterator( const typename Map::const_iterator& itr ) : itr(itr) {}
    typename Map::key_type* operator->()    { return &itr->first;    }
    const typename Map::key_type& operator* () const    { return itr->first;     } 
    key_iterator&  operator++()    { ++itr; return *this;    }
    key_iterator   operator++(int) { return key_iterator(itr++); }
    bool operator==( const key_iterator& other ) { return itr == other.itr; }
    bool operator!=( const key_iterator& other ) { return itr != other.itr; }
protected:
    typename Map::const_iterator itr;
};

#endif /* __UTILS_ITERATOR_H__ */

