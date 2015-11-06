//YgorContainers.cc.
#include <string>
#include <algorithm>    //Needed for std::sort(..).
#include "YgorContainers.h"

//#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
//    #define YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
//#endif

//----------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------ internal template helpers -------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//These are helper templates used for type deduction for use *within this file.*
//
//How they work: we define a base struct for each data type we want to use. We make the default template contain a false bool.
// Then, we define a template specialization *specifically* for that type that has a true bool. Therefore, inspection of the 
// bool value will tell use whether or not the type is what we asked for. 
//template<class T> struct IS_THIS_OF_TYPE_LONG_INT {     static const bool value = false;   };
//template<>        struct IS_THIS_OF_TYPE_LONG_INT<long int> {  static const bool value = true; };
//
//template<class T> struct IS_THIS_OF_TYPE_STD_STRING {     static const bool value = false;   };
//template<>        struct IS_THIS_OF_TYPE_STD_STRING<std::string> {  static const bool value = true; };
//THESE ARE NOT USED - They are here for reference!
// Use like:
// if( IS_THIS_OF_TYPE_LONG_INT<T>::value ) 
// or
// if( IS_THIS_OF_TYPE_LONG_INT< decltype(rhs) >::value )

//---- Are two types equal? ----   (ala boost::is_same(..)  and adapted from
//  http://stackoverflow.com/questions/1708867/check-type-of-element-in-stl-container-c .
template<class UA, class UB> struct ARE_THESE_TYPES_EQUAL      { static const bool value = false; };
//Specialization for the case when UA == UB.
template<class U>            struct ARE_THESE_TYPES_EQUAL<U,U> { static const bool value = true;  };



//----------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------- bimap -------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------- Constructors ---------------------------------------------------------

template <class TA, class TB>  bimap<TA,TB>::bimap() { }

#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bimap<long int, std::string>::bimap();
    template bimap<std::string, long int>::bimap();

    template bimap<float, std::string>::bimap();
    template bimap<std::string, float>::bimap();
#endif


//--------------------------------------------------- Member functions -------------------------------------------------------
template <class TA, class TB>  TA & bimap<TA,TB>::operator[](const TB &in){
    //First, we have to check if the element exists yet. If it does, we return a reference to the corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).second == in) return ((*iter).first);
    }
    //If the element did not exist, we create a new element and push out a reference to the corresponding element.
    (*this).the_pairs.push_back( std::pair<TA,TB>( TA(), in ) ); //Default instantiation.
    return ((*(--((*this).the_pairs.end()))).first); //Return a reference to the last element.
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template long int &  bimap<long int, std::string>::operator[](const std::string &in);
    template std::string &  bimap<std::string, long int>::operator[](const long int &in);

    template float &  bimap<float, std::string>::operator[](const std::string &in);
    template std::string &  bimap<std::string, float>::operator[](const float &in);
#endif


//This is a mirror reflection of the previous operator. We need to ensure that both functions are not instantiated
// when TA == TB.
template <class TA, class TB>  TB & bimap<TA,TB>::operator[](const TA &in){
    //First, we have to check if the element exists yet. If it does, we return a reference to the corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).first == in) return ((*iter).second);
    }
    //If the element did not exist, we create a new element and push out a reference to the corresponding element.
    (*this).the_pairs.push_back( std::pair<TA,TB>( in, TB() ) ); //Default instantiation.
    return ((*(--((*this).the_pairs.end()))).second); //Return a reference to the last element.
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::string &  bimap<long int, std::string>::operator[](const long int &in);
    template long int &  bimap<std::string, long int>::operator[](const std::string &in);

    template std::string &  bimap<float, std::string>::operator[](const float &in);
    template float &  bimap<std::string, float>::operator[](const std::string &in);
#endif


