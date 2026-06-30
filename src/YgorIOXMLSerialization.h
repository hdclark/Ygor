//YgorIOXMLSerialization.h - Lightweight named text serialization archives.

#pragma once

#include <array>
#include <cstddef>
#include <iomanip>
#include <istream>
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

namespace ygor {
namespace serialization {

template <class T>
struct nvp {
    const char *name;
    T &value;
};

template <class T>
nvp<T> make_nvp(const char *name, T &value){
    return { name, value };
}

class xml_oarchive {
public:
    explicit xml_oarchive(std::ostream &out) : out_(out) {}

    template <class T>
    xml_oarchive &operator&(nvp<T> v){
        save_named(v.name, v.value);
        return *this;
    }

    template <class T>
    xml_oarchive &operator<<(nvp<T> v){
        return (*this & v);
    }

    template <class T>
    void save_named(const char *name, T &value){
        out_ << std::quoted(std::string(name)) << ' ';
        save(value);
    }

    template <class T>
    void save(T &value){
        serialize(*this, value);
    }

private:
    std::ostream &out_;

    template <class T>
    friend void save_scalar(xml_oarchive &, T &);
    friend void save_scalar(xml_oarchive &, std::string &);
};

class xml_iarchive {
public:
    explicit xml_iarchive(std::istream &in) : in_(in) {}

    template <class T>
    xml_iarchive &operator&(nvp<T> v){
        load_named(v.value);
        return *this;
    }

    template <class T>
    xml_iarchive &operator>>(nvp<T> v){
        return (*this & v);
    }

    template <class T>
    void load_named(T &value){
        std::string unused_name;
        in_ >> std::quoted(unused_name);
        load(value);
    }

    template <class T>
    void load(T &value){
        serialize(*this, value);
    }

private:
    std::istream &in_;

    template <class T>
    friend void load_scalar(xml_iarchive &, T &);
    friend void load_scalar(xml_iarchive &, std::string &);
};

template <class T>
void save_scalar(xml_oarchive &a, T &value){
    a.out_ << std::setprecision(17) << value << '\n';
}

template <class T>
void load_scalar(xml_iarchive &a, T &value){
    a.in_ >> value;
}

inline void save_scalar(xml_oarchive &a, std::string &value){
    a.out_ << std::quoted(value) << '\n';
}

inline void load_scalar(xml_iarchive &a, std::string &value){
    a.in_ >> std::quoted(value);
}

template <class T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
serialize(xml_oarchive &a, T &value){
    save_scalar(a, value);
}

template <class T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
serialize(xml_iarchive &a, T &value){
    load_scalar(a, value);
}

inline void serialize(xml_oarchive &a, std::string &value){
    save_scalar(a, value);
}

inline void serialize(xml_iarchive &a, std::string &value){
    load_scalar(a, value);
}

template <class T>
void serialize(xml_oarchive &a, std::vector<T> &value){
    auto size = value.size();
    a.save_named("size", size);
    for(auto &v : value) a.save_named("item", v);
}

template <class T>
void serialize(xml_iarchive &a, std::vector<T> &value){
    size_t size = 0;
    a.load_named(size);
    value.clear();
    value.resize(size);
    for(auto &v : value) a.load_named(v);
}

template <class T>
void serialize(xml_oarchive &a, std::list<T> &value){
    auto size = value.size();
    a.save_named("size", size);
    for(auto &v : value) a.save_named("item", v);
}

template <class T>
void serialize(xml_iarchive &a, std::list<T> &value){
    size_t size = 0;
    a.load_named(size);
    value.clear();
    for(size_t i = 0; i < size; ++i){
        T v;
        a.load_named(v);
        value.push_back(v);
    }
}

template <class T, size_t N>
void serialize(xml_oarchive &a, std::array<T,N> &value){
    auto size = value.size();
    a.save_named("size", size);
    for(auto &v : value) a.save_named("item", v);
}

template <class T, size_t N>
void serialize(xml_iarchive &a, std::array<T,N> &value){
    size_t size = 0;
    a.load_named(size);
    for(size_t i = 0; i < N; ++i) a.load_named(value[i]);
}

template <class K, class V>
void serialize(xml_oarchive &a, std::map<K,V> &value){
    auto size = value.size();
    a.save_named("size", size);
    for(auto &kv : value){
        auto key = kv.first;
        a.save_named("key", key);
        a.save_named("value", kv.second);
    }
}

template <class K, class V>
void serialize(xml_iarchive &a, std::map<K,V> &value){
    size_t size = 0;
    a.load_named(size);
    value.clear();
    for(size_t i = 0; i < size; ++i){
        K key;
        V mapped;
        a.load_named(key);
        a.load_named(mapped);
        value.emplace(std::move(key), std::move(mapped));
    }
}

} // namespace serialization
} // namespace ygor
