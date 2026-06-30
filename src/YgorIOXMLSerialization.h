//YgorIOXMLSerialization.h - Lightweight named text serialization archives.

#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <istream>
#include <limits>
#include <locale>
#include <list>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace ygor {
namespace serialization {

namespace detail {

class ostream_token_format_guard {
public:
    explicit ostream_token_format_guard(std::ostream &out)
        : out_(out), flags_(out.flags()), precision_(out.precision()), fill_(out.fill()), locale_(out.getloc()) {
        out_.imbue(std::locale::classic());
        out_.setf(std::ios::dec, std::ios::basefield);
        out_.setf(std::ios::fmtflags(0), std::ios::floatfield);
        out_.unsetf(std::ios::boolalpha);
    }

    ~ostream_token_format_guard(){
        out_.flags(flags_);
        out_.precision(precision_);
        out_.fill(fill_);
        out_.imbue(locale_);
    }

private:
    std::ostream &out_;
    std::ios::fmtflags flags_;
    std::streamsize precision_;
    char fill_;
    std::locale locale_;
};

class istream_token_format_guard {
public:
    explicit istream_token_format_guard(std::istream &in)
        : in_(in), flags_(in.flags()), locale_(in.getloc()) {
        in_.imbue(std::locale::classic());
        in_.setf(std::ios::dec, std::ios::basefield);
        in_.unsetf(std::ios::boolalpha);
    }

    ~istream_token_format_guard(){
        in_.flags(flags_);
        in_.imbue(locale_);
    }

private:
    std::istream &in_;
    std::ios::fmtflags flags_;
    std::locale locale_;
};

} // namespace detail

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
    void save_named(const char *name, const T &value){
        detail::ostream_token_format_guard guard(out_);
        out_ << std::quoted(std::string(name)) << ' ';
        save(value);
        if(!out_) throw std::runtime_error("Failed to write serialized archive field.");
    }

    template <class T>
    void save(const T &value){
        auto &serializable_value = const_cast<std::remove_const_t<T> &>(value);
        serialize(*this, serializable_value);
    }

private:
    std::ostream &out_;

    template <class T> friend std::enable_if_t<std::is_floating_point<T>::value, void> save_scalar(xml_oarchive &, const T &);
    template <class T> friend std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, void> save_scalar(xml_oarchive &, const T &);
    friend void save_scalar(xml_oarchive &, const std::string &);
    friend void save_scalar(xml_oarchive &, const bool &);
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
        detail::istream_token_format_guard guard(in_);
        std::string unused_name;
        in_ >> std::quoted(unused_name);
        if(!in_) throw std::runtime_error("Failed to read serialized archive field name.");
        load(value);
        if(!in_) throw std::runtime_error("Failed to read serialized archive field value.");
    }

    template <class T>
    void load(T &value){
        serialize(*this, value);
    }

private:
    std::istream &in_;

    template <class T> friend std::enable_if_t<std::is_floating_point<T>::value, void> load_scalar(xml_iarchive &, T &);
    template <class T> friend std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, void> load_scalar(xml_iarchive &, T &);
    friend void load_scalar(xml_iarchive &, std::string &);
    friend void load_scalar(xml_iarchive &, bool &);
};

namespace detail {

class floating_point_text_facet : public std::locale::facet {
public:
    template <class T>
    void put(std::ostream &out, T value) const {
        static_assert(std::is_floating_point<T>::value, "floating-point type required");
        if(std::isnan(value)){
            out << (std::signbit(value) ? "-nan" : "nan");
        }else if(std::isinf(value)){
            out << (std::signbit(value) ? "-inf" : "inf");
        }else{
            out << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
        }
    }

    template <class T>
    void get(std::istream &in, T &value) const {
        static_assert(std::is_floating_point<T>::value, "floating-point type required");
        std::string token;
        in >> token;
        if(!in) return;

        if(token == "inf" || token == "+inf" || token == "infinity" || token == "+infinity"){
            value = std::numeric_limits<T>::infinity();
            return;
        }
        if(token == "-inf" || token == "-infinity"){
            value = -std::numeric_limits<T>::infinity();
            return;
        }
        if(token == "nan" || token == "+nan"){
            value = std::numeric_limits<T>::quiet_NaN();
            return;
        }
        if(token == "-nan"){
            value = -std::numeric_limits<T>::quiet_NaN();
            return;
        }

        std::istringstream ss(token);
        ss.imbue(std::locale::classic());
        ss >> value;
        if(!ss || !ss.eof()) in.setstate(std::ios::failbit);
    }
};

inline const floating_point_text_facet &floating_point_facet(){
    static const floating_point_text_facet facet;
    return facet;
}

} // namespace detail

template <class T>
std::enable_if_t<std::is_floating_point<T>::value, void>
save_scalar(xml_oarchive &a, const T &value){
    detail::floating_point_facet().put(a.out_, value);
    a.out_ << '\n';
}

template <class T>
std::enable_if_t<std::is_floating_point<T>::value, void>
load_scalar(xml_iarchive &a, T &value){
    detail::floating_point_facet().get(a.in_, value);
}

template <class T>
std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, void>
save_scalar(xml_oarchive &a, const T &value){
    using promoted_t = std::conditional_t<std::is_signed<T>::value, long long, unsigned long long>;
    a.out_ << static_cast<promoted_t>(value) << '\n';
}

inline void save_scalar(xml_oarchive &a, const bool &value){
    a.out_ << value << '\n';
}

template <class T>
std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, void>
load_scalar(xml_iarchive &a, T &value){
    using promoted_t = std::conditional_t<std::is_signed<T>::value, long long, unsigned long long>;
    promoted_t promoted = 0;
    a.in_ >> promoted;
    if(a.in_) value = static_cast<T>(promoted);
}

inline void load_scalar(xml_iarchive &a, bool &value){
    a.in_ >> value;
}

inline void save_scalar(xml_oarchive &a, const std::string &value){
    a.out_ << std::quoted(value) << '\n';
}

inline void load_scalar(xml_iarchive &a, std::string &value){
    a.in_ >> std::quoted(value);
}

template <class T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
serialize(xml_oarchive &a, const T &value){
    save_scalar(a, value);
}

template <class T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
serialize(xml_iarchive &a, T &value){
    load_scalar(a, value);
}

inline void serialize(xml_oarchive &a, const std::string &value){
    save_scalar(a, value);
}

inline void serialize(xml_iarchive &a, std::string &value){
    load_scalar(a, value);
}

template <class T>
void serialize(xml_oarchive &a, const std::vector<T> &value){
    auto size = value.size();
    a.save_named("size", size);
    for(const auto &v : value) a.save_named("item", v);
}

inline void serialize(xml_oarchive &a, const std::vector<bool> &value){
    auto size = value.size();
    a.save_named("size", size);
    for(bool v : value) a.save_named("item", v);
}

inline void serialize(xml_iarchive &a, std::vector<bool> &value){
    size_t size = 0;
    a.load_named(size);
    value.clear();
    value.resize(size);
    for(size_t i = 0; i < size; ++i){
        bool v = false;
        a.load_named(v);
        value[i] = v;
    }
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
void serialize(xml_oarchive &a, const std::list<T> &value){
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
void serialize(xml_oarchive &a, const std::array<T,N> &value){
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
void serialize(xml_oarchive &a, const std::map<K,V> &value){
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