template <class TA, class TB>    bimap<TA,TB> & bimap<TA,TB>::operator=(const std::map<TA,TB> &rhs) {
    this->the_pairs.clear();
    for(auto it = rhs.begin(); it != rhs.end(); ++it){
        (*this)[it->first] = it->second;
    }
    return *this;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bimap<long int, std::string> &  bimap<long int, std::string>::operator=(const std::map<long int, std::string> &);
    template bimap<std::string, long int> &  bimap<std::string, long int>::operator=(const std::map<std::string, long int> &);

    template bimap<float, std::string> &  bimap<float, std::string>::operator=(const std::map<float, std::string> &);
    template bimap<std::string, float> &  bimap<std::string, float>::operator=(const std::map<std::string, float> &);
#endif

template <class TA, class TB>    bimap<TA,TB> & bimap<TA,TB>::operator=(const std::map<TB,TA> &rhs) {
    this->the_pairs.clear();
    for(auto it = rhs.begin(); it != rhs.end(); ++it){
        (*this)[it->first] = it->second;
    }
    return *this;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bimap<long int, std::string> &  bimap<long int, std::string>::operator=(const std::map<std::string, long int> &);
    template bimap<std::string, long int> &  bimap<std::string, long int>::operator=(const std::map<long int, std::string> &);

    template bimap<float, std::string> &  bimap<float, std::string>::operator=(const std::map<std::string, float> &);
    template bimap<std::string, float> &  bimap<std::string, float>::operator=(const std::map<float, std::string> &);
#endif


template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::begin(void) const {
    return (*this).the_pairs.begin();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<long int,std::string> >::const_iterator   bimap<long int,std::string>::begin(void) const;
    template  std::vector<std::pair<std::string,long int> >::const_iterator   bimap<std::string,long int>::begin(void) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator   bimap<float,std::string>::begin(void) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator   bimap<std::string,float>::begin(void) const;
#endif

template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_reverse_iterator  bimap<TA,TB>::rbegin(void) const {
    return (*this).the_pairs.rbegin();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<long int,std::string> >::const_reverse_iterator   bimap<long int,std::string>::rbegin(void) const;
    template  std::vector<std::pair<std::string,long int> >::const_reverse_iterator   bimap<std::string,long int>::rbegin(void) const;

    template  std::vector<std::pair<float,std::string> >::const_reverse_iterator   bimap<float,std::string>::rbegin(void) const;
    template  std::vector<std::pair<std::string,float> >::const_reverse_iterator   bimap<std::string,float>::rbegin(void) const;
#endif


template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::end(void) const {
    return (*this).the_pairs.end();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<long int,std::string> >::const_iterator   bimap<long int,std::string>::end(void) const;
    template  std::vector<std::pair<std::string,long int> >::const_iterator   bimap<std::string,long int>::end(void) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator   bimap<float,std::string>::end(void) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator   bimap<std::string,float>::end(void) const;
#endif

template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_reverse_iterator  bimap<TA,TB>::rend(void) const {
    return (*this).the_pairs.rend();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<long int,std::string> >::const_reverse_iterator   bimap<long int,std::string>::rend(void) const;
    template  std::vector<std::pair<std::string,long int> >::const_reverse_iterator   bimap<std::string,long int>::rend(void) const;

    template  std::vector<std::pair<float,std::string> >::const_reverse_iterator   bimap<float,std::string>::rend(void) const;
    template  std::vector<std::pair<std::string,float> >::const_reverse_iterator   bimap<std::string,float>::rend(void) const;
#endif

template <class TA, class TB>   typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::find( const TA &in ) const {
    //We simply run through the elements to see if there is a corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).first == in) return iter;
    }
    return (*this).the_pairs.end();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<long int,std::string> >::const_iterator bimap<long int,std::string>::find( const long int & ) const;
    template  std::vector<std::pair<std::string,long int> >::const_iterator bimap<std::string,long int>::find( const std::string & ) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator bimap<float,std::string>::find( const float & ) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator bimap<std::string,float>::find( const std::string & ) const;
#endif


template <class TA, class TB>   typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::find( const TB &in ) const {
    //We simply run through the elements to see if there is a corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).second == in) return iter;
    }
    return (*this).the_pairs.end();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<long int,std::string> >::const_iterator bimap<long int,std::string>::find( const std::string & ) const;
    template  std::vector<std::pair<std::string,long int> >::const_iterator bimap<std::string,long int>::find( const long int & ) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator bimap<float,std::string>::find( const std::string & ) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator bimap<std::string,float>::find( const float & ) const;
#endif



template <class TA, class TB> template <class T>  typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::after( const T &in ) const {
    //First, we determine if the element passed in exists.
    auto it = (*this).find(in);
    if( it == (*this).end() ) return it; //Return the not-found signal.
    //If the element did exist, we iterate it appropriately.
    if( (++it) == (*this).end() ) return (*this).begin();  //Cyclic treatment!
    return it;
} 
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::vector< std::pair<long int,std::string> >::const_iterator bimap<long int,std::string>::after( const std::string & ) const;
    template std::vector< std::pair<long int,std::string> >::const_iterator bimap<long int,std::string>::after( const long int & ) const;
    template std::vector< std::pair<std::string,long int> >::const_iterator bimap<std::string,long int>::after( const std::string & ) const;
    template std::vector< std::pair<std::string,long int> >::const_iterator bimap<std::string,long int>::after( const long int & ) const;

    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::after( const std::string & ) const;
    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::after( const float & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::after( const std::string & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::after( const float & ) const;
#endif

 

template <class TA, class TB> template <class T>  typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::before( const T &in ) const {
    //First, we determine if the element passed in exists.
    auto it = (*this).find(in);
    if( it == (*this).end() ) return it; //Return the not-found signal.
    //If the element did exist, we iterate it appropriately.
    if( it == (*this).begin() ) return --((*this).end());
    return (--it);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::vector< std::pair<long int,std::string> >::const_iterator bimap<long int,std::string>::before( const std::string & ) const;
    template std::vector< std::pair<long int,std::string> >::const_iterator bimap<long int,std::string>::before( const long int & ) const;
    template std::vector< std::pair<std::string,long int> >::const_iterator bimap<std::string,long int>::before( const std::string & ) const;
    template std::vector< std::pair<std::string,long int> >::const_iterator bimap<std::string,long int>::before( const long int & ) const;

    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::before( const std::string & ) const;
    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::before( const float & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::before( const std::string & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::before( const float & ) const;
#endif


template <class TA, class TB> template <class T> T bimap<TA,TB>::get_next( const T &in ) const {
    //First, we take the given value and attempt to get an iterator to the element after it (cyclically.)
    auto it = this->after(in);
    if( it == this->end() ) return in;  //Simply return the value given.    Maybe we should throw an exception instead??

    //Now some tricky footwork. To keep this function general, we cannot indicate the .first or .second parts explicitly.
    // Instead, we use type checking. Do not be frightened by the casts - the types are required to be identical!
    if( ARE_THESE_TYPES_EQUAL<TA,T>::value ){
        return *(reinterpret_cast<const T*>( &(it->first) ));
    }else if( ARE_THESE_TYPES_EQUAL<TB,T>::value ){
        return *(reinterpret_cast<const T*>( &(it->second) ));  
    }
    FUNCERR("Attempted to perform impossible action. Was a wrong type passed to this bimap?");
    return in;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::string bimap<long int,std::string>::get_next( const std::string & ) const;
    template long int    bimap<long int,std::string>::get_next( const long int & ) const;
    template std::string bimap<std::string,long int>::get_next( const std::string & ) const;
    template long int    bimap<std::string,long int>::get_next( const long int & ) const;

    template std::string bimap<float,std::string>::get_next( const std::string & ) const;
    template float       bimap<float,std::string>::get_next( const float & ) const;
    template std::string bimap<std::string,float>::get_next( const std::string & ) const;
    template float       bimap<std::string,float>::get_next( const float & ) const;
#endif


template <class TA, class TB> template <class T> T bimap<TA,TB>::get_previous( const T &in ) const {
    //First, we take the given value and attempt to get an iterator to the element after it (cyclically.)
    auto it = this->before(in);
    if( it == this->end() ) return in;  //Simply return the value given.    Maybe we should throw an exception instead??

    //Now some tricky footwork. To keep this function general, we cannot indicate the .first or .second parts explicitly.
    // Instead, we use type checking. Do not be frightened by the casts - the types are required to be identical!
    if( ARE_THESE_TYPES_EQUAL<TA,T>::value ){
        return *(reinterpret_cast<const T*>( &((*it).first) ));
    }else if( ARE_THESE_TYPES_EQUAL<TB,T>::value ){
        return *(reinterpret_cast<const T*>( &((*it).second) ));
    }
    FUNCERR("Attempted to perform impossible action. Was a wrong type passed to this bimap?");
    return in;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::string bimap<long int,std::string>::get_previous( const std::string & ) const;
    template long int bimap<long int,std::string>::get_previous( const long int & ) const;
    template std::string bimap<std::string,long int>::get_previous( const std::string & ) const;
    template long int bimap<std::string,long int>::get_previous( const long int & ) const;

    template std::string bimap<float,std::string>::get_previous( const std::string & ) const;
    template float bimap<float,std::string>::get_previous( const float & ) const;
    template std::string bimap<std::string,float>::get_previous( const std::string & ) const;
    template float bimap<std::string,float>::get_previous( const float & ) const;
#endif


template <class TA, class TB>  void bimap<TA,TB>::order_on_first(void){
    auto lessthan = [](const std::pair<TA,TB> &A, const std::pair<TA,TB> &B) -> bool {
        return A.first < B.first;
    };
    std::sort(this->the_pairs.begin(), this->the_pairs.end(), lessthan);
    return;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template void bimap<long int,std::string>::order_on_first(void);
    template void bimap<std::string,long int>::order_on_first(void);

    template void bimap<float,std::string>::order_on_first(void);
    template void bimap<std::string,float>::order_on_first(void);
#endif

template <class TA, class TB>  void bimap<TA,TB>::order_on_second(void){
    auto lessthan = [](const std::pair<TA,TB> &A, const std::pair<TA,TB> &B) -> bool {
        return A.second < B.second;
    };
    std::sort(this->the_pairs.begin(), this->the_pairs.end(), lessthan);
    return;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template void bimap<long int,std::string>::order_on_second(void);
    template void bimap<std::string,long int>::order_on_second(void);

    template void bimap<float,std::string>::order_on_second(void);
    template void bimap<std::string,float>::order_on_second(void);
#endif

