//Compiled on 6/24/2023 1:40:45 PM
#include <inttypes.h>
#include <stdbool.h>
#include <new>

#ifndef JUNIPER_H
#define JUNIPER_H

#include <stdlib.h>

namespace juniper
{
    template <typename contained>
    class shared_ptr {
    private:
        contained* ptr_;
        int* ref_count_;

        void inc_ref() {
            if (ref_count_ != nullptr) {
                ++(*ref_count_);
            }
        }

        void dec_ref() {
            if (ref_count_ != nullptr) {
                --(*ref_count_);

                if (*ref_count_ <= 0)
                {
                    if (ptr_ != nullptr)
                    {
                        delete ptr_;
                    }
                    delete ref_count_;
                }
            }
        }

    public:
        shared_ptr()
            : ptr_(nullptr), ref_count_(new int(1))
        {
        }

        shared_ptr(contained* p)
            : ptr_(p), ref_count_(new int(1))
        {
        }

        // Copy constructor
        shared_ptr(const shared_ptr& rhs)
            : ptr_(rhs.ptr_), ref_count_(rhs.ref_count_)
        {
            inc_ref();
        }

        // Move constructor
        shared_ptr(shared_ptr&& dyingObj)
            : ptr_(dyingObj.ptr_), ref_count_(dyingObj.ref_count_) {

            // Clean the dying object
            dyingObj.ptr_ = nullptr;
            dyingObj.ref_count_ = nullptr;
        }

        ~shared_ptr()
        {
            dec_ref();
        }

        void set(contained* p) {
            ptr_ = p;
        }

        contained* get() { return ptr_; }
        const contained* get() const { return ptr_; }

        // Copy assignment
        shared_ptr& operator=(const shared_ptr& rhs) {
            dec_ref();

            this->ptr_ = rhs.ptr_;
            this->ref_count_ = rhs.ref_count_;
            if (rhs.ptr_ != nullptr)
            {
                inc_ref();
            }

            return *this;
        }

        // Move assignment
        shared_ptr& operator=(shared_ptr&& dyingObj) {
            dec_ref();

            this->ptr_ = dyingObj.ptr;
            this->ref_count_ = dyingObj.refCount;

            // Clean the dying object
            dyingObj.ptr_ = nullptr;
            dyingObj.ref_count_ = nullptr;

            return *this;
        }

        contained& operator*() {
            return *ptr_;
        }

        contained* operator->() {
            return ptr_;
        }

        bool operator==(shared_ptr& rhs) {
            return ptr_ == rhs.ptr_;
        }

        bool operator!=(shared_ptr& rhs) {
            return ptr_ != rhs.ptr_;
        }
    };
    
    template <typename ClosureType, typename Result, typename ...Args>
    class function;

    template <typename Result, typename ...Args>
    class function<void, Result(Args...)> {
    private:
        Result(*F)(Args...);

    public:
        function(Result(*f)(Args...)) : F(f) {}

        Result operator()(Args... args) {
            return F(args...);
        }
    };

    template <typename ClosureType, typename Result, typename ...Args>
    class function<ClosureType, Result(Args...)> {
    private:
        ClosureType Closure;
        Result(*F)(ClosureType&, Args...);

    public:
        function(ClosureType closure, Result(*f)(ClosureType&, Args...)) : Closure(closure), F(f) {}

        Result operator()(Args... args) {
            return F(Closure, args...);
        }
    };

    template<typename T, size_t N>
    class array {
    public:
        array<T, N>& fill(T fillWith) {
            for (size_t i = 0; i < N; i++) {
                data[i] = fillWith;
            }

            return *this;
        }

        T& operator[](int i) {
            return data[i];
        }

        bool operator==(array<T, N>& rhs) {
            for (auto i = 0; i < N; i++) {
                if (data[i] != rhs[i]) {
                    return false;
                }
            }
            return true;
        }

        bool operator!=(array<T, N>& rhs) { return !(rhs == *this); }

        T data[N];
    };

    struct unit {
    public:
        bool operator==(unit rhs) {
            return true;
        }

        bool operator!=(unit rhs) {
            return !(rhs == *this);
        }
    };

    class rawpointer_container {
    private:
        void* data;
        function<void, unit(void*)> destructorCallback;

    public:
        rawpointer_container(void* initData, function<void, unit(void*)> callback)
            : data(initData), destructorCallback(callback) {}

        ~rawpointer_container() {
            destructorCallback(data);
        }

        void *get() { return data; }
    };

    using smartpointer = shared_ptr<rawpointer_container>;

    smartpointer make_smartpointer(void *initData, function<void, unit(void*)> callback) {
        return smartpointer(new rawpointer_container(initData, callback));
    }

    template<typename T>
    T quit() {
        exit(1);
    }

    // Equivalent to std::aligned_storage
    template<unsigned int Len, unsigned int Align>
    struct aligned_storage {
        struct type {
            alignas(Align) unsigned char data[Len];
        };
    };

    template <unsigned int arg1, unsigned int ... others>
    struct static_max;

    template <unsigned int arg>
    struct static_max<arg>
    {
        static const unsigned int value = arg;
    };

    template <unsigned int arg1, unsigned int arg2, unsigned int ... others>
    struct static_max<arg1, arg2, others...>
    {
        static const unsigned int value = arg1 >= arg2 ? static_max<arg1, others...>::value :
            static_max<arg2, others...>::value;
    };

    template<class T> struct remove_reference { typedef T type; };
    template<class T> struct remove_reference<T&> { typedef T type; };
    template<class T> struct remove_reference<T&&> { typedef T type; };

    template<unsigned char n, typename... Ts>
    struct variant_helper_rec;

    template<unsigned char n, typename F, typename... Ts>
    struct variant_helper_rec<n, F, Ts...> {
        inline static void destroy(unsigned char id, void* data)
        {
            if (n == id) {
                reinterpret_cast<F*>(data)->~F();
            } else {
                variant_helper_rec<n + 1, Ts...>::destroy(id, data);
            }
        }

        inline static void move(unsigned char id, void* from, void* to)
        {
            if (n == id) {
                // This static_cast and use of remove_reference is equivalent to the use of std::move
                new (to) F(static_cast<typename remove_reference<F>::type&&>(*reinterpret_cast<F*>(from)));
            } else {
                variant_helper_rec<n + 1, Ts...>::move(id, from, to);
            }
        }

        inline static void copy(unsigned char id, const void* from, void* to)
        {
            if (n == id) {
                new (to) F(*reinterpret_cast<const F*>(from));
            } else {
                variant_helper_rec<n + 1, Ts...>::copy(id, from, to);
            }
        }

        inline static bool equal(unsigned char id, void* lhs, void* rhs)
        {
            if (n == id) {
                return (*reinterpret_cast<F*>(lhs)) == (*reinterpret_cast<F*>(rhs));
            } else {
                return variant_helper_rec<n + 1, Ts...>::equal(id, lhs, rhs);
            }
        }
    };

    template<unsigned char n> struct variant_helper_rec<n> {
        inline static void destroy(unsigned char id, void* data) { }
        inline static void move(unsigned char old_t, void* from, void* to) { }
        inline static void copy(unsigned char old_t, const void* from, void* to) { }
        inline static bool equal(unsigned char id, void* lhs, void* rhs) { return false; }
    };

    template<typename... Ts>
    struct variant_helper {
        inline static void destroy(unsigned char id, void* data) {
            variant_helper_rec<0, Ts...>::destroy(id, data);
        }

        inline static void move(unsigned char id, void* from, void* to) {
            variant_helper_rec<0, Ts...>::move(id, from, to);
        }

        inline static void copy(unsigned char id, const void* old_v, void* new_v) {
            variant_helper_rec<0, Ts...>::copy(id, old_v, new_v);
        }

        inline static bool equal(unsigned char id, void* lhs, void* rhs) {
            return variant_helper_rec<0, Ts...>::equal(id, lhs, rhs);
        }
    };

    template<> struct variant_helper<> {
        inline static void destroy(unsigned char id, void* data) { }
        inline static void move(unsigned char old_t, void* old_v, void* new_v) { }
        inline static void copy(unsigned char old_t, const void* old_v, void* new_v) { }
    };

    template<typename F>
    struct variant_helper_static;

    template<typename F>
    struct variant_helper_static {
        inline static void move(void* from, void* to) {
            new (to) F(static_cast<typename remove_reference<F>::type&&>(*reinterpret_cast<F*>(from)));
        }

        inline static void copy(const void* from, void* to) {
            new (to) F(*reinterpret_cast<const F*>(from));
        }
    };

    // Given a unsigned char i, selects the ith type from the list of item types
    template<unsigned char i, typename... Items>
    struct variant_alternative;

    template<typename HeadItem, typename... TailItems>
    struct variant_alternative<0, HeadItem, TailItems...>
    {
        using type = HeadItem;
    };

    template<unsigned char i, typename HeadItem, typename... TailItems>
    struct variant_alternative<i, HeadItem, TailItems...>
    {
        using type = typename variant_alternative<i - 1, TailItems...>::type;
    };

    template<typename... Ts>
    struct variant {
    private:
        static const unsigned int data_size = static_max<sizeof(Ts)...>::value;
        static const unsigned int data_align = static_max<alignof(Ts)...>::value;

        using data_t = typename aligned_storage<data_size, data_align>::type;

        using helper_t = variant_helper<Ts...>;

        template<unsigned char i>
        using alternative = typename variant_alternative<i, Ts...>::type;

        unsigned char variant_id;
        data_t data;

        variant(unsigned char id) : variant_id(id) {}

    public:
        template<unsigned char i>
        static variant create(alternative<i>& value)
        {
            variant ret(i);
            variant_helper_static<alternative<i>>::copy(&value, &ret.data);
            return ret;
        }
        
        template<unsigned char i>
        static variant create(alternative<i>&& value) {
            variant ret(i);
            variant_helper_static<alternative<i>>::move(&value, &ret.data);
            return ret;
        }

        variant() {}

        variant(const variant<Ts...>& from) : variant_id(from.variant_id)
        {
            helper_t::copy(from.variant_id, &from.data, &data);
        }

        variant(variant<Ts...>&& from) : variant_id(from.variant_id)
        {
            helper_t::move(from.variant_id, &from.data, &data);
        }

        variant<Ts...>& operator= (variant<Ts...>& rhs)
        {
            helper_t::destroy(variant_id, &data);
            variant_id = rhs.variant_id;
            helper_t::copy(rhs.variant_id, &rhs.data, &data);
            return *this;
        }

        variant<Ts...>& operator= (variant<Ts...>&& rhs)
        {
            helper_t::destroy(variant_id, &data);
            variant_id = rhs.variant_id;
            helper_t::move(rhs.variant_id, &rhs.data, &data);
            return *this;
        }

        unsigned char id() {
            return variant_id;
        }

        template<unsigned char i>
        void set(alternative<i>& value)
        {
            helper_t::destroy(variant_id, &data);
            variant_id = i;
            variant_helper_static<alternative<i>>::copy(&value, &data);
        }

        template<unsigned char i>
        void set(alternative<i>&& value)
        {
            helper_t::destroy(variant_id, &data);
            variant_id = i;
            variant_helper_static<alternative<i>>::move(&value, &data);
        }

        template<unsigned char i>
        alternative<i>& get()
        {
            if (variant_id == i) {
                return *reinterpret_cast<alternative<i>*>(&data);
            } else {
                return quit<alternative<i>&>();
            }
        }

        ~variant() {
            helper_t::destroy(variant_id, &data);
        }

        bool operator==(variant& rhs) {
            if (variant_id == rhs.variant_id) {
                return helper_t::equal(variant_id, &data, &rhs.data);
            } else {
                return false;
            }
        }

        bool operator==(variant&& rhs) {
            if (variant_id == rhs.variant_id) {
                return helper_t::equal(variant_id, &data, &rhs.data);
            } else {
                return false;
            }
        }

        bool operator!=(variant& rhs) {
            return !(this->operator==(rhs));
        }

        bool operator!=(variant&& rhs) {
            return !(this->operator==(rhs));
        }
    };

    template<typename a, typename b>
    struct tuple2 {
        a e1;
        b e2;

        tuple2(a initE1, b initE2) : e1(initE1), e2(initE2) {}

        bool operator==(tuple2<a,b> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2;
        }

        bool operator!=(tuple2<a,b> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c>
    struct tuple3 {
        a e1;
        b e2;
        c e3;

        tuple3(a initE1, b initE2, c initE3) : e1(initE1), e2(initE2), e3(initE3) {}

        bool operator==(tuple3<a,b,c> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3;
        }

        bool operator!=(tuple3<a,b,c> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c, typename d>
    struct tuple4 {
        a e1;
        b e2;
        c e3;
        d e4;

        tuple4(a initE1, b initE2, c initE3, d initE4) : e1(initE1), e2(initE2), e3(initE3), e4(initE4) {}

        bool operator==(tuple4<a,b,c,d> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3 && e4 == rhs.e4;
        }

        bool operator!=(tuple4<a,b,c,d> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c, typename d, typename e>
    struct tuple5 {
        a e1;
        b e2;
        c e3;
        d e4;
        e e5;

        tuple5(a initE1, b initE2, c initE3, d initE4, e initE5) : e1(initE1), e2(initE2), e3(initE3), e4(initE4), e5(initE5) {}

        bool operator==(tuple5<a,b,c,d,e> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3 && e4 == rhs.e4 && e5 == rhs.e5;
        }

        bool operator!=(tuple5<a,b,c,d,e> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c, typename d, typename e, typename f>
    struct tuple6 {
        a e1;
        b e2;
        c e3;
        d e4;
        e e5;
        f e6;

        tuple6(a initE1, b initE2, c initE3, d initE4, e initE5, f initE6) : e1(initE1), e2(initE2), e3(initE3), e4(initE4), e5(initE5), e6(initE6) {}

        bool operator==(tuple6<a,b,c,d,e,f> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3 && e4 == rhs.e4 && e5 == rhs.e5 && e6 == rhs.e6;
        }

        bool operator!=(tuple6<a,b,c,d,e,f> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c, typename d, typename e, typename f, typename g>
    struct tuple7 {
        a e1;
        b e2;
        c e3;
        d e4;
        e e5;
        f e6;
        g e7;

        tuple7(a initE1, b initE2, c initE3, d initE4, e initE5, f initE6, g initE7) : e1(initE1), e2(initE2), e3(initE3), e4(initE4), e5(initE5), e6(initE6), e7(initE7) {}

        bool operator==(tuple7<a,b,c,d,e,f,g> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3 && e4 == rhs.e4 && e5 == rhs.e5 && e6 == rhs.e6 && e7 == rhs.e7;
        }

        bool operator!=(tuple7<a,b,c,d,e,f,g> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c, typename d, typename e, typename f, typename g, typename h>
    struct tuple8 {
        a e1;
        b e2;
        c e3;
        d e4;
        e e5;
        f e6;
        g e7;
        h e8;

        tuple8(a initE1, b initE2, c initE3, d initE4, e initE5, f initE6, g initE7, h initE8) : e1(initE1), e2(initE2), e3(initE3), e4(initE4), e5(initE5), e6(initE6), e7(initE7), e8(initE8) {}

        bool operator==(tuple8<a,b,c,d,e,f,g,h> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3 && e4 == rhs.e4 && e5 == rhs.e5 && e6 == rhs.e6 && e7 == rhs.e7 && e8 == rhs.e8;
        }

        bool operator!=(tuple8<a,b,c,d,e,f,g,h> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c, typename d, typename e, typename f, typename g, typename h, typename i>
    struct tuple9 {
        a e1;
        b e2;
        c e3;
        d e4;
        e e5;
        f e6;
        g e7;
        h e8;
        i e9;

        tuple9(a initE1, b initE2, c initE3, d initE4, e initE5, f initE6, g initE7, h initE8, i initE9) : e1(initE1), e2(initE2), e3(initE3), e4(initE4), e5(initE5), e6(initE6), e7(initE7), e8(initE8), e9(initE9) {}

        bool operator==(tuple9<a,b,c,d,e,f,g,h,i> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3 && e4 == rhs.e4 && e5 == rhs.e5 && e6 == rhs.e6 && e7 == rhs.e7 && e8 == rhs.e8 && e9 == rhs.e9;
        }

        bool operator!=(tuple9<a,b,c,d,e,f,g,h,i> rhs) {
            return !(rhs == *this);
        }
    };

    template<typename a, typename b, typename c, typename d, typename e, typename f, typename g, typename h, typename i, typename j>
    struct tuple10 {
        a e1;
        b e2;
        c e3;
        d e4;
        e e5;
        f e6;
        g e7;
        h e8;
        i e9;
        j e10;

        tuple10(a initE1, b initE2, c initE3, d initE4, e initE5, f initE6, g initE7, h initE8, i initE9, j initE10) : e1(initE1), e2(initE2), e3(initE3), e4(initE4), e5(initE5), e6(initE6), e7(initE7), e8(initE8), e9(initE9), e10(initE10) {}

        bool operator==(tuple10<a,b,c,d,e,f,g,h,i,j> rhs) {
            return e1 == rhs.e1 && e2 == rhs.e2 && e3 == rhs.e3 && e4 == rhs.e4 && e5 == rhs.e5 && e6 == rhs.e6 && e7 == rhs.e7 && e8 == rhs.e8 && e9 == rhs.e9 && e10 == rhs.e10;
        }

        bool operator!=(tuple10<a,b,c,d,e,f,g,h,i,j> rhs) {
            return !(rhs == *this);
        }
    };
}

#endif

#include <Arduino.h>
#include <Arduino.h>

namespace Prelude {}
namespace List {}
namespace Signal {}
namespace Io {}
namespace Maybe {}
namespace Time {}
namespace Math {}
namespace Button {}
namespace Vector {}
namespace CharList {}
namespace StringM {}
namespace Random {}
namespace Color {}
namespace Blink {}
namespace List {
    using namespace Prelude;

}

namespace Signal {
    using namespace Prelude;

}

namespace Io {
    using namespace Prelude;

}

namespace Maybe {
    using namespace Prelude;

}

namespace Time {
    using namespace Prelude;

}

namespace Math {
    using namespace Prelude;

}

namespace Button {
    using namespace Prelude;
    using namespace Io;

}

namespace Vector {
    using namespace Prelude;
    using namespace List;
    using namespace Math;

}

namespace CharList {
    using namespace Prelude;

}

namespace StringM {
    using namespace Prelude;

}

namespace Random {
    using namespace Prelude;

}

namespace Color {
    using namespace Prelude;

}

namespace Blink {
    using namespace Prelude;
    using namespace Io;

}

namespace juniper {
    namespace records {
        template<typename T1,typename T2,typename T3,typename T4>
        struct recordt_5 {
            T1 a;
            T2 b;
            T3 g;
            T4 r;

            recordt_5() {}

            recordt_5(T1 init_a, T2 init_b, T3 init_g, T4 init_r)
                : a(init_a), b(init_b), g(init_g), r(init_r) {}

            bool operator==(recordt_5<T1, T2, T3, T4> rhs) {
                return true && a == rhs.a && b == rhs.b && g == rhs.g && r == rhs.r;
            }

            bool operator!=(recordt_5<T1, T2, T3, T4> rhs) {
                return !(rhs == *this);
            }
        };

        template<typename T1,typename T2,typename T3,typename T4>
        struct recordt_7 {
            T1 a;
            T2 h;
            T3 s;
            T4 v;

            recordt_7() {}

            recordt_7(T1 init_a, T2 init_h, T3 init_s, T4 init_v)
                : a(init_a), h(init_h), s(init_s), v(init_v) {}

            bool operator==(recordt_7<T1, T2, T3, T4> rhs) {
                return true && a == rhs.a && h == rhs.h && s == rhs.s && v == rhs.v;
            }

            bool operator!=(recordt_7<T1, T2, T3, T4> rhs) {
                return !(rhs == *this);
            }
        };

        template<typename T1,typename T2,typename T3>
        struct recordt_2 {
            T1 actualState;
            T2 lastDebounceTime;
            T3 lastState;

            recordt_2() {}

            recordt_2(T1 init_actualState, T2 init_lastDebounceTime, T3 init_lastState)
                : actualState(init_actualState), lastDebounceTime(init_lastDebounceTime), lastState(init_lastState) {}

            bool operator==(recordt_2<T1, T2, T3> rhs) {
                return true && actualState == rhs.actualState && lastDebounceTime == rhs.lastDebounceTime && lastState == rhs.lastState;
            }

            bool operator!=(recordt_2<T1, T2, T3> rhs) {
                return !(rhs == *this);
            }
        };

        template<typename T1,typename T2,typename T3>
        struct recordt_4 {
            T1 b;
            T2 g;
            T3 r;

            recordt_4() {}

            recordt_4(T1 init_b, T2 init_g, T3 init_r)
                : b(init_b), g(init_g), r(init_r) {}

            bool operator==(recordt_4<T1, T2, T3> rhs) {
                return true && b == rhs.b && g == rhs.g && r == rhs.r;
            }

            bool operator!=(recordt_4<T1, T2, T3> rhs) {
                return !(rhs == *this);
            }
        };

        template<typename T1>
        struct recordt_3 {
            T1 data;

            recordt_3() {}

            recordt_3(T1 init_data)
                : data(init_data) {}

            bool operator==(recordt_3<T1> rhs) {
                return true && data == rhs.data;
            }

            bool operator!=(recordt_3<T1> rhs) {
                return !(rhs == *this);
            }
        };

        template<typename T1,typename T2>
        struct recordt_0 {
            T1 data;
            T2 length;

            recordt_0() {}

            recordt_0(T1 init_data, T2 init_length)
                : data(init_data), length(init_length) {}

            bool operator==(recordt_0<T1, T2> rhs) {
                return true && data == rhs.data && length == rhs.length;
            }

            bool operator!=(recordt_0<T1, T2> rhs) {
                return !(rhs == *this);
            }
        };

        template<typename T1,typename T2,typename T3>
        struct recordt_6 {
            T1 h;
            T2 s;
            T3 v;

            recordt_6() {}

            recordt_6(T1 init_h, T2 init_s, T3 init_v)
                : h(init_h), s(init_s), v(init_v) {}

            bool operator==(recordt_6<T1, T2, T3> rhs) {
                return true && h == rhs.h && s == rhs.s && v == rhs.v;
            }

            bool operator!=(recordt_6<T1, T2, T3> rhs) {
                return !(rhs == *this);
            }
        };

        template<typename T1>
        struct recordt_1 {
            T1 lastPulse;

            recordt_1() {}

            recordt_1(T1 init_lastPulse)
                : lastPulse(init_lastPulse) {}

            bool operator==(recordt_1<T1> rhs) {
                return true && lastPulse == rhs.lastPulse;
            }

            bool operator!=(recordt_1<T1> rhs) {
                return !(rhs == *this);
            }
        };


    }
}

namespace juniper {
    namespace closures {
        template<typename T1,typename T2>
        struct closuret_8 {
            T1 buttonState;
            T2 delay;


            closuret_8(T1 init_buttonState, T2 init_delay) :
                buttonState(init_buttonState), delay(init_delay) {}
        };

        template<typename T1>
        struct closuret_1 {
            T1 f;


            closuret_1(T1 init_f) :
                f(init_f) {}
        };

        template<typename T1,typename T2>
        struct closuret_0 {
            T1 f;
            T2 g;


            closuret_0(T1 init_f, T2 init_g) :
                f(init_f), g(init_g) {}
        };

        template<typename T1,typename T2>
        struct closuret_2 {
            T1 f;
            T2 valueA;


            closuret_2(T1 init_f, T2 init_valueA) :
                f(init_f), valueA(init_valueA) {}
        };

        template<typename T1,typename T2,typename T3>
        struct closuret_3 {
            T1 f;
            T2 valueA;
            T3 valueB;


            closuret_3(T1 init_f, T2 init_valueA, T3 init_valueB) :
                f(init_f), valueA(init_valueA), valueB(init_valueB) {}
        };

        template<typename T1>
        struct closuret_4 {
            T1 maybePrevValue;


            closuret_4(T1 init_maybePrevValue) :
                maybePrevValue(init_maybePrevValue) {}
        };

        template<typename T1>
        struct closuret_6 {
            T1 pin;


            closuret_6(T1 init_pin) :
                pin(init_pin) {}
        };

        template<typename T1>
        struct closuret_7 {
            T1 prevState;


            closuret_7(T1 init_prevState) :
                prevState(init_prevState) {}
        };

        template<typename T1,typename T2>
        struct closuret_5 {
            T1 val1;
            T2 val2;


            closuret_5(T1 init_val1, T2 init_val2) :
                val1(init_val1), val2(init_val2) {}
        };


    }
}

namespace Prelude {
    template<typename a>
    struct maybe {
        juniper::variant<a, uint8_t> data;

        maybe() {}

        maybe(juniper::variant<a, uint8_t> initData) : data(initData) {}

        a just() {
            return data.template get<0>();
        }

        uint8_t nothing() {
            return data.template get<1>();
        }

        uint8_t id() {
            return data.id();
        }

        bool operator==(maybe rhs) {
            return data == rhs.data;
        }

        bool operator!=(maybe rhs) {
            return !(this->operator==(rhs));
        }
    };

    template<typename a>
    Prelude::maybe<a> just(a data0) {
        return Prelude::maybe<a>(juniper::variant<a, uint8_t>::template create<0>(data0));
    }

    template<typename a>
    Prelude::maybe<a> nothing() {
        return Prelude::maybe<a>(juniper::variant<a, uint8_t>::template create<1>(0));
    }


}

namespace Prelude {
    template<typename a, typename b>
    struct either {
        juniper::variant<a, b> data;

        either() {}

        either(juniper::variant<a, b> initData) : data(initData) {}

        a left() {
            return data.template get<0>();
        }

        b right() {
            return data.template get<1>();
        }

        uint8_t id() {
            return data.id();
        }

        bool operator==(either rhs) {
            return data == rhs.data;
        }

        bool operator!=(either rhs) {
            return !(this->operator==(rhs));
        }
    };

    template<typename a, typename b>
    Prelude::either<a, b> left(a data0) {
        return Prelude::either<a, b>(juniper::variant<a, b>::template create<0>(data0));
    }

    template<typename a, typename b>
    Prelude::either<a, b> right(b data0) {
        return Prelude::either<a, b>(juniper::variant<a, b>::template create<1>(data0));
    }


}

namespace Prelude {
    template<typename a, int n>
    using list = juniper::records::recordt_0<juniper::array<a, n>, uint32_t>;


}

namespace Prelude {
    template<int n>
    using charlist = juniper::records::recordt_0<juniper::array<uint8_t, (1)+(n)>, uint32_t>;


}

namespace Prelude {
    template<typename a>
    struct sig {
        juniper::variant<Prelude::maybe<a>> data;

        sig() {}

        sig(juniper::variant<Prelude::maybe<a>> initData) : data(initData) {}

        Prelude::maybe<a> signal() {
            return data.template get<0>();
        }

        uint8_t id() {
            return data.id();
        }

        bool operator==(sig rhs) {
            return data == rhs.data;
        }

        bool operator!=(sig rhs) {
            return !(this->operator==(rhs));
        }
    };

    template<typename a>
    Prelude::sig<a> signal(Prelude::maybe<a> data0) {
        return Prelude::sig<a>(juniper::variant<Prelude::maybe<a>>::template create<0>(data0));
    }


}

namespace Io {
    struct pinState {
        juniper::variant<uint8_t, uint8_t> data;

        pinState() {}

        pinState(juniper::variant<uint8_t, uint8_t> initData) : data(initData) {}

        uint8_t high() {
            return data.template get<0>();
        }

        uint8_t low() {
            return data.template get<1>();
        }

        uint8_t id() {
            return data.id();
        }

        bool operator==(pinState rhs) {
            return data == rhs.data;
        }

        bool operator!=(pinState rhs) {
            return !(this->operator==(rhs));
        }
    };

    Io::pinState high() {
        return Io::pinState(juniper::variant<uint8_t, uint8_t>::template create<0>(0));
    }

    Io::pinState low() {
        return Io::pinState(juniper::variant<uint8_t, uint8_t>::template create<1>(0));
    }


}

namespace Io {
    struct mode {
        juniper::variant<uint8_t, uint8_t, uint8_t> data;

        mode() {}

        mode(juniper::variant<uint8_t, uint8_t, uint8_t> initData) : data(initData) {}

        uint8_t input() {
            return data.template get<0>();
        }

        uint8_t output() {
            return data.template get<1>();
        }

        uint8_t inputPullup() {
            return data.template get<2>();
        }

        uint8_t id() {
            return data.id();
        }

        bool operator==(mode rhs) {
            return data == rhs.data;
        }

        bool operator!=(mode rhs) {
            return !(this->operator==(rhs));
        }
    };

    Io::mode input() {
        return Io::mode(juniper::variant<uint8_t, uint8_t, uint8_t>::template create<0>(0));
    }

    Io::mode output() {
        return Io::mode(juniper::variant<uint8_t, uint8_t, uint8_t>::template create<1>(0));
    }

    Io::mode inputPullup() {
        return Io::mode(juniper::variant<uint8_t, uint8_t, uint8_t>::template create<2>(0));
    }


}

namespace Io {
    struct base {
        juniper::variant<uint8_t, uint8_t, uint8_t, uint8_t> data;

        base() {}

        base(juniper::variant<uint8_t, uint8_t, uint8_t, uint8_t> initData) : data(initData) {}

        uint8_t binary() {
            return data.template get<0>();
        }

        uint8_t octal() {
            return data.template get<1>();
        }

        uint8_t decimal() {
            return data.template get<2>();
        }

        uint8_t hexadecimal() {
            return data.template get<3>();
        }

        uint8_t id() {
            return data.id();
        }

        bool operator==(base rhs) {
            return data == rhs.data;
        }

        bool operator!=(base rhs) {
            return !(this->operator==(rhs));
        }
    };

    Io::base binary() {
        return Io::base(juniper::variant<uint8_t, uint8_t, uint8_t, uint8_t>::template create<0>(0));
    }

    Io::base octal() {
        return Io::base(juniper::variant<uint8_t, uint8_t, uint8_t, uint8_t>::template create<1>(0));
    }

    Io::base decimal() {
        return Io::base(juniper::variant<uint8_t, uint8_t, uint8_t, uint8_t>::template create<2>(0));
    }

    Io::base hexadecimal() {
        return Io::base(juniper::variant<uint8_t, uint8_t, uint8_t, uint8_t>::template create<3>(0));
    }


}

namespace Time {
    using timerState = juniper::records::recordt_1<uint32_t>;


}

namespace Button {
    using buttonState = juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>;


}

namespace Vector {
    template<typename a, int n>
    using vector = juniper::records::recordt_3<juniper::array<a, n>>;


}

namespace Color {
    using rgb = juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>;


}

namespace Color {
    using rgba = juniper::records::recordt_5<uint8_t, uint8_t, uint8_t, uint8_t>;


}

namespace Color {
    using hsv = juniper::records::recordt_6<float, float, float>;


}

namespace Color {
    using hsva = juniper::records::recordt_7<float, float, float, float>;


}

namespace Prelude {
    void * extractptr(juniper::smartpointer p);
}

namespace Prelude {
    template<typename t10, typename t6, typename t7, typename t8, typename t9>
    juniper::function<juniper::closures::closuret_0<juniper::function<t8, t7(t6)>, juniper::function<t9, t6(t10)>>, t7(t10)> compose(juniper::function<t8, t7(t6)> f, juniper::function<t9, t6(t10)> g);
}

namespace Prelude {
    template<typename t22, typename t23, typename t20, typename t21>
    juniper::function<juniper::closures::closuret_1<juniper::function<t21, t20(t22, t23)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>, t20(t23)>(t22)> curry(juniper::function<t21, t20(t22, t23)> f);
}

namespace Prelude {
    template<typename t34, typename t35, typename t31, typename t32, typename t33>
    juniper::function<juniper::closures::closuret_1<juniper::function<t32, juniper::function<t33, t31(t35)>(t34)>>, t31(t34, t35)> uncurry(juniper::function<t32, juniper::function<t33, t31(t35)>(t34)> f);
}

namespace Prelude {
    template<typename t48, typename t49, typename t50, typename t46, typename t47>
    juniper::function<juniper::closures::closuret_1<juniper::function<t47, t46(t48, t49, t50)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>, juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)>(t49)>(t48)> curry3(juniper::function<t47, t46(t48, t49, t50)> f);
}

namespace Prelude {
    template<typename t64, typename t65, typename t66, typename t60, typename t61, typename t62, typename t63>
    juniper::function<juniper::closures::closuret_1<juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)>>, t60(t64, t65, t66)> uncurry3(juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)> f);
}

namespace Prelude {
    template<typename t77>
    bool eq(t77 x, t77 y);
}

namespace Prelude {
    template<typename t80>
    bool neq(t80 x, t80 y);
}

namespace Prelude {
    template<typename t82, typename t83>
    bool gt(t82 x, t83 y);
}

namespace Prelude {
    template<typename t85, typename t86>
    bool geq(t85 x, t86 y);
}

namespace Prelude {
    template<typename t88, typename t89>
    bool lt(t88 x, t89 y);
}

namespace Prelude {
    template<typename t91, typename t92>
    bool leq(t91 x, t92 y);
}

namespace Prelude {
    bool notf(bool x);
}

namespace Prelude {
    bool andf(bool x, bool y);
}

namespace Prelude {
    bool orf(bool x, bool y);
}

namespace Prelude {
    template<typename t102, typename t103, typename t104>
    t103 apply(juniper::function<t104, t103(t102)> f, t102 x);
}

namespace Prelude {
    template<typename t109, typename t110, typename t111, typename t112>
    t111 apply2(juniper::function<t112, t111(t109, t110)> f, juniper::tuple2<t109,t110> tup);
}

namespace Prelude {
    template<typename t122, typename t123, typename t124, typename t125, typename t126>
    t125 apply3(juniper::function<t126, t125(t122, t123, t124)> f, juniper::tuple3<t122,t123,t124> tup);
}

namespace Prelude {
    template<typename t139, typename t140, typename t141, typename t142, typename t143, typename t144>
    t143 apply4(juniper::function<t144, t143(t139, t140, t141, t142)> f, juniper::tuple4<t139,t140,t141,t142> tup);
}

namespace Prelude {
    template<typename t160, typename t161>
    t160 fst(juniper::tuple2<t160,t161> tup);
}

namespace Prelude {
    template<typename t165, typename t166>
    t166 snd(juniper::tuple2<t165,t166> tup);
}

namespace Prelude {
    template<typename t170>
    t170 add(t170 numA, t170 numB);
}

namespace Prelude {
    template<typename t172>
    t172 sub(t172 numA, t172 numB);
}

namespace Prelude {
    template<typename t174>
    t174 mul(t174 numA, t174 numB);
}

namespace Prelude {
    template<typename t176>
    t176 div(t176 numA, t176 numB);
}

namespace Prelude {
    template<typename t179, typename t180>
    juniper::tuple2<t180,t179> swap(juniper::tuple2<t179,t180> tup);
}

namespace Prelude {
    template<typename t184, typename t185, typename t186>
    t184 until(juniper::function<t185, bool(t184)> p, juniper::function<t186, t184(t184)> f, t184 a0);
}

namespace Prelude {
    template<typename t194>
    juniper::unit ignore(t194 val);
}

namespace Prelude {
    uint16_t u8ToU16(uint8_t n);
}

namespace Prelude {
    uint32_t u8ToU32(uint8_t n);
}

namespace Prelude {
    int8_t u8ToI8(uint8_t n);
}

namespace Prelude {
    int16_t u8ToI16(uint8_t n);
}

namespace Prelude {
    int32_t u8ToI32(uint8_t n);
}

namespace Prelude {
    float u8ToFloat(uint8_t n);
}

namespace Prelude {
    double u8ToDouble(uint8_t n);
}

namespace Prelude {
    uint8_t u16ToU8(uint16_t n);
}

namespace Prelude {
    uint32_t u16ToU32(uint16_t n);
}

namespace Prelude {
    int8_t u16ToI8(uint16_t n);
}

namespace Prelude {
    int16_t u16ToI16(uint16_t n);
}

namespace Prelude {
    int32_t u16ToI32(uint16_t n);
}

namespace Prelude {
    float u16ToFloat(uint16_t n);
}

namespace Prelude {
    double u16ToDouble(uint16_t n);
}

namespace Prelude {
    uint8_t u32ToU8(uint32_t n);
}

namespace Prelude {
    uint16_t u32ToU16(uint32_t n);
}

namespace Prelude {
    int8_t u32ToI8(uint32_t n);
}

namespace Prelude {
    int16_t u32ToI16(uint32_t n);
}

namespace Prelude {
    int32_t u32ToI32(uint32_t n);
}

namespace Prelude {
    float u32ToFloat(uint32_t n);
}

namespace Prelude {
    double u32ToDouble(uint32_t n);
}

namespace Prelude {
    uint8_t i8ToU8(int8_t n);
}

namespace Prelude {
    uint16_t i8ToU16(int8_t n);
}

namespace Prelude {
    uint32_t i8ToU32(int8_t n);
}

namespace Prelude {
    int16_t i8ToI16(int8_t n);
}

namespace Prelude {
    int32_t i8ToI32(int8_t n);
}

namespace Prelude {
    float i8ToFloat(int8_t n);
}

namespace Prelude {
    double i8ToDouble(int8_t n);
}

namespace Prelude {
    uint8_t i16ToU8(int16_t n);
}

namespace Prelude {
    uint16_t i16ToU16(int16_t n);
}

namespace Prelude {
    uint32_t i16ToU32(int16_t n);
}

namespace Prelude {
    int8_t i16ToI8(int16_t n);
}

namespace Prelude {
    int32_t i16ToI32(int16_t n);
}

namespace Prelude {
    float i16ToFloat(int16_t n);
}

namespace Prelude {
    double i16ToDouble(int16_t n);
}

namespace Prelude {
    uint8_t i32ToU8(int32_t n);
}

namespace Prelude {
    uint16_t i32ToU16(int32_t n);
}

namespace Prelude {
    uint32_t i32ToU32(int32_t n);
}

namespace Prelude {
    int8_t i32ToI8(int32_t n);
}

namespace Prelude {
    int16_t i32ToI16(int32_t n);
}

namespace Prelude {
    float i32ToFloat(int32_t n);
}

namespace Prelude {
    double i32ToDouble(int32_t n);
}

namespace Prelude {
    uint8_t floatToU8(float n);
}

namespace Prelude {
    uint16_t floatToU16(float n);
}

namespace Prelude {
    uint32_t floatToU32(float n);
}

namespace Prelude {
    int8_t floatToI8(float n);
}

namespace Prelude {
    int16_t floatToI16(float n);
}

namespace Prelude {
    int32_t floatToI32(float n);
}

namespace Prelude {
    double floatToDouble(float n);
}

namespace Prelude {
    uint8_t doubleToU8(double n);
}

namespace Prelude {
    uint16_t doubleToU16(double n);
}

namespace Prelude {
    uint32_t doubleToU32(double n);
}

namespace Prelude {
    int8_t doubleToI8(double n);
}

namespace Prelude {
    int16_t doubleToI16(double n);
}

namespace Prelude {
    int32_t doubleToI32(double n);
}

namespace Prelude {
    float doubleToFloat(double n);
}

namespace Prelude {
    template<typename t252>
    uint8_t toUInt8(t252 n);
}

namespace Prelude {
    template<typename t254>
    int8_t toInt8(t254 n);
}

namespace Prelude {
    template<typename t256>
    uint16_t toUInt16(t256 n);
}

namespace Prelude {
    template<typename t258>
    int16_t toInt16(t258 n);
}

namespace Prelude {
    template<typename t260>
    uint32_t toUInt32(t260 n);
}

namespace Prelude {
    template<typename t262>
    int32_t toInt32(t262 n);
}

namespace Prelude {
    template<typename t264>
    float toFloat(t264 n);
}

namespace Prelude {
    template<typename t266>
    double toDouble(t266 n);
}

namespace Prelude {
    template<typename t268>
    t268 fromUInt8(uint8_t n);
}

namespace Prelude {
    template<typename t270>
    t270 fromInt8(int8_t n);
}

namespace Prelude {
    template<typename t272>
    t272 fromUInt16(uint16_t n);
}

namespace Prelude {
    template<typename t274>
    t274 fromInt16(int16_t n);
}

namespace Prelude {
    template<typename t276>
    t276 fromUInt32(uint32_t n);
}

namespace Prelude {
    template<typename t278>
    t278 fromInt32(int32_t n);
}

namespace Prelude {
    template<typename t280>
    t280 fromFloat(float n);
}

namespace Prelude {
    template<typename t282>
    t282 fromDouble(double n);
}

namespace Prelude {
    template<typename t284, typename t285>
    t285 cast(t284 x);
}

namespace List {
    template<typename t292, typename t296, typename t289, int c4>
    juniper::records::recordt_0<juniper::array<t296, c4>, uint32_t> map(juniper::function<t289, t296(t292)> f, juniper::records::recordt_0<juniper::array<t292, c4>, uint32_t> lst);
}

namespace List {
    template<typename t304, typename t300, typename t301, int c7>
    t300 foldl(juniper::function<t301, t300(t304, t300)> f, t300 initState, juniper::records::recordt_0<juniper::array<t304, c7>, uint32_t> lst);
}

namespace List {
    template<typename t315, typename t311, typename t312, int c9>
    t311 foldr(juniper::function<t312, t311(t315, t311)> f, t311 initState, juniper::records::recordt_0<juniper::array<t315, c9>, uint32_t> lst);
}

namespace List {
    template<typename t331, int c11, int c12, int c13>
    juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t> append(juniper::records::recordt_0<juniper::array<t331, c11>, uint32_t> lstA, juniper::records::recordt_0<juniper::array<t331, c12>, uint32_t> lstB);
}

namespace List {
    template<typename t335, int c18>
    t335 nth(uint32_t i, juniper::records::recordt_0<juniper::array<t335, c18>, uint32_t> lst);
}

namespace List {
    template<typename t348, int c20, int c21>
    juniper::records::recordt_0<juniper::array<t348, (c21)*(c20)>, uint32_t> flattenSafe(juniper::records::recordt_0<juniper::array<juniper::records::recordt_0<juniper::array<t348, c20>, uint32_t>, c21>, uint32_t> listOfLists);
}

namespace List {
    template<typename t354, int c26, int c27>
    juniper::records::recordt_0<juniper::array<t354, c27>, uint32_t> resize(juniper::records::recordt_0<juniper::array<t354, c26>, uint32_t> lst);
}

namespace List {
    template<typename t362, typename t359, int c30>
    bool all(juniper::function<t359, bool(t362)> pred, juniper::records::recordt_0<juniper::array<t362, c30>, uint32_t> lst);
}

namespace List {
    template<typename t371, typename t368, int c32>
    bool any(juniper::function<t368, bool(t371)> pred, juniper::records::recordt_0<juniper::array<t371, c32>, uint32_t> lst);
}

namespace List {
    template<typename t376, int c34>
    juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> pushBack(t376 elem, juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> lst);
}

namespace List {
    template<typename t386, int c36>
    juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> pushOffFront(t386 elem, juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> lst);
}

namespace List {
    template<typename t398, int c40>
    juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> setNth(uint32_t index, t398 elem, juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> lst);
}

namespace List {
    template<typename t403, int c42>
    juniper::records::recordt_0<juniper::array<t403, c42>, uint32_t> replicate(uint32_t numOfElements, t403 elem);
}

namespace List {
    template<typename t413, int c43>
    juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> remove(t413 elem, juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> lst);
}

namespace List {
    template<typename t417, int c47>
    juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t> dropLast(juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t> lst);
}

namespace List {
    template<typename t426, typename t423, int c48>
    juniper::unit foreach(juniper::function<t423, juniper::unit(t426)> f, juniper::records::recordt_0<juniper::array<t426, c48>, uint32_t> lst);
}

namespace List {
    template<typename t436, int c50>
    t436 last(juniper::records::recordt_0<juniper::array<t436, c50>, uint32_t> lst);
}

namespace List {
    template<typename t446, int c52>
    t446 max_(juniper::records::recordt_0<juniper::array<t446, c52>, uint32_t> lst);
}

namespace List {
    template<typename t456, int c56>
    t456 min_(juniper::records::recordt_0<juniper::array<t456, c56>, uint32_t> lst);
}

namespace List {
    template<typename t458, int c60>
    bool member(t458 elem, juniper::records::recordt_0<juniper::array<t458, c60>, uint32_t> lst);
}

namespace List {
    template<typename t470, typename t472, int c62>
    juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t> zip(juniper::records::recordt_0<juniper::array<t470, c62>, uint32_t> lstA, juniper::records::recordt_0<juniper::array<t472, c62>, uint32_t> lstB);
}

namespace List {
    template<typename t483, typename t484, int c66>
    juniper::tuple2<juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t>,juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t>> unzip(juniper::records::recordt_0<juniper::array<juniper::tuple2<t483,t484>, c66>, uint32_t> lst);
}

namespace List {
    template<typename t493, int c70>
    t493 sum(juniper::records::recordt_0<juniper::array<t493, c70>, uint32_t> lst);
}

namespace List {
    template<typename t505, int c71>
    t505 average(juniper::records::recordt_0<juniper::array<t505, c71>, uint32_t> lst);
}

namespace Signal {
    template<typename t511, typename t512, typename t513>
    Prelude::sig<t512> map(juniper::function<t513, t512(t511)> f, Prelude::sig<t511> s);
}

namespace Signal {
    template<typename t529, typename t530>
    juniper::unit sink(juniper::function<t530, juniper::unit(t529)> f, Prelude::sig<t529> s);
}

namespace Signal {
    template<typename t535, typename t536>
    Prelude::sig<t535> filter(juniper::function<t536, bool(t535)> f, Prelude::sig<t535> s);
}

namespace Signal {
    template<typename t551>
    Prelude::sig<t551> merge(Prelude::sig<t551> sigA, Prelude::sig<t551> sigB);
}

namespace Signal {
    template<typename t553, int c72>
    Prelude::sig<t553> mergeMany(juniper::records::recordt_0<juniper::array<Prelude::sig<t553>, c72>, uint32_t> sigs);
}

namespace Signal {
    template<typename t570, typename t571>
    Prelude::sig<Prelude::either<t570, t571>> join(Prelude::sig<t570> sigA, Prelude::sig<t571> sigB);
}

namespace Signal {
    template<typename t601>
    Prelude::sig<juniper::unit> toUnit(Prelude::sig<t601> s);
}

namespace Signal {
    template<typename t607, typename t615, typename t609>
    Prelude::sig<t615> foldP(juniper::function<t609, t615(t607, t615)> f, juniper::shared_ptr<t615> state0, Prelude::sig<t607> incoming);
}

namespace Signal {
    template<typename t631>
    Prelude::sig<t631> dropRepeats(juniper::shared_ptr<Prelude::maybe<t631>> maybePrevValue, Prelude::sig<t631> incoming);
}

namespace Signal {
    template<typename t643>
    Prelude::sig<t643> latch(juniper::shared_ptr<t643> prevValue, Prelude::sig<t643> incoming);
}

namespace Signal {
    template<typename t658, typename t662, typename t654, typename t655>
    Prelude::sig<t654> map2(juniper::function<t655, t654(t658, t662)> f, juniper::shared_ptr<juniper::tuple2<t658,t662>> state, Prelude::sig<t658> incomingA, Prelude::sig<t662> incomingB);
}

namespace Signal {
    template<typename t683, int c74>
    Prelude::sig<juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>> record(juniper::shared_ptr<juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>> pastValues, Prelude::sig<t683> incoming);
}

namespace Signal {
    template<typename t693>
    Prelude::sig<t693> constant(t693 val);
}

namespace Signal {
    template<typename t701>
    Prelude::sig<Prelude::maybe<t701>> meta(Prelude::sig<t701> sigA);
}

namespace Signal {
    template<typename t706>
    Prelude::sig<t706> unmeta(Prelude::sig<Prelude::maybe<t706>> sigA);
}

namespace Signal {
    template<typename t718, typename t719>
    Prelude::sig<juniper::tuple2<t718,t719>> zip(juniper::shared_ptr<juniper::tuple2<t718,t719>> state, Prelude::sig<t718> sigA, Prelude::sig<t719> sigB);
}

namespace Signal {
    template<typename t750, typename t757>
    juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>> unzip(Prelude::sig<juniper::tuple2<t750,t757>> incoming);
}

namespace Signal {
    template<typename t764, typename t765>
    Prelude::sig<t764> toggle(t764 val1, t764 val2, juniper::shared_ptr<t764> state, Prelude::sig<t765> incoming);
}

namespace Io {
    Io::pinState toggle(Io::pinState p);
}

namespace Io {
    juniper::unit printStr(const char * str);
}

namespace Io {
    template<int c75>
    juniper::unit printCharList(juniper::records::recordt_0<juniper::array<uint8_t, c75>, uint32_t> cl);
}

namespace Io {
    juniper::unit printFloat(float f);
}

namespace Io {
    juniper::unit printInt(int32_t n);
}

namespace Io {
    template<typename t790>
    t790 baseToInt(Io::base b);
}

namespace Io {
    juniper::unit printIntBase(int32_t n, Io::base b);
}

namespace Io {
    juniper::unit printFloatPlaces(float f, int32_t numPlaces);
}

namespace Io {
    juniper::unit beginSerial(uint32_t speed);
}

namespace Io {
    uint8_t pinStateToInt(Io::pinState value);
}

namespace Io {
    Io::pinState intToPinState(uint8_t value);
}

namespace Io {
    juniper::unit digWrite(uint16_t pin, Io::pinState value);
}

namespace Io {
    Io::pinState digRead(uint16_t pin);
}

namespace Io {
    Prelude::sig<Io::pinState> digIn(uint16_t pin);
}

namespace Io {
    juniper::unit digOut(uint16_t pin, Prelude::sig<Io::pinState> sig);
}

namespace Io {
    uint16_t anaRead(uint16_t pin);
}

namespace Io {
    juniper::unit anaWrite(uint16_t pin, uint8_t value);
}

namespace Io {
    Prelude::sig<uint16_t> anaIn(uint16_t pin);
}

namespace Io {
    juniper::unit anaOut(uint16_t pin, Prelude::sig<uint8_t> sig);
}

namespace Io {
    uint8_t pinModeToInt(Io::mode m);
}

namespace Io {
    Io::mode intToPinMode(uint8_t m);
}

namespace Io {
    juniper::unit setPinMode(uint16_t pin, Io::mode m);
}

namespace Io {
    Prelude::sig<juniper::unit> risingEdge(Prelude::sig<Io::pinState> sig, juniper::shared_ptr<Io::pinState> prevState);
}

namespace Io {
    Prelude::sig<juniper::unit> fallingEdge(Prelude::sig<Io::pinState> sig, juniper::shared_ptr<Io::pinState> prevState);
}

namespace Io {
    Prelude::sig<Io::pinState> edge(Prelude::sig<Io::pinState> sig, juniper::shared_ptr<Io::pinState> prevState);
}

namespace Maybe {
    template<typename t905, typename t906, typename t907>
    Prelude::maybe<t906> map(juniper::function<t907, t906(t905)> f, Prelude::maybe<t905> maybeVal);
}

namespace Maybe {
    template<typename t917>
    t917 get(Prelude::maybe<t917> maybeVal);
}

namespace Maybe {
    template<typename t919>
    bool isJust(Prelude::maybe<t919> maybeVal);
}

namespace Maybe {
    template<typename t921>
    bool isNothing(Prelude::maybe<t921> maybeVal);
}

namespace Maybe {
    template<typename t926>
    uint8_t count(Prelude::maybe<t926> maybeVal);
}

namespace Maybe {
    template<typename t930, typename t931, typename t932>
    t931 foldl(juniper::function<t932, t931(t930, t931)> f, t931 initState, Prelude::maybe<t930> maybeVal);
}

namespace Maybe {
    template<typename t938, typename t939, typename t940>
    t939 fodlr(juniper::function<t940, t939(t938, t939)> f, t939 initState, Prelude::maybe<t938> maybeVal);
}

namespace Maybe {
    template<typename t947, typename t948>
    juniper::unit iter(juniper::function<t948, juniper::unit(t947)> f, Prelude::maybe<t947> maybeVal);
}

namespace Time {
    juniper::unit wait(uint32_t time);
}

namespace Time {
    uint32_t now();
}

namespace Time {
    juniper::shared_ptr<juniper::records::recordt_1<uint32_t>> state();
}

namespace Time {
    Prelude::sig<uint32_t> every(uint32_t interval, juniper::shared_ptr<juniper::records::recordt_1<uint32_t>> state);
}

namespace Math {
    double degToRad(double degrees);
}

namespace Math {
    double radToDeg(double radians);
}

namespace Math {
    double acos_(double x);
}

namespace Math {
    double asin_(double x);
}

namespace Math {
    double atan_(double x);
}

namespace Math {
    double atan2_(double y, double x);
}

namespace Math {
    double cos_(double x);
}

namespace Math {
    double cosh_(double x);
}

namespace Math {
    double sin_(double x);
}

namespace Math {
    double sinh_(double x);
}

namespace Math {
    double tan_(double x);
}

namespace Math {
    double tanh_(double x);
}

namespace Math {
    double exp_(double x);
}

namespace Math {
    juniper::tuple2<double,int16_t> frexp_(double x);
}

namespace Math {
    double ldexp_(double x, int16_t exponent);
}

namespace Math {
    double log_(double x);
}

namespace Math {
    double log10_(double x);
}

namespace Math {
    juniper::tuple2<double,double> modf_(double x);
}

namespace Math {
    double pow_(double x, double y);
}

namespace Math {
    double sqrt_(double x);
}

namespace Math {
    double ceil_(double x);
}

namespace Math {
    double fabs_(double x);
}

namespace Math {
    double floor_(double x);
}

namespace Math {
    double fmod_(double x, double y);
}

namespace Math {
    double round_(double x);
}

namespace Math {
    template<typename t1009>
    t1009 min_(t1009 x, t1009 y);
}

namespace Math {
    template<typename a>
    a max_(a x, a y);
}

namespace Math {
    double mapRange(double x, double a1, double a2, double b1, double b2);
}

namespace Math {
    template<typename t1013>
    t1013 clamp(t1013 x, t1013 min, t1013 max);
}

namespace Math {
    template<typename t1018>
    int8_t sign(t1018 n);
}

namespace Button {
    juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>> state();
}

namespace Button {
    Prelude::sig<Io::pinState> debounceDelay(Prelude::sig<Io::pinState> incoming, uint16_t delay, juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>> buttonState);
}

namespace Button {
    Prelude::sig<Io::pinState> debounce(Prelude::sig<Io::pinState> incoming, juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>> buttonState);
}

namespace Vector {
    template<typename t1060, int c76>
    juniper::records::recordt_3<juniper::array<t1060, c76>> make(juniper::array<t1060, c76> d);
}

namespace Vector {
    template<typename t1062, int c77>
    t1062 get(uint32_t i, juniper::records::recordt_3<juniper::array<t1062, c77>> v);
}

namespace Vector {
    template<typename t1072, int c79>
    juniper::records::recordt_3<juniper::array<t1072, c79>> add(juniper::records::recordt_3<juniper::array<t1072, c79>> v1, juniper::records::recordt_3<juniper::array<t1072, c79>> v2);
}

namespace Vector {
    template<typename t1076, int c83>
    juniper::records::recordt_3<juniper::array<t1076, c83>> zero();
}

namespace Vector {
    template<typename t1085, int c84>
    juniper::records::recordt_3<juniper::array<t1085, c84>> subtract(juniper::records::recordt_3<juniper::array<t1085, c84>> v1, juniper::records::recordt_3<juniper::array<t1085, c84>> v2);
}

namespace Vector {
    template<typename t1093, int c88>
    juniper::records::recordt_3<juniper::array<t1093, c88>> scale(t1093 scalar, juniper::records::recordt_3<juniper::array<t1093, c88>> v);
}

namespace Vector {
    template<typename t1104, int c91>
    t1104 dot(juniper::records::recordt_3<juniper::array<t1104, c91>> v1, juniper::records::recordt_3<juniper::array<t1104, c91>> v2);
}

namespace Vector {
    template<typename t1113, int c94>
    t1113 magnitude2(juniper::records::recordt_3<juniper::array<t1113, c94>> v);
}

namespace Vector {
    template<typename t1115, int c97>
    double magnitude(juniper::records::recordt_3<juniper::array<t1115, c97>> v);
}

namespace Vector {
    template<typename t1133, int c98>
    juniper::records::recordt_3<juniper::array<t1133, c98>> multiply(juniper::records::recordt_3<juniper::array<t1133, c98>> u, juniper::records::recordt_3<juniper::array<t1133, c98>> v);
}

namespace Vector {
    template<typename t1144, int c102>
    juniper::records::recordt_3<juniper::array<t1144, c102>> normalize(juniper::records::recordt_3<juniper::array<t1144, c102>> v);
}

namespace Vector {
    template<typename t1157, int c105>
    double angle(juniper::records::recordt_3<juniper::array<t1157, c105>> v1, juniper::records::recordt_3<juniper::array<t1157, c105>> v2);
}

namespace Vector {
    template<typename t1175>
    juniper::records::recordt_3<juniper::array<t1175, 3>> cross(juniper::records::recordt_3<juniper::array<t1175, 3>> u, juniper::records::recordt_3<juniper::array<t1175, 3>> v);
}

namespace Vector {
    template<typename t1202, int c118>
    juniper::records::recordt_3<juniper::array<t1202, c118>> project(juniper::records::recordt_3<juniper::array<t1202, c118>> a, juniper::records::recordt_3<juniper::array<t1202, c118>> b);
}

namespace Vector {
    template<typename t1215, int c119>
    juniper::records::recordt_3<juniper::array<t1215, c119>> projectPlane(juniper::records::recordt_3<juniper::array<t1215, c119>> a, juniper::records::recordt_3<juniper::array<t1215, c119>> m);
}

namespace CharList {
    template<int c120>
    juniper::records::recordt_0<juniper::array<uint8_t, c120>, uint32_t> toUpper(juniper::records::recordt_0<juniper::array<uint8_t, c120>, uint32_t> str);
}

namespace CharList {
    template<int c121>
    juniper::records::recordt_0<juniper::array<uint8_t, c121>, uint32_t> toLower(juniper::records::recordt_0<juniper::array<uint8_t, c121>, uint32_t> str);
}

namespace CharList {
    template<int c122>
    juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t> i32ToCharList(int32_t m);
}

namespace CharList {
    template<int c123>
    uint32_t length(juniper::records::recordt_0<juniper::array<uint8_t, c123>, uint32_t> s);
}

namespace CharList {
    template<int c124, int c125, int c126>
    juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t> concat(juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c124)>, uint32_t> sA, juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c125)>, uint32_t> sB);
}

namespace CharList {
    template<int c133, int c134>
    juniper::records::recordt_0<juniper::array<uint8_t, ((1)+(c133))+(c134)>, uint32_t> safeConcat(juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c133)>, uint32_t> sA, juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c134)>, uint32_t> sB);
}

namespace Random {
    int32_t random_(int32_t low, int32_t high);
}

namespace Random {
    juniper::unit seed(uint32_t n);
}

namespace Random {
    template<typename t1279, int c138>
    t1279 choice(juniper::records::recordt_0<juniper::array<t1279, c138>, uint32_t> lst);
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> hsvToRgb(juniper::records::recordt_6<float, float, float> color);
}

namespace Color {
    uint16_t rgbToRgb565(juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> color);
}

namespace Blink {
    juniper::unit setup();
}

namespace Blink {
    juniper::unit loop();
}

namespace Prelude {
    void * extractptr(juniper::smartpointer p) {
        return (([&]() -> void * {
            void * ret;
            
            (([&]() -> juniper::unit {
                ret = p.get()->get();
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    template<typename t10, typename t6, typename t7, typename t8, typename t9>
    juniper::function<juniper::closures::closuret_0<juniper::function<t8, t7(t6)>, juniper::function<t9, t6(t10)>>, t7(t10)> compose(juniper::function<t8, t7(t6)> f, juniper::function<t9, t6(t10)> g) {
        return (([&]() -> juniper::function<juniper::closures::closuret_0<juniper::function<t8, t7(t6)>, juniper::function<t9, t6(t10)>>, t7(t10)> {
            using a = t10;
            using b = t6;
            using c = t7;
            using closureF = t8;
            using closureG = t9;
            return juniper::function<juniper::closures::closuret_0<juniper::function<t8, t7(t6)>, juniper::function<t9, t6(t10)>>, t7(t10)>(juniper::closures::closuret_0<juniper::function<t8, t7(t6)>, juniper::function<t9, t6(t10)>>(f, g), [](juniper::closures::closuret_0<juniper::function<t8, t7(t6)>, juniper::function<t9, t6(t10)>>& junclosure, t10 x) -> t7 { 
                juniper::function<t8, t7(t6)>& f = junclosure.f;
                juniper::function<t9, t6(t10)>& g = junclosure.g;
                return f(g(x));
             });
        })());
    }
}

namespace Prelude {
    template<typename t22, typename t23, typename t20, typename t21>
    juniper::function<juniper::closures::closuret_1<juniper::function<t21, t20(t22, t23)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>, t20(t23)>(t22)> curry(juniper::function<t21, t20(t22, t23)> f) {
        return (([&]() -> juniper::function<juniper::closures::closuret_1<juniper::function<t21, t20(t22, t23)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>, t20(t23)>(t22)> {
            using a = t22;
            using b = t23;
            using c = t20;
            using closure = t21;
            return juniper::function<juniper::closures::closuret_1<juniper::function<t21, t20(t22, t23)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>, t20(t23)>(t22)>(juniper::closures::closuret_1<juniper::function<t21, t20(t22, t23)>>(f), [](juniper::closures::closuret_1<juniper::function<t21, t20(t22, t23)>>& junclosure, t22 valueA) -> juniper::function<juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>, t20(t23)> { 
                juniper::function<t21, t20(t22, t23)>& f = junclosure.f;
                return juniper::function<juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>, t20(t23)>(juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>(f, valueA), [](juniper::closures::closuret_2<juniper::function<t21, t20(t22, t23)>, t22>& junclosure, t23 valueB) -> t20 { 
                    juniper::function<t21, t20(t22, t23)>& f = junclosure.f;
                    t22& valueA = junclosure.valueA;
                    return f(valueA, valueB);
                 });
             });
        })());
    }
}

namespace Prelude {
    template<typename t34, typename t35, typename t31, typename t32, typename t33>
    juniper::function<juniper::closures::closuret_1<juniper::function<t32, juniper::function<t33, t31(t35)>(t34)>>, t31(t34, t35)> uncurry(juniper::function<t32, juniper::function<t33, t31(t35)>(t34)> f) {
        return (([&]() -> juniper::function<juniper::closures::closuret_1<juniper::function<t32, juniper::function<t33, t31(t35)>(t34)>>, t31(t34, t35)> {
            using a = t34;
            using b = t35;
            using c = t31;
            using closureA = t32;
            using closureB = t33;
            return juniper::function<juniper::closures::closuret_1<juniper::function<t32, juniper::function<t33, t31(t35)>(t34)>>, t31(t34,t35)>(juniper::closures::closuret_1<juniper::function<t32, juniper::function<t33, t31(t35)>(t34)>>(f), [](juniper::closures::closuret_1<juniper::function<t32, juniper::function<t33, t31(t35)>(t34)>>& junclosure, t34 valueA, t35 valueB) -> t31 { 
                juniper::function<t32, juniper::function<t33, t31(t35)>(t34)>& f = junclosure.f;
                return f(valueA)(valueB);
             });
        })());
    }
}

namespace Prelude {
    template<typename t48, typename t49, typename t50, typename t46, typename t47>
    juniper::function<juniper::closures::closuret_1<juniper::function<t47, t46(t48, t49, t50)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>, juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)>(t49)>(t48)> curry3(juniper::function<t47, t46(t48, t49, t50)> f) {
        return (([&]() -> juniper::function<juniper::closures::closuret_1<juniper::function<t47, t46(t48, t49, t50)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>, juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)>(t49)>(t48)> {
            using a = t48;
            using b = t49;
            using c = t50;
            using d = t46;
            using closureF = t47;
            return juniper::function<juniper::closures::closuret_1<juniper::function<t47, t46(t48, t49, t50)>>, juniper::function<juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>, juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)>(t49)>(t48)>(juniper::closures::closuret_1<juniper::function<t47, t46(t48, t49, t50)>>(f), [](juniper::closures::closuret_1<juniper::function<t47, t46(t48, t49, t50)>>& junclosure, t48 valueA) -> juniper::function<juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>, juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)>(t49)> { 
                juniper::function<t47, t46(t48, t49, t50)>& f = junclosure.f;
                return juniper::function<juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>, juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)>(t49)>(juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>(f, valueA), [](juniper::closures::closuret_2<juniper::function<t47, t46(t48, t49, t50)>, t48>& junclosure, t49 valueB) -> juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)> { 
                    juniper::function<t47, t46(t48, t49, t50)>& f = junclosure.f;
                    t48& valueA = junclosure.valueA;
                    return juniper::function<juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>, t46(t50)>(juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>(f, valueA, valueB), [](juniper::closures::closuret_3<juniper::function<t47, t46(t48, t49, t50)>, t48, t49>& junclosure, t50 valueC) -> t46 { 
                        juniper::function<t47, t46(t48, t49, t50)>& f = junclosure.f;
                        t48& valueA = junclosure.valueA;
                        t49& valueB = junclosure.valueB;
                        return f(valueA, valueB, valueC);
                     });
                 });
             });
        })());
    }
}

namespace Prelude {
    template<typename t64, typename t65, typename t66, typename t60, typename t61, typename t62, typename t63>
    juniper::function<juniper::closures::closuret_1<juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)>>, t60(t64, t65, t66)> uncurry3(juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)> f) {
        return (([&]() -> juniper::function<juniper::closures::closuret_1<juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)>>, t60(t64, t65, t66)> {
            using a = t64;
            using b = t65;
            using c = t66;
            using d = t60;
            using closureA = t61;
            using closureB = t62;
            using closureC = t63;
            return juniper::function<juniper::closures::closuret_1<juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)>>, t60(t64,t65,t66)>(juniper::closures::closuret_1<juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)>>(f), [](juniper::closures::closuret_1<juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)>>& junclosure, t64 valueA, t65 valueB, t66 valueC) -> t60 { 
                juniper::function<t61, juniper::function<t62, juniper::function<t63, t60(t66)>(t65)>(t64)>& f = junclosure.f;
                return f(valueA)(valueB)(valueC);
             });
        })());
    }
}

namespace Prelude {
    template<typename t77>
    bool eq(t77 x, t77 y) {
        return (([&]() -> bool {
            using a = t77;
            return (x == y);
        })());
    }
}

namespace Prelude {
    template<typename t80>
    bool neq(t80 x, t80 y) {
        return (x != y);
    }
}

namespace Prelude {
    template<typename t82, typename t83>
    bool gt(t82 x, t83 y) {
        return (x > y);
    }
}

namespace Prelude {
    template<typename t85, typename t86>
    bool geq(t85 x, t86 y) {
        return (x >= y);
    }
}

namespace Prelude {
    template<typename t88, typename t89>
    bool lt(t88 x, t89 y) {
        return (x < y);
    }
}

namespace Prelude {
    template<typename t91, typename t92>
    bool leq(t91 x, t92 y) {
        return (x <= y);
    }
}

namespace Prelude {
    bool notf(bool x) {
        return !(x);
    }
}

namespace Prelude {
    bool andf(bool x, bool y) {
        return (x && y);
    }
}

namespace Prelude {
    bool orf(bool x, bool y) {
        return (x || y);
    }
}

namespace Prelude {
    template<typename t102, typename t103, typename t104>
    t103 apply(juniper::function<t104, t103(t102)> f, t102 x) {
        return (([&]() -> t103 {
            using a = t102;
            using b = t103;
            using closure = t104;
            return f(x);
        })());
    }
}

namespace Prelude {
    template<typename t109, typename t110, typename t111, typename t112>
    t111 apply2(juniper::function<t112, t111(t109, t110)> f, juniper::tuple2<t109,t110> tup) {
        return (([&]() -> t111 {
            using a = t109;
            using b = t110;
            using c = t111;
            using closure = t112;
            return (([&]() -> t111 {
                juniper::tuple2<t109,t110> guid0 = tup;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t110 b = (guid0).e2;
                t109 a = (guid0).e1;
                
                return f(a, b);
            })());
        })());
    }
}

namespace Prelude {
    template<typename t122, typename t123, typename t124, typename t125, typename t126>
    t125 apply3(juniper::function<t126, t125(t122, t123, t124)> f, juniper::tuple3<t122,t123,t124> tup) {
        return (([&]() -> t125 {
            using a = t122;
            using b = t123;
            using c = t124;
            using d = t125;
            using closure = t126;
            return (([&]() -> t125 {
                juniper::tuple3<t122,t123,t124> guid1 = tup;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t124 c = (guid1).e3;
                t123 b = (guid1).e2;
                t122 a = (guid1).e1;
                
                return f(a, b, c);
            })());
        })());
    }
}

namespace Prelude {
    template<typename t139, typename t140, typename t141, typename t142, typename t143, typename t144>
    t143 apply4(juniper::function<t144, t143(t139, t140, t141, t142)> f, juniper::tuple4<t139,t140,t141,t142> tup) {
        return (([&]() -> t143 {
            using a = t139;
            using b = t140;
            using c = t141;
            using d = t142;
            using e = t143;
            using closure = t144;
            return (([&]() -> t143 {
                juniper::tuple4<t139,t140,t141,t142> guid2 = tup;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t142 d = (guid2).e4;
                t141 c = (guid2).e3;
                t140 b = (guid2).e2;
                t139 a = (guid2).e1;
                
                return f(a, b, c, d);
            })());
        })());
    }
}

namespace Prelude {
    template<typename t160, typename t161>
    t160 fst(juniper::tuple2<t160,t161> tup) {
        return (([&]() -> t160 {
            using a = t160;
            using b = t161;
            return (([&]() -> t160 {
                juniper::tuple2<t160,t161> guid3 = tup;
                return (true ? 
                    (([&]() -> t160 {
                        t160 x = (guid3).e1;
                        return x;
                    })())
                :
                    juniper::quit<t160>());
            })());
        })());
    }
}

namespace Prelude {
    template<typename t165, typename t166>
    t166 snd(juniper::tuple2<t165,t166> tup) {
        return (([&]() -> t166 {
            using a = t165;
            using b = t166;
            return (([&]() -> t166 {
                juniper::tuple2<t165,t166> guid4 = tup;
                return (true ? 
                    (([&]() -> t166 {
                        t166 x = (guid4).e2;
                        return x;
                    })())
                :
                    juniper::quit<t166>());
            })());
        })());
    }
}

namespace Prelude {
    template<typename t170>
    t170 add(t170 numA, t170 numB) {
        return (([&]() -> t170 {
            using a = t170;
            return (numA + numB);
        })());
    }
}

namespace Prelude {
    template<typename t172>
    t172 sub(t172 numA, t172 numB) {
        return (([&]() -> t172 {
            using a = t172;
            return (numA - numB);
        })());
    }
}

namespace Prelude {
    template<typename t174>
    t174 mul(t174 numA, t174 numB) {
        return (([&]() -> t174 {
            using a = t174;
            return (numA * numB);
        })());
    }
}

namespace Prelude {
    template<typename t176>
    t176 div(t176 numA, t176 numB) {
        return (([&]() -> t176 {
            using a = t176;
            return (numA / numB);
        })());
    }
}

namespace Prelude {
    template<typename t179, typename t180>
    juniper::tuple2<t180,t179> swap(juniper::tuple2<t179,t180> tup) {
        return (([&]() -> juniper::tuple2<t180,t179> {
            juniper::tuple2<t179,t180> guid5 = tup;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            t180 beta = (guid5).e2;
            t179 alpha = (guid5).e1;
            
            return (juniper::tuple2<t180,t179>{beta, alpha});
        })());
    }
}

namespace Prelude {
    template<typename t184, typename t185, typename t186>
    t184 until(juniper::function<t185, bool(t184)> p, juniper::function<t186, t184(t184)> f, t184 a0) {
        return (([&]() -> t184 {
            using a = t184;
            using closureP = t185;
            using closureF = t186;
            return (([&]() -> t184 {
                t184 guid6 = a0;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t184 a = guid6;
                
                (([&]() -> juniper::unit {
                    while (!(p(a))) {
                        (([&]() -> juniper::unit {
                            (a = f(a));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return a;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t194>
    juniper::unit ignore(t194 val) {
        return (([&]() -> juniper::unit {
            using a = t194;
            return juniper::unit();
        })());
    }
}

namespace Prelude {
    uint16_t u8ToU16(uint8_t n) {
        return (([&]() -> uint16_t {
            uint16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint32_t u8ToU32(uint8_t n) {
        return (([&]() -> uint32_t {
            uint32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int8_t u8ToI8(uint8_t n) {
        return (([&]() -> int8_t {
            int8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int16_t u8ToI16(uint8_t n) {
        return (([&]() -> int16_t {
            int16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int32_t u8ToI32(uint8_t n) {
        return (([&]() -> int32_t {
            int32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    float u8ToFloat(uint8_t n) {
        return (([&]() -> float {
            float ret;
            
            (([&]() -> juniper::unit {
                ret = (float) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    double u8ToDouble(uint8_t n) {
        return (([&]() -> double {
            double ret;
            
            (([&]() -> juniper::unit {
                ret = (double) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint8_t u16ToU8(uint16_t n) {
        return (([&]() -> uint8_t {
            uint8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint32_t u16ToU32(uint16_t n) {
        return (([&]() -> uint32_t {
            uint32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int8_t u16ToI8(uint16_t n) {
        return (([&]() -> int8_t {
            int8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int16_t u16ToI16(uint16_t n) {
        return (([&]() -> int16_t {
            int16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int32_t u16ToI32(uint16_t n) {
        return (([&]() -> int32_t {
            int32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    float u16ToFloat(uint16_t n) {
        return (([&]() -> float {
            float ret;
            
            (([&]() -> juniper::unit {
                ret = (float) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    double u16ToDouble(uint16_t n) {
        return (([&]() -> double {
            double ret;
            
            (([&]() -> juniper::unit {
                ret = (double) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint8_t u32ToU8(uint32_t n) {
        return (([&]() -> uint8_t {
            uint8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint16_t u32ToU16(uint32_t n) {
        return (([&]() -> uint16_t {
            uint16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int8_t u32ToI8(uint32_t n) {
        return (([&]() -> int8_t {
            int8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int16_t u32ToI16(uint32_t n) {
        return (([&]() -> int16_t {
            int16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int32_t u32ToI32(uint32_t n) {
        return (([&]() -> int32_t {
            int32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    float u32ToFloat(uint32_t n) {
        return (([&]() -> float {
            float ret;
            
            (([&]() -> juniper::unit {
                ret = (float) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    double u32ToDouble(uint32_t n) {
        return (([&]() -> double {
            double ret;
            
            (([&]() -> juniper::unit {
                ret = (double) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint8_t i8ToU8(int8_t n) {
        return (([&]() -> uint8_t {
            uint8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint16_t i8ToU16(int8_t n) {
        return (([&]() -> uint16_t {
            uint16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint32_t i8ToU32(int8_t n) {
        return (([&]() -> uint32_t {
            uint32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int16_t i8ToI16(int8_t n) {
        return (([&]() -> int16_t {
            int16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int32_t i8ToI32(int8_t n) {
        return (([&]() -> int32_t {
            int32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    float i8ToFloat(int8_t n) {
        return (([&]() -> float {
            float ret;
            
            (([&]() -> juniper::unit {
                ret = (float) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    double i8ToDouble(int8_t n) {
        return (([&]() -> double {
            double ret;
            
            (([&]() -> juniper::unit {
                ret = (double) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint8_t i16ToU8(int16_t n) {
        return (([&]() -> uint8_t {
            uint8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint16_t i16ToU16(int16_t n) {
        return (([&]() -> uint16_t {
            uint16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint32_t i16ToU32(int16_t n) {
        return (([&]() -> uint32_t {
            uint32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int8_t i16ToI8(int16_t n) {
        return (([&]() -> int8_t {
            int8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int32_t i16ToI32(int16_t n) {
        return (([&]() -> int32_t {
            int32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    float i16ToFloat(int16_t n) {
        return (([&]() -> float {
            float ret;
            
            (([&]() -> juniper::unit {
                ret = (float) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    double i16ToDouble(int16_t n) {
        return (([&]() -> double {
            double ret;
            
            (([&]() -> juniper::unit {
                ret = (double) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint8_t i32ToU8(int32_t n) {
        return (([&]() -> uint8_t {
            uint8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint16_t i32ToU16(int32_t n) {
        return (([&]() -> uint16_t {
            uint16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint32_t i32ToU32(int32_t n) {
        return (([&]() -> uint32_t {
            uint32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int8_t i32ToI8(int32_t n) {
        return (([&]() -> int8_t {
            int8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int16_t i32ToI16(int32_t n) {
        return (([&]() -> int16_t {
            int16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    float i32ToFloat(int32_t n) {
        return (([&]() -> float {
            float ret;
            
            (([&]() -> juniper::unit {
                ret = (float) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    double i32ToDouble(int32_t n) {
        return (([&]() -> double {
            double ret;
            
            (([&]() -> juniper::unit {
                ret = (double) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint8_t floatToU8(float n) {
        return (([&]() -> uint8_t {
            uint8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint16_t floatToU16(float n) {
        return (([&]() -> uint16_t {
            uint16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint32_t floatToU32(float n) {
        return (([&]() -> uint32_t {
            uint32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int8_t floatToI8(float n) {
        return (([&]() -> int8_t {
            int8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int16_t floatToI16(float n) {
        return (([&]() -> int16_t {
            int16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int32_t floatToI32(float n) {
        return (([&]() -> int32_t {
            int32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    double floatToDouble(float n) {
        return (([&]() -> double {
            double ret;
            
            (([&]() -> juniper::unit {
                ret = (double) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint8_t doubleToU8(double n) {
        return (([&]() -> uint8_t {
            uint8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint16_t doubleToU16(double n) {
        return (([&]() -> uint16_t {
            uint16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    uint32_t doubleToU32(double n) {
        return (([&]() -> uint32_t {
            uint32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (uint32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int8_t doubleToI8(double n) {
        return (([&]() -> int8_t {
            int8_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int8_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int16_t doubleToI16(double n) {
        return (([&]() -> int16_t {
            int16_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int16_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    int32_t doubleToI32(double n) {
        return (([&]() -> int32_t {
            int32_t ret;
            
            (([&]() -> juniper::unit {
                ret = (int32_t) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    float doubleToFloat(double n) {
        return (([&]() -> float {
            float ret;
            
            (([&]() -> juniper::unit {
                ret = (float) n;
                return {};
            })());
            return ret;
        })());
    }
}

namespace Prelude {
    template<typename t252>
    uint8_t toUInt8(t252 n) {
        return (([&]() -> uint8_t {
            using t = t252;
            return (([&]() -> uint8_t {
                uint8_t ret;
                
                (([&]() -> juniper::unit {
                    ret = (uint8_t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t254>
    int8_t toInt8(t254 n) {
        return (([&]() -> int8_t {
            using t = t254;
            return (([&]() -> int8_t {
                int8_t ret;
                
                (([&]() -> juniper::unit {
                    ret = (int8_t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t256>
    uint16_t toUInt16(t256 n) {
        return (([&]() -> uint16_t {
            using t = t256;
            return (([&]() -> uint16_t {
                uint16_t ret;
                
                (([&]() -> juniper::unit {
                    ret = (uint16_t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t258>
    int16_t toInt16(t258 n) {
        return (([&]() -> int16_t {
            using t = t258;
            return (([&]() -> int16_t {
                int16_t ret;
                
                (([&]() -> juniper::unit {
                    ret = (int16_t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t260>
    uint32_t toUInt32(t260 n) {
        return (([&]() -> uint32_t {
            using t = t260;
            return (([&]() -> uint32_t {
                uint32_t ret;
                
                (([&]() -> juniper::unit {
                    ret = (uint32_t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t262>
    int32_t toInt32(t262 n) {
        return (([&]() -> int32_t {
            using t = t262;
            return (([&]() -> int32_t {
                int32_t ret;
                
                (([&]() -> juniper::unit {
                    ret = (int32_t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t264>
    float toFloat(t264 n) {
        return (([&]() -> float {
            using t = t264;
            return (([&]() -> float {
                float ret;
                
                (([&]() -> juniper::unit {
                    ret = (float) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t266>
    double toDouble(t266 n) {
        return (([&]() -> double {
            using t = t266;
            return (([&]() -> double {
                double ret;
                
                (([&]() -> juniper::unit {
                    ret = (double) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t268>
    t268 fromUInt8(uint8_t n) {
        return (([&]() -> t268 {
            using t = t268;
            return (([&]() -> t268 {
                t268 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t270>
    t270 fromInt8(int8_t n) {
        return (([&]() -> t270 {
            using t = t270;
            return (([&]() -> t270 {
                t270 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t272>
    t272 fromUInt16(uint16_t n) {
        return (([&]() -> t272 {
            using t = t272;
            return (([&]() -> t272 {
                t272 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t274>
    t274 fromInt16(int16_t n) {
        return (([&]() -> t274 {
            using t = t274;
            return (([&]() -> t274 {
                t274 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t276>
    t276 fromUInt32(uint32_t n) {
        return (([&]() -> t276 {
            using t = t276;
            return (([&]() -> t276 {
                t276 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t278>
    t278 fromInt32(int32_t n) {
        return (([&]() -> t278 {
            using t = t278;
            return (([&]() -> t278 {
                t278 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t280>
    t280 fromFloat(float n) {
        return (([&]() -> t280 {
            using t = t280;
            return (([&]() -> t280 {
                t280 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t282>
    t282 fromDouble(double n) {
        return (([&]() -> t282 {
            using t = t282;
            return (([&]() -> t282 {
                t282 ret;
                
                (([&]() -> juniper::unit {
                    ret = (t) n;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace Prelude {
    template<typename t284, typename t285>
    t285 cast(t284 x) {
        return (([&]() -> t285 {
            using a = t284;
            using b = t285;
            return (([&]() -> t285 {
                t285 ret;
                
                (([&]() -> juniper::unit {
                    ret = (b) x;
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace List {
    template<typename t292, typename t296, typename t289, int c4>
    juniper::records::recordt_0<juniper::array<t296, c4>, uint32_t> map(juniper::function<t289, t296(t292)> f, juniper::records::recordt_0<juniper::array<t292, c4>, uint32_t> lst) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t296, c4>, uint32_t> {
            constexpr int32_t n = c4;
            return (([&]() -> juniper::records::recordt_0<juniper::array<t296, c4>, uint32_t> {
                juniper::array<t296, c4> guid7 = (juniper::array<t296, c4>());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::array<t296, c4> ret = guid7;
                
                (([&]() -> juniper::unit {
                    uint32_t guid8 = ((uint32_t) 0);
                    uint32_t guid9 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid8; i <= guid9; i++) {
                        (([&]() -> juniper::unit {
                            ((ret)[i] = f(((lst).data)[i]));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return (([&]() -> juniper::records::recordt_0<juniper::array<t296, c4>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<t296, c4>, uint32_t> guid10;
                    guid10.data = ret;
                    guid10.length = (lst).length;
                    return guid10;
                })());
            })());
        })());
    }
}

namespace List {
    template<typename t304, typename t300, typename t301, int c7>
    t300 foldl(juniper::function<t301, t300(t304, t300)> f, t300 initState, juniper::records::recordt_0<juniper::array<t304, c7>, uint32_t> lst) {
        return (([&]() -> t300 {
            constexpr int32_t n = c7;
            return (([&]() -> t300 {
                t300 guid11 = initState;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t300 s = guid11;
                
                (([&]() -> juniper::unit {
                    uint32_t guid12 = ((uint32_t) 0);
                    uint32_t guid13 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid12; i <= guid13; i++) {
                        (([&]() -> juniper::unit {
                            (s = f(((lst).data)[i], s));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return s;
            })());
        })());
    }
}

namespace List {
    template<typename t315, typename t311, typename t312, int c9>
    t311 foldr(juniper::function<t312, t311(t315, t311)> f, t311 initState, juniper::records::recordt_0<juniper::array<t315, c9>, uint32_t> lst) {
        return (([&]() -> t311 {
            constexpr int32_t n = c9;
            return (([&]() -> t311 {
                t311 guid14 = initState;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t311 s = guid14;
                
                (([&]() -> juniper::unit {
                    uint32_t guid15 = ((lst).length - ((uint32_t) 1));
                    uint32_t guid16 = ((uint32_t) 0);
                    for (uint32_t i = guid15; i >= guid16; i--) {
                        (([&]() -> juniper::unit {
                            (s = f(((lst).data)[i], s));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return s;
            })());
        })());
    }
}

namespace List {
    template<typename t331, int c11, int c12, int c13>
    juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t> append(juniper::records::recordt_0<juniper::array<t331, c11>, uint32_t> lstA, juniper::records::recordt_0<juniper::array<t331, c12>, uint32_t> lstB) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t> {
            constexpr int32_t aCap = c11;
            constexpr int32_t bCap = c12;
            constexpr int32_t retCap = c13;
            return (([&]() -> juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t> {
                uint32_t guid17 = ((uint32_t) 0);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                uint32_t j = guid17;
                
                juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t> guid18 = (([&]() -> juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t> guid19;
                    guid19.data = (juniper::array<t331, c13>());
                    guid19.length = ((lstA).length + (lstB).length);
                    return guid19;
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_0<juniper::array<t331, c13>, uint32_t> out = guid18;
                
                (([&]() -> juniper::unit {
                    uint32_t guid20 = ((uint32_t) 0);
                    uint32_t guid21 = ((lstA).length - ((uint32_t) 1));
                    for (uint32_t i = guid20; i <= guid21; i++) {
                        (([&]() -> juniper::unit {
                            (((out).data)[j] = ((lstA).data)[i]);
                            (j = (j + ((uint32_t) 1)));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                (([&]() -> juniper::unit {
                    uint32_t guid22 = ((uint32_t) 0);
                    uint32_t guid23 = ((lstB).length - ((uint32_t) 1));
                    for (uint32_t i = guid22; i <= guid23; i++) {
                        (([&]() -> juniper::unit {
                            (((out).data)[j] = ((lstB).data)[i]);
                            (j = (j + ((uint32_t) 1)));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return out;
            })());
        })());
    }
}

namespace List {
    template<typename t335, int c18>
    t335 nth(uint32_t i, juniper::records::recordt_0<juniper::array<t335, c18>, uint32_t> lst) {
        return (([&]() -> t335 {
            constexpr int32_t n = c18;
            return ((i < (lst).length) ? 
                ((lst).data)[i]
            :
                juniper::quit<t335>());
        })());
    }
}

namespace List {
    template<typename t348, int c20, int c21>
    juniper::records::recordt_0<juniper::array<t348, (c21)*(c20)>, uint32_t> flattenSafe(juniper::records::recordt_0<juniper::array<juniper::records::recordt_0<juniper::array<t348, c20>, uint32_t>, c21>, uint32_t> listOfLists) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t348, (c21)*(c20)>, uint32_t> {
            constexpr int32_t m = c20;
            constexpr int32_t n = c21;
            return (([&]() -> juniper::records::recordt_0<juniper::array<t348, (c21)*(c20)>, uint32_t> {
                juniper::array<t348, (c21)*(c20)> guid24 = (juniper::array<t348, (c21)*(c20)>());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::array<t348, (c21)*(c20)> ret = guid24;
                
                uint32_t guid25 = ((uint32_t) 0);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                uint32_t index = guid25;
                
                (([&]() -> juniper::unit {
                    uint32_t guid26 = ((uint32_t) 0);
                    uint32_t guid27 = ((listOfLists).length - ((uint32_t) 1));
                    for (uint32_t i = guid26; i <= guid27; i++) {
                        (([&]() -> juniper::unit {
                            uint32_t guid28 = ((uint32_t) 0);
                            uint32_t guid29 = ((((listOfLists).data)[i]).length - ((uint32_t) 1));
                            for (uint32_t j = guid28; j <= guid29; j++) {
                                (([&]() -> juniper::unit {
                                    ((ret)[index] = ((((listOfLists).data)[i]).data)[j]);
                                    (index = (index + ((uint32_t) 1)));
                                    return juniper::unit();
                                })());
                            }
                            return {};
                        })());
                    }
                    return {};
                })());
                return (([&]() -> juniper::records::recordt_0<juniper::array<t348, (c21)*(c20)>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<t348, (c21)*(c20)>, uint32_t> guid30;
                    guid30.data = ret;
                    guid30.length = index;
                    return guid30;
                })());
            })());
        })());
    }
}

namespace List {
    template<typename t354, int c26, int c27>
    juniper::records::recordt_0<juniper::array<t354, c27>, uint32_t> resize(juniper::records::recordt_0<juniper::array<t354, c26>, uint32_t> lst) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t354, c27>, uint32_t> {
            constexpr int32_t n = c26;
            constexpr int32_t m = c27;
            return (([&]() -> juniper::records::recordt_0<juniper::array<t354, c27>, uint32_t> {
                juniper::array<t354, c27> guid31 = (juniper::array<t354, c27>());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::array<t354, c27> ret = guid31;
                
                (([&]() -> juniper::unit {
                    uint32_t guid32 = ((uint32_t) 0);
                    uint32_t guid33 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid32; i <= guid33; i++) {
                        (([&]() -> juniper::unit {
                            ((ret)[i] = ((lst).data)[i]);
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return (([&]() -> juniper::records::recordt_0<juniper::array<t354, c27>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<t354, c27>, uint32_t> guid34;
                    guid34.data = ret;
                    guid34.length = (lst).length;
                    return guid34;
                })());
            })());
        })());
    }
}

namespace List {
    template<typename t362, typename t359, int c30>
    bool all(juniper::function<t359, bool(t362)> pred, juniper::records::recordt_0<juniper::array<t362, c30>, uint32_t> lst) {
        return (([&]() -> bool {
            constexpr int32_t n = c30;
            return (([&]() -> bool {
                bool guid35 = true;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                bool satisfied = guid35;
                
                (([&]() -> juniper::unit {
                    uint32_t guid36 = ((uint32_t) 0);
                    uint32_t guid37 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid36; i <= guid37; i++) {
                        (satisfied ? 
                            (([&]() -> juniper::unit {
                                (satisfied = pred(((lst).data)[i]));
                                return juniper::unit();
                            })())
                        :
                            juniper::unit());
                    }
                    return {};
                })());
                return satisfied;
            })());
        })());
    }
}

namespace List {
    template<typename t371, typename t368, int c32>
    bool any(juniper::function<t368, bool(t371)> pred, juniper::records::recordt_0<juniper::array<t371, c32>, uint32_t> lst) {
        return (([&]() -> bool {
            constexpr int32_t n = c32;
            return (([&]() -> bool {
                bool guid38 = false;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                bool satisfied = guid38;
                
                (([&]() -> juniper::unit {
                    uint32_t guid39 = ((uint32_t) 0);
                    uint32_t guid40 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid39; i <= guid40; i++) {
                        (!(satisfied) ? 
                            (([&]() -> juniper::unit {
                                (satisfied = pred(((lst).data)[i]));
                                return juniper::unit();
                            })())
                        :
                            juniper::unit());
                    }
                    return {};
                })());
                return satisfied;
            })());
        })());
    }
}

namespace List {
    template<typename t376, int c34>
    juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> pushBack(t376 elem, juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> lst) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> {
            constexpr int32_t n = c34;
            return (((lst).length >= n) ? 
                juniper::quit<juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t>>()
            :
                (([&]() -> juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> {
                    juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> guid41 = lst;
                    if (!(true)) {
                        juniper::quit<juniper::unit>();
                    }
                    juniper::records::recordt_0<juniper::array<t376, c34>, uint32_t> ret = guid41;
                    
                    (((ret).data)[(lst).length] = elem);
                    ((ret).length = ((lst).length + ((uint32_t) 1)));
                    return ret;
                })()));
        })());
    }
}

namespace List {
    template<typename t386, int c36>
    juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> pushOffFront(t386 elem, juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> lst) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> {
            constexpr int32_t n = c36;
            return (([&]() -> juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> {
                juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> guid42 = lst;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> ret = guid42;
                
                (([&]() -> juniper::unit {
                    int32_t guid43 = (n - ((int32_t) 2));
                    int32_t guid44 = ((int32_t) 0);
                    for (int32_t i = guid43; i >= guid44; i--) {
                        (([&]() -> juniper::unit {
                            (((ret).data)[(i + ((int32_t) 1))] = ((ret).data)[i]);
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                (((ret).data)[((uint32_t) 0)] = elem);
                return (((ret).length == i32ToU32(n)) ? 
                    ret
                :
                    (([&]() -> juniper::records::recordt_0<juniper::array<t386, c36>, uint32_t> {
                        ((ret).length = ((lst).length + ((uint32_t) 1)));
                        return ret;
                    })()));
            })());
        })());
    }
}

namespace List {
    template<typename t398, int c40>
    juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> setNth(uint32_t index, t398 elem, juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> lst) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> {
            constexpr int32_t n = c40;
            return (((lst).length <= index) ? 
                juniper::quit<juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t>>()
            :
                (([&]() -> juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> {
                    juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> guid45 = lst;
                    if (!(true)) {
                        juniper::quit<juniper::unit>();
                    }
                    juniper::records::recordt_0<juniper::array<t398, c40>, uint32_t> ret = guid45;
                    
                    (((ret).data)[index] = elem);
                    return ret;
                })()));
        })());
    }
}

namespace List {
    template<typename t403, int c42>
    juniper::records::recordt_0<juniper::array<t403, c42>, uint32_t> replicate(uint32_t numOfElements, t403 elem) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t403, c42>, uint32_t> {
            constexpr int32_t n = c42;
            return (([&]() -> juniper::records::recordt_0<juniper::array<t403, c42>, uint32_t>{
                juniper::records::recordt_0<juniper::array<t403, c42>, uint32_t> guid46;
                guid46.data = (juniper::array<t403, c42>().fill(elem));
                guid46.length = numOfElements;
                return guid46;
            })());
        })());
    }
}

namespace List {
    template<typename t413, int c43>
    juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> remove(t413 elem, juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> lst) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> {
            constexpr int32_t n = c43;
            return (([&]() -> juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> {
                uint32_t guid47 = ((uint32_t) 0);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                uint32_t index = guid47;
                
                bool guid48 = false;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                bool found = guid48;
                
                (([&]() -> juniper::unit {
                    uint32_t guid49 = ((uint32_t) 0);
                    uint32_t guid50 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid49; i <= guid50; i++) {
                        ((!(found) && (((lst).data)[i] == elem)) ? 
                            (([&]() -> juniper::unit {
                                (index = i);
                                (found = true);
                                return juniper::unit();
                            })())
                        :
                            juniper::unit());
                    }
                    return {};
                })());
                return (found ? 
                    (([&]() -> juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> {
                        juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> guid51 = lst;
                        if (!(true)) {
                            juniper::quit<juniper::unit>();
                        }
                        juniper::records::recordt_0<juniper::array<t413, c43>, uint32_t> ret = guid51;
                        
                        ((ret).length = ((lst).length - ((uint32_t) 1)));
                        (([&]() -> juniper::unit {
                            uint32_t guid52 = index;
                            uint32_t guid53 = ((lst).length - ((uint32_t) 2));
                            for (uint32_t i = guid52; i <= guid53; i++) {
                                (([&]() -> juniper::unit {
                                    (((ret).data)[i] = ((lst).data)[(i + ((uint32_t) 1))]);
                                    return juniper::unit();
                                })());
                            }
                            return {};
                        })());
                        return ret;
                    })())
                :
                    lst);
            })());
        })());
    }
}

namespace List {
    template<typename t417, int c47>
    juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t> dropLast(juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t> lst) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t> {
            constexpr int32_t n = c47;
            return (((lst).length == ((uint32_t) 0)) ? 
                juniper::quit<juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t>>()
            :
                (([&]() -> juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<t417, c47>, uint32_t> guid54;
                    guid54.data = (lst).data;
                    guid54.length = ((lst).length - ((uint32_t) 1));
                    return guid54;
                })()));
        })());
    }
}

namespace List {
    template<typename t426, typename t423, int c48>
    juniper::unit foreach(juniper::function<t423, juniper::unit(t426)> f, juniper::records::recordt_0<juniper::array<t426, c48>, uint32_t> lst) {
        return (([&]() -> juniper::unit {
            constexpr int32_t n = c48;
            return (([&]() -> juniper::unit {
                uint32_t guid55 = ((uint32_t) 0);
                uint32_t guid56 = ((lst).length - ((uint32_t) 1));
                for (uint32_t i = guid55; i <= guid56; i++) {
                    f(((lst).data)[i]);
                }
                return {};
            })());
        })());
    }
}

namespace List {
    template<typename t436, int c50>
    t436 last(juniper::records::recordt_0<juniper::array<t436, c50>, uint32_t> lst) {
        return (([&]() -> t436 {
            constexpr int32_t n = c50;
            return (((lst).length == ((uint32_t) 0)) ? 
                juniper::quit<t436>()
            :
                ((lst).data)[((lst).length - ((uint32_t) 1))]);
        })());
    }
}

namespace List {
    template<typename t446, int c52>
    t446 max_(juniper::records::recordt_0<juniper::array<t446, c52>, uint32_t> lst) {
        return (([&]() -> t446 {
            constexpr int32_t n = c52;
            return ((((lst).length == ((uint32_t) 0)) || (n == ((int32_t) 0))) ? 
                juniper::quit<t446>()
            :
                (([&]() -> t446 {
                    t446 guid57 = ((lst).data)[((uint32_t) 0)];
                    if (!(true)) {
                        juniper::quit<juniper::unit>();
                    }
                    t446 maxVal = guid57;
                    
                    (([&]() -> juniper::unit {
                        uint32_t guid58 = ((uint32_t) 1);
                        uint32_t guid59 = ((lst).length - ((uint32_t) 1));
                        for (uint32_t i = guid58; i <= guid59; i++) {
                            ((((lst).data)[i] > maxVal) ? 
                                (([&]() -> juniper::unit {
                                    (maxVal = ((lst).data)[i]);
                                    return juniper::unit();
                                })())
                            :
                                juniper::unit());
                        }
                        return {};
                    })());
                    return maxVal;
                })()));
        })());
    }
}

namespace List {
    template<typename t456, int c56>
    t456 min_(juniper::records::recordt_0<juniper::array<t456, c56>, uint32_t> lst) {
        return (([&]() -> t456 {
            constexpr int32_t n = c56;
            return ((((lst).length == ((uint32_t) 0)) || (n == ((int32_t) 0))) ? 
                juniper::quit<t456>()
            :
                (([&]() -> t456 {
                    t456 guid60 = ((lst).data)[((uint32_t) 0)];
                    if (!(true)) {
                        juniper::quit<juniper::unit>();
                    }
                    t456 minVal = guid60;
                    
                    (([&]() -> juniper::unit {
                        uint32_t guid61 = ((uint32_t) 1);
                        uint32_t guid62 = ((lst).length - ((uint32_t) 1));
                        for (uint32_t i = guid61; i <= guid62; i++) {
                            ((((lst).data)[i] < minVal) ? 
                                (([&]() -> juniper::unit {
                                    (minVal = ((lst).data)[i]);
                                    return juniper::unit();
                                })())
                            :
                                juniper::unit());
                        }
                        return {};
                    })());
                    return minVal;
                })()));
        })());
    }
}

namespace List {
    template<typename t458, int c60>
    bool member(t458 elem, juniper::records::recordt_0<juniper::array<t458, c60>, uint32_t> lst) {
        return (([&]() -> bool {
            constexpr int32_t n = c60;
            return (([&]() -> bool {
                bool guid63 = false;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                bool found = guid63;
                
                (([&]() -> juniper::unit {
                    uint32_t guid64 = ((uint32_t) 0);
                    uint32_t guid65 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid64; i <= guid65; i++) {
                        ((!(found) && (((lst).data)[i] == elem)) ? 
                            (([&]() -> juniper::unit {
                                (found = true);
                                return juniper::unit();
                            })())
                        :
                            juniper::unit());
                    }
                    return {};
                })());
                return found;
            })());
        })());
    }
}

namespace List {
    template<typename t470, typename t472, int c62>
    juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t> zip(juniper::records::recordt_0<juniper::array<t470, c62>, uint32_t> lstA, juniper::records::recordt_0<juniper::array<t472, c62>, uint32_t> lstB) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t> {
            constexpr int32_t n = c62;
            return (((lstA).length == (lstB).length) ? 
                (([&]() -> juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t> {
                    juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t> guid66 = (([&]() -> juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t>{
                        juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t> guid67;
                        guid67.data = (juniper::array<juniper::tuple2<t470,t472>, c62>());
                        guid67.length = (lstA).length;
                        return guid67;
                    })());
                    if (!(true)) {
                        juniper::quit<juniper::unit>();
                    }
                    juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t> ret = guid66;
                    
                    (([&]() -> juniper::unit {
                        uint32_t guid68 = ((uint32_t) 0);
                        uint32_t guid69 = (lstA).length;
                        for (uint32_t i = guid68; i <= guid69; i++) {
                            (([&]() -> juniper::unit {
                                (((ret).data)[i] = (juniper::tuple2<t470,t472>{((lstA).data)[i], ((lstB).data)[i]}));
                                return juniper::unit();
                            })());
                        }
                        return {};
                    })());
                    return ret;
                })())
            :
                juniper::quit<juniper::records::recordt_0<juniper::array<juniper::tuple2<t470,t472>, c62>, uint32_t>>());
        })());
    }
}

namespace List {
    template<typename t483, typename t484, int c66>
    juniper::tuple2<juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t>,juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t>> unzip(juniper::records::recordt_0<juniper::array<juniper::tuple2<t483,t484>, c66>, uint32_t> lst) {
        return (([&]() -> juniper::tuple2<juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t>,juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t>> {
            constexpr int32_t n = c66;
            return (([&]() -> juniper::tuple2<juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t>,juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t>> {
                juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t> guid70 = (([&]() -> juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t> guid71;
                    guid71.data = (juniper::array<t483, c66>());
                    guid71.length = (lst).length;
                    return guid71;
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t> retA = guid70;
                
                juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t> guid72 = (([&]() -> juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t> guid73;
                    guid73.data = (juniper::array<t484, c66>());
                    guid73.length = (lst).length;
                    return guid73;
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t> retB = guid72;
                
                (([&]() -> juniper::unit {
                    uint32_t guid74 = ((uint32_t) 0);
                    uint32_t guid75 = ((lst).length - ((uint32_t) 1));
                    for (uint32_t i = guid74; i <= guid75; i++) {
                        (([&]() -> juniper::unit {
                            juniper::tuple2<t483,t484> guid76 = ((lst).data)[i];
                            if (!(true)) {
                                juniper::quit<juniper::unit>();
                            }
                            t484 b = (guid76).e2;
                            t483 a = (guid76).e1;
                            
                            (((retA).data)[i] = a);
                            (((retB).data)[i] = b);
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return (juniper::tuple2<juniper::records::recordt_0<juniper::array<t483, c66>, uint32_t>,juniper::records::recordt_0<juniper::array<t484, c66>, uint32_t>>{retA, retB});
            })());
        })());
    }
}

namespace List {
    template<typename t493, int c70>
    t493 sum(juniper::records::recordt_0<juniper::array<t493, c70>, uint32_t> lst) {
        return (([&]() -> t493 {
            constexpr int32_t n = c70;
            return List::foldl<t493, t493, void, c70>(juniper::function<void, t493(t493, t493)>(add<t493>), ((t493) 0), lst);
        })());
    }
}

namespace List {
    template<typename t505, int c71>
    t505 average(juniper::records::recordt_0<juniper::array<t505, c71>, uint32_t> lst) {
        return (([&]() -> t505 {
            constexpr int32_t n = c71;
            return (sum<t505, c71>(lst) / cast<uint32_t, t505>((lst).length));
        })());
    }
}

namespace Signal {
    template<typename t511, typename t512, typename t513>
    Prelude::sig<t512> map(juniper::function<t513, t512(t511)> f, Prelude::sig<t511> s) {
        return (([&]() -> Prelude::sig<t512> {
            using a = t511;
            using b = t512;
            using closure = t513;
            return (([&]() -> Prelude::sig<t512> {
                Prelude::sig<t511> guid77 = s;
                return ((((guid77).id() == ((uint8_t) 0)) && ((((guid77).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> Prelude::sig<t512> {
                        t511 val = ((guid77).signal()).just();
                        return signal<t512>(just<t512>(f(val)));
                    })())
                :
                    (true ? 
                        (([&]() -> Prelude::sig<t512> {
                            return signal<t512>(nothing<t512>());
                        })())
                    :
                        juniper::quit<Prelude::sig<t512>>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t529, typename t530>
    juniper::unit sink(juniper::function<t530, juniper::unit(t529)> f, Prelude::sig<t529> s) {
        return (([&]() -> juniper::unit {
            using a = t529;
            using closure = t530;
            return (([&]() -> juniper::unit {
                Prelude::sig<t529> guid78 = s;
                return ((((guid78).id() == ((uint8_t) 0)) && ((((guid78).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> juniper::unit {
                        t529 val = ((guid78).signal()).just();
                        return f(val);
                    })())
                :
                    (true ? 
                        (([&]() -> juniper::unit {
                            return juniper::unit();
                        })())
                    :
                        juniper::quit<juniper::unit>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t535, typename t536>
    Prelude::sig<t535> filter(juniper::function<t536, bool(t535)> f, Prelude::sig<t535> s) {
        return (([&]() -> Prelude::sig<t535> {
            using a = t535;
            using closure = t536;
            return (([&]() -> Prelude::sig<t535> {
                Prelude::sig<t535> guid79 = s;
                return ((((guid79).id() == ((uint8_t) 0)) && ((((guid79).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> Prelude::sig<t535> {
                        t535 val = ((guid79).signal()).just();
                        return (f(val) ? 
                            signal<t535>(nothing<t535>())
                        :
                            s);
                    })())
                :
                    (true ? 
                        (([&]() -> Prelude::sig<t535> {
                            return signal<t535>(nothing<t535>());
                        })())
                    :
                        juniper::quit<Prelude::sig<t535>>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t551>
    Prelude::sig<t551> merge(Prelude::sig<t551> sigA, Prelude::sig<t551> sigB) {
        return (([&]() -> Prelude::sig<t551> {
            using a = t551;
            return (([&]() -> Prelude::sig<t551> {
                Prelude::sig<t551> guid80 = sigA;
                return ((((guid80).id() == ((uint8_t) 0)) && ((((guid80).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> Prelude::sig<t551> {
                        return sigA;
                    })())
                :
                    (true ? 
                        (([&]() -> Prelude::sig<t551> {
                            return sigB;
                        })())
                    :
                        juniper::quit<Prelude::sig<t551>>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t553, int c72>
    Prelude::sig<t553> mergeMany(juniper::records::recordt_0<juniper::array<Prelude::sig<t553>, c72>, uint32_t> sigs) {
        return (([&]() -> Prelude::sig<t553> {
            constexpr int32_t n = c72;
            return (([&]() -> Prelude::sig<t553> {
                Prelude::maybe<t553> guid81 = List::foldl<Prelude::sig<t553>, Prelude::maybe<t553>, void, c72>(juniper::function<void, Prelude::maybe<t553>(Prelude::sig<t553>,Prelude::maybe<t553>)>([](Prelude::sig<t553> sig, Prelude::maybe<t553> accum) -> Prelude::maybe<t553> { 
                    return (([&]() -> Prelude::maybe<t553> {
                        Prelude::maybe<t553> guid82 = accum;
                        return ((((guid82).id() == ((uint8_t) 1)) && true) ? 
                            (([&]() -> Prelude::maybe<t553> {
                                return (([&]() -> Prelude::maybe<t553> {
                                    Prelude::sig<t553> guid83 = sig;
                                    if (!((((guid83).id() == ((uint8_t) 0)) && true))) {
                                        juniper::quit<juniper::unit>();
                                    }
                                    Prelude::maybe<t553> heldValue = (guid83).signal();
                                    
                                    return heldValue;
                                })());
                            })())
                        :
                            (true ? 
                                (([&]() -> Prelude::maybe<t553> {
                                    return accum;
                                })())
                            :
                                juniper::quit<Prelude::maybe<t553>>()));
                    })());
                 }), nothing<t553>(), sigs);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                Prelude::maybe<t553> ret = guid81;
                
                return signal<t553>(ret);
            })());
        })());
    }
}

namespace Signal {
    template<typename t570, typename t571>
    Prelude::sig<Prelude::either<t570, t571>> join(Prelude::sig<t570> sigA, Prelude::sig<t571> sigB) {
        return (([&]() -> Prelude::sig<Prelude::either<t570, t571>> {
            using a = t570;
            using b = t571;
            return (([&]() -> Prelude::sig<Prelude::either<t570, t571>> {
                juniper::tuple2<Prelude::sig<t570>,Prelude::sig<t571>> guid84 = (juniper::tuple2<Prelude::sig<t570>,Prelude::sig<t571>>{sigA, sigB});
                return (((((guid84).e1).id() == ((uint8_t) 0)) && (((((guid84).e1).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> Prelude::sig<Prelude::either<t570, t571>> {
                        t570 value = (((guid84).e1).signal()).just();
                        return signal<Prelude::either<t570, t571>>(just<Prelude::either<t570, t571>>(left<t570, t571>(value)));
                    })())
                :
                    (((((guid84).e2).id() == ((uint8_t) 0)) && (((((guid84).e2).signal()).id() == ((uint8_t) 0)) && true)) ? 
                        (([&]() -> Prelude::sig<Prelude::either<t570, t571>> {
                            t571 value = (((guid84).e2).signal()).just();
                            return signal<Prelude::either<t570, t571>>(just<Prelude::either<t570, t571>>(right<t570, t571>(value)));
                        })())
                    :
                        (true ? 
                            (([&]() -> Prelude::sig<Prelude::either<t570, t571>> {
                                return signal<Prelude::either<t570, t571>>(nothing<Prelude::either<t570, t571>>());
                            })())
                        :
                            juniper::quit<Prelude::sig<Prelude::either<t570, t571>>>())));
            })());
        })());
    }
}

namespace Signal {
    template<typename t601>
    Prelude::sig<juniper::unit> toUnit(Prelude::sig<t601> s) {
        return (([&]() -> Prelude::sig<juniper::unit> {
            using a = t601;
            return map<t601, juniper::unit, void>(juniper::function<void, juniper::unit(t601)>([](t601 x) -> juniper::unit { 
                return juniper::unit();
             }), s);
        })());
    }
}

namespace Signal {
    template<typename t607, typename t615, typename t609>
    Prelude::sig<t615> foldP(juniper::function<t609, t615(t607, t615)> f, juniper::shared_ptr<t615> state0, Prelude::sig<t607> incoming) {
        return (([&]() -> Prelude::sig<t615> {
            using a = t607;
            using state = t615;
            using closure = t609;
            return (([&]() -> Prelude::sig<t615> {
                Prelude::sig<t607> guid85 = incoming;
                return ((((guid85).id() == ((uint8_t) 0)) && ((((guid85).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> Prelude::sig<t615> {
                        t607 val = ((guid85).signal()).just();
                        return (([&]() -> Prelude::sig<t615> {
                            t615 guid86 = f(val, (*((state0).get())));
                            if (!(true)) {
                                juniper::quit<juniper::unit>();
                            }
                            t615 state1 = guid86;
                            
                            (*((t615*) (state0.get())) = state1);
                            return signal<t615>(just<t615>(state1));
                        })());
                    })())
                :
                    (true ? 
                        (([&]() -> Prelude::sig<t615> {
                            return signal<t615>(nothing<t615>());
                        })())
                    :
                        juniper::quit<Prelude::sig<t615>>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t631>
    Prelude::sig<t631> dropRepeats(juniper::shared_ptr<Prelude::maybe<t631>> maybePrevValue, Prelude::sig<t631> incoming) {
        return (([&]() -> Prelude::sig<t631> {
            using a = t631;
            return filter<t631, juniper::closures::closuret_4<juniper::shared_ptr<Prelude::maybe<t631>>>>(juniper::function<juniper::closures::closuret_4<juniper::shared_ptr<Prelude::maybe<t631>>>, bool(t631)>(juniper::closures::closuret_4<juniper::shared_ptr<Prelude::maybe<t631>>>(maybePrevValue), [](juniper::closures::closuret_4<juniper::shared_ptr<Prelude::maybe<t631>>>& junclosure, t631 value) -> bool { 
                juniper::shared_ptr<Prelude::maybe<t631>>& maybePrevValue = junclosure.maybePrevValue;
                return (([&]() -> bool {
                    bool guid87 = (([&]() -> bool {
                        Prelude::maybe<t631> guid88 = (*((maybePrevValue).get()));
                        return ((((guid88).id() == ((uint8_t) 1)) && true) ? 
                            (([&]() -> bool {
                                return false;
                            })())
                        :
                            ((((guid88).id() == ((uint8_t) 0)) && true) ? 
                                (([&]() -> bool {
                                    t631 prevValue = (guid88).just();
                                    return (value == prevValue);
                                })())
                            :
                                juniper::quit<bool>()));
                    })());
                    if (!(true)) {
                        juniper::quit<juniper::unit>();
                    }
                    bool filtered = guid87;
                    
                    (!(filtered) ? 
                        (([&]() -> juniper::unit {
                            (*((Prelude::maybe<t631>*) (maybePrevValue.get())) = just<t631>(value));
                            return juniper::unit();
                        })())
                    :
                        juniper::unit());
                    return filtered;
                })());
             }), incoming);
        })());
    }
}

namespace Signal {
    template<typename t643>
    Prelude::sig<t643> latch(juniper::shared_ptr<t643> prevValue, Prelude::sig<t643> incoming) {
        return (([&]() -> Prelude::sig<t643> {
            using a = t643;
            return (([&]() -> Prelude::sig<t643> {
                Prelude::sig<t643> guid89 = incoming;
                return ((((guid89).id() == ((uint8_t) 0)) && ((((guid89).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> Prelude::sig<t643> {
                        t643 val = ((guid89).signal()).just();
                        return (([&]() -> Prelude::sig<t643> {
                            (*((t643*) (prevValue.get())) = val);
                            return incoming;
                        })());
                    })())
                :
                    (true ? 
                        (([&]() -> Prelude::sig<t643> {
                            return signal<t643>(just<t643>((*((prevValue).get()))));
                        })())
                    :
                        juniper::quit<Prelude::sig<t643>>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t658, typename t662, typename t654, typename t655>
    Prelude::sig<t654> map2(juniper::function<t655, t654(t658, t662)> f, juniper::shared_ptr<juniper::tuple2<t658,t662>> state, Prelude::sig<t658> incomingA, Prelude::sig<t662> incomingB) {
        return (([&]() -> Prelude::sig<t654> {
            using a = t658;
            using b = t662;
            using c = t654;
            using closure = t655;
            return (([&]() -> Prelude::sig<t654> {
                t658 guid90 = (([&]() -> t658 {
                    Prelude::sig<t658> guid91 = incomingA;
                    return ((((guid91).id() == ((uint8_t) 0)) && ((((guid91).signal()).id() == ((uint8_t) 0)) && true)) ? 
                        (([&]() -> t658 {
                            t658 val1 = ((guid91).signal()).just();
                            return val1;
                        })())
                    :
                        (true ? 
                            (([&]() -> t658 {
                                return fst<t658, t662>((*((state).get())));
                            })())
                        :
                            juniper::quit<t658>()));
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t658 valA = guid90;
                
                t662 guid92 = (([&]() -> t662 {
                    Prelude::sig<t662> guid93 = incomingB;
                    return ((((guid93).id() == ((uint8_t) 0)) && ((((guid93).signal()).id() == ((uint8_t) 0)) && true)) ? 
                        (([&]() -> t662 {
                            t662 val2 = ((guid93).signal()).just();
                            return val2;
                        })())
                    :
                        (true ? 
                            (([&]() -> t662 {
                                return snd<t658, t662>((*((state).get())));
                            })())
                        :
                            juniper::quit<t662>()));
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t662 valB = guid92;
                
                (*((juniper::tuple2<t658,t662>*) (state.get())) = (juniper::tuple2<t658,t662>{valA, valB}));
                return (([&]() -> Prelude::sig<t654> {
                    juniper::tuple2<Prelude::sig<t658>,Prelude::sig<t662>> guid94 = (juniper::tuple2<Prelude::sig<t658>,Prelude::sig<t662>>{incomingA, incomingB});
                    return (((((guid94).e2).id() == ((uint8_t) 0)) && (((((guid94).e2).signal()).id() == ((uint8_t) 1)) && ((((guid94).e1).id() == ((uint8_t) 0)) && (((((guid94).e1).signal()).id() == ((uint8_t) 1)) && true)))) ? 
                        (([&]() -> Prelude::sig<t654> {
                            return signal<t654>(nothing<t654>());
                        })())
                    :
                        (true ? 
                            (([&]() -> Prelude::sig<t654> {
                                return signal<t654>(just<t654>(f(valA, valB)));
                            })())
                        :
                            juniper::quit<Prelude::sig<t654>>()));
                })());
            })());
        })());
    }
}

namespace Signal {
    template<typename t683, int c74>
    Prelude::sig<juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>> record(juniper::shared_ptr<juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>> pastValues, Prelude::sig<t683> incoming) {
        return (([&]() -> Prelude::sig<juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>> {
            constexpr int32_t n = c74;
            return foldP<t683, juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>, void>(juniper::function<void, juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>(t683, juniper::records::recordt_0<juniper::array<t683, c74>, uint32_t>)>(List::pushOffFront<t683, c74>), pastValues, incoming);
        })());
    }
}

namespace Signal {
    template<typename t693>
    Prelude::sig<t693> constant(t693 val) {
        return (([&]() -> Prelude::sig<t693> {
            using a = t693;
            return signal<t693>(just<t693>(val));
        })());
    }
}

namespace Signal {
    template<typename t701>
    Prelude::sig<Prelude::maybe<t701>> meta(Prelude::sig<t701> sigA) {
        return (([&]() -> Prelude::sig<Prelude::maybe<t701>> {
            using a = t701;
            return (([&]() -> Prelude::sig<Prelude::maybe<t701>> {
                Prelude::sig<t701> guid95 = sigA;
                if (!((((guid95).id() == ((uint8_t) 0)) && true))) {
                    juniper::quit<juniper::unit>();
                }
                Prelude::maybe<t701> val = (guid95).signal();
                
                return constant<Prelude::maybe<t701>>(val);
            })());
        })());
    }
}

namespace Signal {
    template<typename t706>
    Prelude::sig<t706> unmeta(Prelude::sig<Prelude::maybe<t706>> sigA) {
        return (([&]() -> Prelude::sig<t706> {
            using a = t706;
            return (([&]() -> Prelude::sig<t706> {
                Prelude::sig<Prelude::maybe<t706>> guid96 = sigA;
                return ((((guid96).id() == ((uint8_t) 0)) && ((((guid96).signal()).id() == ((uint8_t) 0)) && (((((guid96).signal()).just()).id() == ((uint8_t) 0)) && true))) ? 
                    (([&]() -> Prelude::sig<t706> {
                        t706 val = (((guid96).signal()).just()).just();
                        return constant<t706>(val);
                    })())
                :
                    (true ? 
                        (([&]() -> Prelude::sig<t706> {
                            return signal<t706>(nothing<t706>());
                        })())
                    :
                        juniper::quit<Prelude::sig<t706>>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t718, typename t719>
    Prelude::sig<juniper::tuple2<t718,t719>> zip(juniper::shared_ptr<juniper::tuple2<t718,t719>> state, Prelude::sig<t718> sigA, Prelude::sig<t719> sigB) {
        return (([&]() -> Prelude::sig<juniper::tuple2<t718,t719>> {
            using a = t718;
            using b = t719;
            return map2<t718, t719, juniper::tuple2<t718,t719>, void>(juniper::function<void, juniper::tuple2<t718,t719>(t718,t719)>([](t718 valA, t719 valB) -> juniper::tuple2<t718,t719> { 
                return (juniper::tuple2<t718,t719>{valA, valB});
             }), state, sigA, sigB);
        })());
    }
}

namespace Signal {
    template<typename t750, typename t757>
    juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>> unzip(Prelude::sig<juniper::tuple2<t750,t757>> incoming) {
        return (([&]() -> juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>> {
            using a = t750;
            using b = t757;
            return (([&]() -> juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>> {
                Prelude::sig<juniper::tuple2<t750,t757>> guid97 = incoming;
                return ((((guid97).id() == ((uint8_t) 0)) && ((((guid97).signal()).id() == ((uint8_t) 0)) && true)) ? 
                    (([&]() -> juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>> {
                        t757 y = (((guid97).signal()).just()).e2;
                        t750 x = (((guid97).signal()).just()).e1;
                        return (juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>>{signal<t750>(just<t750>(x)), signal<t757>(just<t757>(y))});
                    })())
                :
                    (true ? 
                        (([&]() -> juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>> {
                            return (juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>>{signal<t750>(nothing<t750>()), signal<t757>(nothing<t757>())});
                        })())
                    :
                        juniper::quit<juniper::tuple2<Prelude::sig<t750>,Prelude::sig<t757>>>()));
            })());
        })());
    }
}

namespace Signal {
    template<typename t764, typename t765>
    Prelude::sig<t764> toggle(t764 val1, t764 val2, juniper::shared_ptr<t764> state, Prelude::sig<t765> incoming) {
        return (([&]() -> Prelude::sig<t764> {
            using a = t764;
            using b = t765;
            return foldP<t765, t764, juniper::closures::closuret_5<t764, t764>>(juniper::function<juniper::closures::closuret_5<t764, t764>, t764(t765,t764)>(juniper::closures::closuret_5<t764, t764>(val1, val2), [](juniper::closures::closuret_5<t764, t764>& junclosure, t765 event, t764 prevVal) -> t764 { 
                t764& val1 = junclosure.val1;
                t764& val2 = junclosure.val2;
                return ((prevVal == val1) ? 
                    val2
                :
                    val1);
             }), state, incoming);
        })());
    }
}

namespace Io {
    Io::pinState toggle(Io::pinState p) {
        return (([&]() -> Io::pinState {
            Io::pinState guid98 = p;
            return ((((guid98).id() == ((uint8_t) 0)) && true) ? 
                (([&]() -> Io::pinState {
                    return low();
                })())
            :
                ((((guid98).id() == ((uint8_t) 1)) && true) ? 
                    (([&]() -> Io::pinState {
                        return high();
                    })())
                :
                    juniper::quit<Io::pinState>()));
        })());
    }
}

namespace Io {
    juniper::unit printStr(const char * str) {
        return (([&]() -> juniper::unit {
            Serial.print(str);
            return {};
        })());
    }
}

namespace Io {
    template<int c75>
    juniper::unit printCharList(juniper::records::recordt_0<juniper::array<uint8_t, c75>, uint32_t> cl) {
        return (([&]() -> juniper::unit {
            constexpr int32_t n = c75;
            return (([&]() -> juniper::unit {
                Serial.print((char *) &cl.data[0]);
                return {};
            })());
        })());
    }
}

namespace Io {
    juniper::unit printFloat(float f) {
        return (([&]() -> juniper::unit {
            Serial.print(f);
            return {};
        })());
    }
}

namespace Io {
    juniper::unit printInt(int32_t n) {
        return (([&]() -> juniper::unit {
            Serial.print(n);
            return {};
        })());
    }
}

namespace Io {
    template<typename t790>
    t790 baseToInt(Io::base b) {
        return (([&]() -> t790 {
            Io::base guid99 = b;
            return ((((guid99).id() == ((uint8_t) 0)) && true) ? 
                (([&]() -> t790 {
                    return ((t790) 2);
                })())
            :
                ((((guid99).id() == ((uint8_t) 1)) && true) ? 
                    (([&]() -> t790 {
                        return ((t790) 8);
                    })())
                :
                    ((((guid99).id() == ((uint8_t) 2)) && true) ? 
                        (([&]() -> t790 {
                            return ((t790) 10);
                        })())
                    :
                        ((((guid99).id() == ((uint8_t) 3)) && true) ? 
                            (([&]() -> t790 {
                                return ((t790) 16);
                            })())
                        :
                            juniper::quit<t790>()))));
        })());
    }
}

namespace Io {
    juniper::unit printIntBase(int32_t n, Io::base b) {
        return (([&]() -> juniper::unit {
            int32_t guid100 = baseToInt<int32_t>(b);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            int32_t bint = guid100;
            
            return (([&]() -> juniper::unit {
                Serial.print(n, bint);
                return {};
            })());
        })());
    }
}

namespace Io {
    juniper::unit printFloatPlaces(float f, int32_t numPlaces) {
        return (([&]() -> juniper::unit {
            Serial.print(f, numPlaces);
            return {};
        })());
    }
}

namespace Io {
    juniper::unit beginSerial(uint32_t speed) {
        return (([&]() -> juniper::unit {
            Serial.begin(speed);
            return {};
        })());
    }
}

namespace Io {
    uint8_t pinStateToInt(Io::pinState value) {
        return (([&]() -> uint8_t {
            Io::pinState guid101 = value;
            return ((((guid101).id() == ((uint8_t) 1)) && true) ? 
                (([&]() -> uint8_t {
                    return ((uint8_t) 0);
                })())
            :
                ((((guid101).id() == ((uint8_t) 0)) && true) ? 
                    (([&]() -> uint8_t {
                        return ((uint8_t) 1);
                    })())
                :
                    juniper::quit<uint8_t>()));
        })());
    }
}

namespace Io {
    Io::pinState intToPinState(uint8_t value) {
        return ((value == ((uint8_t) 0)) ? 
            low()
        :
            high());
    }
}

namespace Io {
    juniper::unit digWrite(uint16_t pin, Io::pinState value) {
        return (([&]() -> juniper::unit {
            uint8_t guid102 = pinStateToInt(value);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            uint8_t intVal = guid102;
            
            return (([&]() -> juniper::unit {
                digitalWrite(pin, intVal);
                return {};
            })());
        })());
    }
}

namespace Io {
    Io::pinState digRead(uint16_t pin) {
        return (([&]() -> Io::pinState {
            uint8_t intVal;
            
            (([&]() -> juniper::unit {
                intVal = digitalRead(pin);
                return {};
            })());
            return intToPinState(intVal);
        })());
    }
}

namespace Io {
    Prelude::sig<Io::pinState> digIn(uint16_t pin) {
        return signal<Io::pinState>(just<Io::pinState>(digRead(pin)));
    }
}

namespace Io {
    juniper::unit digOut(uint16_t pin, Prelude::sig<Io::pinState> sig) {
        return Signal::sink<Io::pinState, juniper::closures::closuret_6<uint16_t>>(juniper::function<juniper::closures::closuret_6<uint16_t>, juniper::unit(Io::pinState)>(juniper::closures::closuret_6<uint16_t>(pin), [](juniper::closures::closuret_6<uint16_t>& junclosure, Io::pinState value) -> juniper::unit { 
            uint16_t& pin = junclosure.pin;
            return digWrite(pin, value);
         }), sig);
    }
}

namespace Io {
    uint16_t anaRead(uint16_t pin) {
        return (([&]() -> uint16_t {
            uint16_t value;
            
            (([&]() -> juniper::unit {
                value = analogRead(pin);
                return {};
            })());
            return value;
        })());
    }
}

namespace Io {
    juniper::unit anaWrite(uint16_t pin, uint8_t value) {
        return (([&]() -> juniper::unit {
            analogWrite(pin, value);
            return {};
        })());
    }
}

namespace Io {
    Prelude::sig<uint16_t> anaIn(uint16_t pin) {
        return signal<uint16_t>(just<uint16_t>(anaRead(pin)));
    }
}

namespace Io {
    juniper::unit anaOut(uint16_t pin, Prelude::sig<uint8_t> sig) {
        return Signal::sink<uint8_t, juniper::closures::closuret_6<uint16_t>>(juniper::function<juniper::closures::closuret_6<uint16_t>, juniper::unit(uint8_t)>(juniper::closures::closuret_6<uint16_t>(pin), [](juniper::closures::closuret_6<uint16_t>& junclosure, uint8_t value) -> juniper::unit { 
            uint16_t& pin = junclosure.pin;
            return anaWrite(pin, value);
         }), sig);
    }
}

namespace Io {
    uint8_t pinModeToInt(Io::mode m) {
        return (([&]() -> uint8_t {
            Io::mode guid103 = m;
            return ((((guid103).id() == ((uint8_t) 0)) && true) ? 
                (([&]() -> uint8_t {
                    return ((uint8_t) 0);
                })())
            :
                ((((guid103).id() == ((uint8_t) 1)) && true) ? 
                    (([&]() -> uint8_t {
                        return ((uint8_t) 1);
                    })())
                :
                    ((((guid103).id() == ((uint8_t) 2)) && true) ? 
                        (([&]() -> uint8_t {
                            return ((uint8_t) 2);
                        })())
                    :
                        juniper::quit<uint8_t>())));
        })());
    }
}

namespace Io {
    Io::mode intToPinMode(uint8_t m) {
        return (([&]() -> Io::mode {
            uint8_t guid104 = m;
            return (((guid104 == ((int32_t) 0)) && true) ? 
                (([&]() -> Io::mode {
                    return input();
                })())
            :
                (((guid104 == ((int32_t) 1)) && true) ? 
                    (([&]() -> Io::mode {
                        return output();
                    })())
                :
                    (((guid104 == ((int32_t) 2)) && true) ? 
                        (([&]() -> Io::mode {
                            return inputPullup();
                        })())
                    :
                        juniper::quit<Io::mode>())));
        })());
    }
}

namespace Io {
    juniper::unit setPinMode(uint16_t pin, Io::mode m) {
        return (([&]() -> juniper::unit {
            uint8_t guid105 = pinModeToInt(m);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            uint8_t m2 = guid105;
            
            return (([&]() -> juniper::unit {
                pinMode(pin, m2);
                return {};
            })());
        })());
    }
}

namespace Io {
    Prelude::sig<juniper::unit> risingEdge(Prelude::sig<Io::pinState> sig, juniper::shared_ptr<Io::pinState> prevState) {
        return Signal::toUnit<Io::pinState>(Signal::filter<Io::pinState, juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>>(juniper::function<juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>, bool(Io::pinState)>(juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>(prevState), [](juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>& junclosure, Io::pinState currState) -> bool { 
            juniper::shared_ptr<Io::pinState>& prevState = junclosure.prevState;
            return (([&]() -> bool {
                bool guid106 = (([&]() -> bool {
                    juniper::tuple2<Io::pinState,Io::pinState> guid107 = (juniper::tuple2<Io::pinState,Io::pinState>{currState, (*((prevState).get()))});
                    return (((((guid107).e2).id() == ((uint8_t) 1)) && ((((guid107).e1).id() == ((uint8_t) 0)) && true)) ? 
                        (([&]() -> bool {
                            return false;
                        })())
                    :
                        (true ? 
                            (([&]() -> bool {
                                return true;
                            })())
                        :
                            juniper::quit<bool>()));
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                bool ret = guid106;
                
                (*((Io::pinState*) (prevState.get())) = currState);
                return ret;
            })());
         }), sig));
    }
}

namespace Io {
    Prelude::sig<juniper::unit> fallingEdge(Prelude::sig<Io::pinState> sig, juniper::shared_ptr<Io::pinState> prevState) {
        return Signal::toUnit<Io::pinState>(Signal::filter<Io::pinState, juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>>(juniper::function<juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>, bool(Io::pinState)>(juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>(prevState), [](juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>& junclosure, Io::pinState currState) -> bool { 
            juniper::shared_ptr<Io::pinState>& prevState = junclosure.prevState;
            return (([&]() -> bool {
                bool guid108 = (([&]() -> bool {
                    juniper::tuple2<Io::pinState,Io::pinState> guid109 = (juniper::tuple2<Io::pinState,Io::pinState>{currState, (*((prevState).get()))});
                    return (((((guid109).e2).id() == ((uint8_t) 0)) && ((((guid109).e1).id() == ((uint8_t) 1)) && true)) ? 
                        (([&]() -> bool {
                            return false;
                        })())
                    :
                        (true ? 
                            (([&]() -> bool {
                                return true;
                            })())
                        :
                            juniper::quit<bool>()));
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                bool ret = guid108;
                
                (*((Io::pinState*) (prevState.get())) = currState);
                return ret;
            })());
         }), sig));
    }
}

namespace Io {
    Prelude::sig<Io::pinState> edge(Prelude::sig<Io::pinState> sig, juniper::shared_ptr<Io::pinState> prevState) {
        return Signal::filter<Io::pinState, juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>>(juniper::function<juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>, bool(Io::pinState)>(juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>(prevState), [](juniper::closures::closuret_7<juniper::shared_ptr<Io::pinState>>& junclosure, Io::pinState currState) -> bool { 
            juniper::shared_ptr<Io::pinState>& prevState = junclosure.prevState;
            return (([&]() -> bool {
                bool guid110 = (([&]() -> bool {
                    juniper::tuple2<Io::pinState,Io::pinState> guid111 = (juniper::tuple2<Io::pinState,Io::pinState>{currState, (*((prevState).get()))});
                    return (((((guid111).e2).id() == ((uint8_t) 1)) && ((((guid111).e1).id() == ((uint8_t) 0)) && true)) ? 
                        (([&]() -> bool {
                            return false;
                        })())
                    :
                        (((((guid111).e2).id() == ((uint8_t) 0)) && ((((guid111).e1).id() == ((uint8_t) 1)) && true)) ? 
                            (([&]() -> bool {
                                return false;
                            })())
                        :
                            (true ? 
                                (([&]() -> bool {
                                    return true;
                                })())
                            :
                                juniper::quit<bool>())));
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                bool ret = guid110;
                
                (*((Io::pinState*) (prevState.get())) = currState);
                return ret;
            })());
         }), sig);
    }
}

namespace Maybe {
    template<typename t905, typename t906, typename t907>
    Prelude::maybe<t906> map(juniper::function<t907, t906(t905)> f, Prelude::maybe<t905> maybeVal) {
        return (([&]() -> Prelude::maybe<t906> {
            using a = t905;
            using b = t906;
            using closure = t907;
            return (([&]() -> Prelude::maybe<t906> {
                Prelude::maybe<t905> guid112 = maybeVal;
                return ((((guid112).id() == ((uint8_t) 0)) && true) ? 
                    (([&]() -> Prelude::maybe<t906> {
                        t905 val = (guid112).just();
                        return just<t906>(f(val));
                    })())
                :
                    (true ? 
                        (([&]() -> Prelude::maybe<t906> {
                            return nothing<t906>();
                        })())
                    :
                        juniper::quit<Prelude::maybe<t906>>()));
            })());
        })());
    }
}

namespace Maybe {
    template<typename t917>
    t917 get(Prelude::maybe<t917> maybeVal) {
        return (([&]() -> t917 {
            using a = t917;
            return (([&]() -> t917 {
                Prelude::maybe<t917> guid113 = maybeVal;
                return ((((guid113).id() == ((uint8_t) 0)) && true) ? 
                    (([&]() -> t917 {
                        t917 val = (guid113).just();
                        return val;
                    })())
                :
                    juniper::quit<t917>());
            })());
        })());
    }
}

namespace Maybe {
    template<typename t919>
    bool isJust(Prelude::maybe<t919> maybeVal) {
        return (([&]() -> bool {
            using a = t919;
            return (([&]() -> bool {
                Prelude::maybe<t919> guid114 = maybeVal;
                return ((((guid114).id() == ((uint8_t) 0)) && true) ? 
                    (([&]() -> bool {
                        return true;
                    })())
                :
                    (true ? 
                        (([&]() -> bool {
                            return false;
                        })())
                    :
                        juniper::quit<bool>()));
            })());
        })());
    }
}

namespace Maybe {
    template<typename t921>
    bool isNothing(Prelude::maybe<t921> maybeVal) {
        return (([&]() -> bool {
            using a = t921;
            return !(isJust<t921>(maybeVal));
        })());
    }
}

namespace Maybe {
    template<typename t926>
    uint8_t count(Prelude::maybe<t926> maybeVal) {
        return (([&]() -> uint8_t {
            using a = t926;
            return (([&]() -> uint8_t {
                Prelude::maybe<t926> guid115 = maybeVal;
                return ((((guid115).id() == ((uint8_t) 0)) && true) ? 
                    (([&]() -> uint8_t {
                        return ((uint8_t) 1);
                    })())
                :
                    (true ? 
                        (([&]() -> uint8_t {
                            return ((uint8_t) 0);
                        })())
                    :
                        juniper::quit<uint8_t>()));
            })());
        })());
    }
}

namespace Maybe {
    template<typename t930, typename t931, typename t932>
    t931 foldl(juniper::function<t932, t931(t930, t931)> f, t931 initState, Prelude::maybe<t930> maybeVal) {
        return (([&]() -> t931 {
            using t = t930;
            using state = t931;
            using closure = t932;
            return (([&]() -> t931 {
                Prelude::maybe<t930> guid116 = maybeVal;
                return ((((guid116).id() == ((uint8_t) 0)) && true) ? 
                    (([&]() -> t931 {
                        t930 val = (guid116).just();
                        return f(val, initState);
                    })())
                :
                    (true ? 
                        (([&]() -> t931 {
                            return initState;
                        })())
                    :
                        juniper::quit<t931>()));
            })());
        })());
    }
}

namespace Maybe {
    template<typename t938, typename t939, typename t940>
    t939 fodlr(juniper::function<t940, t939(t938, t939)> f, t939 initState, Prelude::maybe<t938> maybeVal) {
        return (([&]() -> t939 {
            using t = t938;
            using state = t939;
            using closure = t940;
            return foldl<t938, t939, t940>(f, initState, maybeVal);
        })());
    }
}

namespace Maybe {
    template<typename t947, typename t948>
    juniper::unit iter(juniper::function<t948, juniper::unit(t947)> f, Prelude::maybe<t947> maybeVal) {
        return (([&]() -> juniper::unit {
            using a = t947;
            using closure = t948;
            return (([&]() -> juniper::unit {
                Prelude::maybe<t947> guid117 = maybeVal;
                return ((((guid117).id() == ((uint8_t) 0)) && true) ? 
                    (([&]() -> juniper::unit {
                        t947 val = (guid117).just();
                        return f(val);
                    })())
                :
                    (true ? 
                        (([&]() -> juniper::unit {
                            Prelude::maybe<t947> nothing = guid117;
                            return juniper::unit();
                        })())
                    :
                        juniper::quit<juniper::unit>()));
            })());
        })());
    }
}

namespace Time {
    juniper::unit wait(uint32_t time) {
        return (([&]() -> juniper::unit {
            delay(time);
            return {};
        })());
    }
}

namespace Time {
    uint32_t now() {
        return (([&]() -> uint32_t {
            uint32_t guid118 = ((uint32_t) 0);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            uint32_t ret = guid118;
            
            (([&]() -> juniper::unit {
                ret = millis();
                return {};
            })());
            return ret;
        })());
    }
}

namespace Time {
    juniper::shared_ptr<juniper::records::recordt_1<uint32_t>> state() {
        return (juniper::shared_ptr<juniper::records::recordt_1<uint32_t>>(new juniper::records::recordt_1<uint32_t>((([&]() -> juniper::records::recordt_1<uint32_t>{
            juniper::records::recordt_1<uint32_t> guid119;
            guid119.lastPulse = ((uint32_t) 0);
            return guid119;
        })()))));
    }
}

namespace Time {
    Prelude::sig<uint32_t> every(uint32_t interval, juniper::shared_ptr<juniper::records::recordt_1<uint32_t>> state) {
        return (([&]() -> Prelude::sig<uint32_t> {
            uint32_t guid120 = now();
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            uint32_t t = guid120;
            
            uint32_t guid121 = ((interval == ((uint32_t) 0)) ? 
                t
            :
                ((t / interval) * interval));
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            uint32_t lastWindow = guid121;
            
            return ((((*((state).get()))).lastPulse >= lastWindow) ? 
                signal<uint32_t>(nothing<uint32_t>())
            :
                (([&]() -> Prelude::sig<uint32_t> {
                    (*((juniper::records::recordt_1<uint32_t>*) (state.get())) = (([&]() -> juniper::records::recordt_1<uint32_t>{
                        juniper::records::recordt_1<uint32_t> guid122;
                        guid122.lastPulse = t;
                        return guid122;
                    })()));
                    return signal<uint32_t>(just<uint32_t>(t));
                })()));
        })());
    }
}

namespace Math {
    double pi = 3.141593;
}

namespace Math {
    double e = 2.718282;
}

namespace Math {
    double degToRad(double degrees) {
        return (degrees * 0.017453);
    }
}

namespace Math {
    double radToDeg(double radians) {
        return (radians * 57.295780);
    }
}

namespace Math {
    double acos_(double x) {
        return (([&]() -> double {
            double guid123 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid123;
            
            (([&]() -> juniper::unit {
                ret = acos(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double asin_(double x) {
        return (([&]() -> double {
            double guid124 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid124;
            
            (([&]() -> juniper::unit {
                ret = asin(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double atan_(double x) {
        return (([&]() -> double {
            double guid125 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid125;
            
            (([&]() -> juniper::unit {
                ret = atan(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double atan2_(double y, double x) {
        return (([&]() -> double {
            double guid126 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid126;
            
            (([&]() -> juniper::unit {
                ret = atan2(y, x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double cos_(double x) {
        return (([&]() -> double {
            double guid127 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid127;
            
            (([&]() -> juniper::unit {
                ret = cos(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double cosh_(double x) {
        return (([&]() -> double {
            double guid128 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid128;
            
            (([&]() -> juniper::unit {
                ret = cosh(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double sin_(double x) {
        return (([&]() -> double {
            double guid129 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid129;
            
            (([&]() -> juniper::unit {
                ret = sin(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double sinh_(double x) {
        return (([&]() -> double {
            double guid130 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid130;
            
            (([&]() -> juniper::unit {
                ret = sinh(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double tan_(double x) {
        return (sin_(x) / cos_(x));
    }
}

namespace Math {
    double tanh_(double x) {
        return (([&]() -> double {
            double guid131 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid131;
            
            (([&]() -> juniper::unit {
                ret = tanh(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double exp_(double x) {
        return (([&]() -> double {
            double guid132 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid132;
            
            (([&]() -> juniper::unit {
                ret = exp(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    juniper::tuple2<double,int16_t> frexp_(double x) {
        return (([&]() -> juniper::tuple2<double,int16_t> {
            double guid133 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid133;
            
            int16_t guid134 = ((int16_t) 0);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            int16_t exponent = guid134;
            
            (([&]() -> juniper::unit {
                int exponent2 = (int) exponent;
    ret = frexp(x, &exponent2);
                return {};
            })());
            return (juniper::tuple2<double,int16_t>{ret, exponent});
        })());
    }
}

namespace Math {
    double ldexp_(double x, int16_t exponent) {
        return (([&]() -> double {
            double guid135 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid135;
            
            (([&]() -> juniper::unit {
                ret = ldexp(x, exponent);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double log_(double x) {
        return (([&]() -> double {
            double guid136 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid136;
            
            (([&]() -> juniper::unit {
                ret = log(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double log10_(double x) {
        return (([&]() -> double {
            double guid137 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid137;
            
            (([&]() -> juniper::unit {
                ret = log10(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    juniper::tuple2<double,double> modf_(double x) {
        return (([&]() -> juniper::tuple2<double,double> {
            double guid138 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid138;
            
            double guid139 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double integer = guid139;
            
            (([&]() -> juniper::unit {
                ret = modf(x, &integer);
                return {};
            })());
            return (juniper::tuple2<double,double>{ret, integer});
        })());
    }
}

namespace Math {
    double pow_(double x, double y) {
        return (([&]() -> double {
            double guid140 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid140;
            
            (([&]() -> juniper::unit {
                ret = pow(x, y);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double sqrt_(double x) {
        return (([&]() -> double {
            double guid141 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid141;
            
            (([&]() -> juniper::unit {
                ret = sqrt(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double ceil_(double x) {
        return (([&]() -> double {
            double guid142 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid142;
            
            (([&]() -> juniper::unit {
                ret = ceil(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double fabs_(double x) {
        return (([&]() -> double {
            double guid143 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid143;
            
            (([&]() -> juniper::unit {
                ret = fabs(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double floor_(double x) {
        return (([&]() -> double {
            double guid144 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid144;
            
            (([&]() -> juniper::unit {
                ret = floor(x);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double fmod_(double x, double y) {
        return (([&]() -> double {
            double guid145 = 0.000000;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            double ret = guid145;
            
            (([&]() -> juniper::unit {
                ret = fmod(x, y);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Math {
    double round_(double x) {
        return floor_((x + 0.500000));
    }
}

namespace Math {
    template<typename t1009>
    t1009 min_(t1009 x, t1009 y) {
        return (([&]() -> t1009 {
            using a = t1009;
            return ((x > y) ? 
                y
            :
                x);
        })());
    }
}

namespace Math {
    template<typename a>
    a max_(a x, a y) {
        return ((x > y) ? 
            x
        :
            y);
    }
}

namespace Math {
    double mapRange(double x, double a1, double a2, double b1, double b2) {
        return (b1 + (((x - a1) * (b2 - b1)) / (a2 - a1)));
    }
}

namespace Math {
    template<typename t1013>
    t1013 clamp(t1013 x, t1013 min, t1013 max) {
        return (([&]() -> t1013 {
            using a = t1013;
            return ((min > x) ? 
                min
            :
                ((x > max) ? 
                    max
                :
                    x));
        })());
    }
}

namespace Math {
    template<typename t1018>
    int8_t sign(t1018 n) {
        return (([&]() -> int8_t {
            using a = t1018;
            return ((n == ((t1018) 0)) ? 
                ((int8_t) 0)
            :
                ((n > ((t1018) 0)) ? 
                    ((int8_t) 1)
                :
                    -(((int8_t) 1))));
        })());
    }
}

namespace Button {
    juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>> state() {
        return (juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>>(new juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>((([&]() -> juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>{
            juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState> guid146;
            guid146.actualState = Io::low();
            guid146.lastState = Io::low();
            guid146.lastDebounceTime = ((uint32_t) 0);
            return guid146;
        })()))));
    }
}

namespace Button {
    Prelude::sig<Io::pinState> debounceDelay(Prelude::sig<Io::pinState> incoming, uint16_t delay, juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>> buttonState) {
        return Signal::map<Io::pinState, Io::pinState, juniper::closures::closuret_8<juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>>, uint16_t>>(juniper::function<juniper::closures::closuret_8<juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>>, uint16_t>, Io::pinState(Io::pinState)>(juniper::closures::closuret_8<juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>>, uint16_t>(buttonState, delay), [](juniper::closures::closuret_8<juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>>, uint16_t>& junclosure, Io::pinState currentState) -> Io::pinState { 
            juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>>& buttonState = junclosure.buttonState;
            uint16_t& delay = junclosure.delay;
            return (([&]() -> Io::pinState {
                juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState> guid147 = (*((buttonState).get()));
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                uint32_t lastDebounceTime = (guid147).lastDebounceTime;
                Io::pinState lastState = (guid147).lastState;
                Io::pinState actualState = (guid147).actualState;
                
                return ((currentState != lastState) ? 
                    (([&]() -> Io::pinState {
                        (*((juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>*) (buttonState.get())) = (([&]() -> juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>{
                            juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState> guid148;
                            guid148.actualState = actualState;
                            guid148.lastState = currentState;
                            guid148.lastDebounceTime = Time::now();
                            return guid148;
                        })()));
                        return actualState;
                    })())
                :
                    (((currentState != actualState) && ((Time::now() - ((*((buttonState).get()))).lastDebounceTime) > delay)) ? 
                        (([&]() -> Io::pinState {
                            (*((juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>*) (buttonState.get())) = (([&]() -> juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>{
                                juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState> guid149;
                                guid149.actualState = currentState;
                                guid149.lastState = currentState;
                                guid149.lastDebounceTime = lastDebounceTime;
                                return guid149;
                            })()));
                            return currentState;
                        })())
                    :
                        (([&]() -> Io::pinState {
                            (*((juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>*) (buttonState.get())) = (([&]() -> juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>{
                                juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState> guid150;
                                guid150.actualState = actualState;
                                guid150.lastState = currentState;
                                guid150.lastDebounceTime = lastDebounceTime;
                                return guid150;
                            })()));
                            return actualState;
                        })())));
            })());
         }), incoming);
    }
}

namespace Button {
    Prelude::sig<Io::pinState> debounce(Prelude::sig<Io::pinState> incoming, juniper::shared_ptr<juniper::records::recordt_2<Io::pinState, uint32_t, Io::pinState>> buttonState) {
        return debounceDelay(incoming, ((uint16_t) 50), buttonState);
    }
}

namespace Vector {
    uint8_t x = ((uint8_t) 0);
}

namespace Vector {
    uint8_t y = ((uint8_t) 1);
}

namespace Vector {
    uint8_t z = ((uint8_t) 2);
}

namespace Vector {
    template<typename t1060, int c76>
    juniper::records::recordt_3<juniper::array<t1060, c76>> make(juniper::array<t1060, c76> d) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1060, c76>> {
            constexpr int32_t n = c76;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1060, c76>>{
                juniper::records::recordt_3<juniper::array<t1060, c76>> guid151;
                guid151.data = d;
                return guid151;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1062, int c77>
    t1062 get(uint32_t i, juniper::records::recordt_3<juniper::array<t1062, c77>> v) {
        return (([&]() -> t1062 {
            constexpr int32_t n = c77;
            return ((v).data)[i];
        })());
    }
}

namespace Vector {
    template<typename t1072, int c79>
    juniper::records::recordt_3<juniper::array<t1072, c79>> add(juniper::records::recordt_3<juniper::array<t1072, c79>> v1, juniper::records::recordt_3<juniper::array<t1072, c79>> v2) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1072, c79>> {
            constexpr int32_t n = c79;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1072, c79>> {
                juniper::records::recordt_3<juniper::array<t1072, c79>> guid152 = v1;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_3<juniper::array<t1072, c79>> result = guid152;
                
                (([&]() -> juniper::unit {
                    int32_t guid153 = ((int32_t) 0);
                    int32_t guid154 = (n - ((int32_t) 1));
                    for (int32_t i = guid153; i <= guid154; i++) {
                        (([&]() -> juniper::unit {
                            (((result).data)[i] = (((result).data)[i] + ((v2).data)[i]));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return result;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1076, int c83>
    juniper::records::recordt_3<juniper::array<t1076, c83>> zero() {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1076, c83>> {
            constexpr int32_t n = c83;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1076, c83>>{
                juniper::records::recordt_3<juniper::array<t1076, c83>> guid155;
                guid155.data = (juniper::array<t1076, c83>().fill(((t1076) 0)));
                return guid155;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1085, int c84>
    juniper::records::recordt_3<juniper::array<t1085, c84>> subtract(juniper::records::recordt_3<juniper::array<t1085, c84>> v1, juniper::records::recordt_3<juniper::array<t1085, c84>> v2) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1085, c84>> {
            constexpr int32_t n = c84;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1085, c84>> {
                juniper::records::recordt_3<juniper::array<t1085, c84>> guid156 = v1;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_3<juniper::array<t1085, c84>> result = guid156;
                
                (([&]() -> juniper::unit {
                    int32_t guid157 = ((int32_t) 0);
                    int32_t guid158 = (n - ((int32_t) 1));
                    for (int32_t i = guid157; i <= guid158; i++) {
                        (([&]() -> juniper::unit {
                            (((result).data)[i] = (((result).data)[i] - ((v2).data)[i]));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return result;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1093, int c88>
    juniper::records::recordt_3<juniper::array<t1093, c88>> scale(t1093 scalar, juniper::records::recordt_3<juniper::array<t1093, c88>> v) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1093, c88>> {
            constexpr int32_t n = c88;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1093, c88>> {
                juniper::records::recordt_3<juniper::array<t1093, c88>> guid159 = v;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_3<juniper::array<t1093, c88>> result = guid159;
                
                (([&]() -> juniper::unit {
                    int32_t guid160 = ((int32_t) 0);
                    int32_t guid161 = (n - ((int32_t) 1));
                    for (int32_t i = guid160; i <= guid161; i++) {
                        (([&]() -> juniper::unit {
                            (((result).data)[i] = (((result).data)[i] * scalar));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return result;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1104, int c91>
    t1104 dot(juniper::records::recordt_3<juniper::array<t1104, c91>> v1, juniper::records::recordt_3<juniper::array<t1104, c91>> v2) {
        return (([&]() -> t1104 {
            constexpr int32_t n = c91;
            return (([&]() -> t1104 {
                t1104 guid162 = ((t1104) 0);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t1104 sum = guid162;
                
                (([&]() -> juniper::unit {
                    int32_t guid163 = ((int32_t) 0);
                    int32_t guid164 = (n - ((int32_t) 1));
                    for (int32_t i = guid163; i <= guid164; i++) {
                        (([&]() -> juniper::unit {
                            (sum = (sum + (((v1).data)[i] * ((v2).data)[i])));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return sum;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1113, int c94>
    t1113 magnitude2(juniper::records::recordt_3<juniper::array<t1113, c94>> v) {
        return (([&]() -> t1113 {
            constexpr int32_t n = c94;
            return (([&]() -> t1113 {
                t1113 guid165 = ((t1113) 0);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                t1113 sum = guid165;
                
                (([&]() -> juniper::unit {
                    int32_t guid166 = ((int32_t) 0);
                    int32_t guid167 = (n - ((int32_t) 1));
                    for (int32_t i = guid166; i <= guid167; i++) {
                        (([&]() -> juniper::unit {
                            (sum = (sum + (((v).data)[i] * ((v).data)[i])));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return sum;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1115, int c97>
    double magnitude(juniper::records::recordt_3<juniper::array<t1115, c97>> v) {
        return (([&]() -> double {
            constexpr int32_t n = c97;
            return sqrt_(toDouble<t1115>(magnitude2<t1115, c97>(v)));
        })());
    }
}

namespace Vector {
    template<typename t1133, int c98>
    juniper::records::recordt_3<juniper::array<t1133, c98>> multiply(juniper::records::recordt_3<juniper::array<t1133, c98>> u, juniper::records::recordt_3<juniper::array<t1133, c98>> v) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1133, c98>> {
            constexpr int32_t n = c98;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1133, c98>> {
                juniper::records::recordt_3<juniper::array<t1133, c98>> guid168 = u;
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_3<juniper::array<t1133, c98>> result = guid168;
                
                (([&]() -> juniper::unit {
                    int32_t guid169 = ((int32_t) 0);
                    int32_t guid170 = (n - ((int32_t) 1));
                    for (int32_t i = guid169; i <= guid170; i++) {
                        (([&]() -> juniper::unit {
                            (((result).data)[i] = (((result).data)[i] * ((v).data)[i]));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return result;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1144, int c102>
    juniper::records::recordt_3<juniper::array<t1144, c102>> normalize(juniper::records::recordt_3<juniper::array<t1144, c102>> v) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1144, c102>> {
            constexpr int32_t n = c102;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1144, c102>> {
                double guid171 = magnitude<t1144, c102>(v);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                double mag = guid171;
                
                return ((mag > ((t1144) 0)) ? 
                    (([&]() -> juniper::records::recordt_3<juniper::array<t1144, c102>> {
                        juniper::records::recordt_3<juniper::array<t1144, c102>> guid172 = v;
                        if (!(true)) {
                            juniper::quit<juniper::unit>();
                        }
                        juniper::records::recordt_3<juniper::array<t1144, c102>> result = guid172;
                        
                        (([&]() -> juniper::unit {
                            int32_t guid173 = ((int32_t) 0);
                            int32_t guid174 = (n - ((int32_t) 1));
                            for (int32_t i = guid173; i <= guid174; i++) {
                                (([&]() -> juniper::unit {
                                    (((result).data)[i] = fromDouble<t1144>((toDouble<t1144>(((result).data)[i]) / mag)));
                                    return juniper::unit();
                                })());
                            }
                            return {};
                        })());
                        return result;
                    })())
                :
                    v);
            })());
        })());
    }
}

namespace Vector {
    template<typename t1157, int c105>
    double angle(juniper::records::recordt_3<juniper::array<t1157, c105>> v1, juniper::records::recordt_3<juniper::array<t1157, c105>> v2) {
        return (([&]() -> double {
            constexpr int32_t n = c105;
            return acos_((dot<t1157, c105>(v1, v2) / sqrt_((magnitude2<t1157, c105>(v1) * magnitude2<t1157, c105>(v2)))));
        })());
    }
}

namespace Vector {
    template<typename t1175>
    juniper::records::recordt_3<juniper::array<t1175, 3>> cross(juniper::records::recordt_3<juniper::array<t1175, 3>> u, juniper::records::recordt_3<juniper::array<t1175, 3>> v) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1175, 3>> {
            using a = t1175;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1175, 3>>{
                juniper::records::recordt_3<juniper::array<t1175, 3>> guid175;
                guid175.data = (juniper::array<t1175, 3> { {((((u).data)[((uint32_t) 1)] * ((v).data)[((uint32_t) 2)]) - (((u).data)[((uint32_t) 2)] * ((v).data)[((uint32_t) 1)])), ((((u).data)[((uint32_t) 2)] * ((v).data)[((uint32_t) 0)]) - (((u).data)[((uint32_t) 0)] * ((v).data)[((uint32_t) 2)])), ((((u).data)[((uint32_t) 0)] * ((v).data)[((uint32_t) 1)]) - (((u).data)[((uint32_t) 1)] * ((v).data)[((uint32_t) 0)]))} });
                return guid175;
            })());
        })());
    }
}

namespace Vector {
    template<typename t1202, int c118>
    juniper::records::recordt_3<juniper::array<t1202, c118>> project(juniper::records::recordt_3<juniper::array<t1202, c118>> a, juniper::records::recordt_3<juniper::array<t1202, c118>> b) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1202, c118>> {
            constexpr int32_t n = c118;
            return (([&]() -> juniper::records::recordt_3<juniper::array<t1202, c118>> {
                juniper::records::recordt_3<juniper::array<t1202, c118>> guid176 = normalize<t1202, c118>(b);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_3<juniper::array<t1202, c118>> bn = guid176;
                
                return scale<t1202, c118>(dot<t1202, c118>(a, bn), bn);
            })());
        })());
    }
}

namespace Vector {
    template<typename t1215, int c119>
    juniper::records::recordt_3<juniper::array<t1215, c119>> projectPlane(juniper::records::recordt_3<juniper::array<t1215, c119>> a, juniper::records::recordt_3<juniper::array<t1215, c119>> m) {
        return (([&]() -> juniper::records::recordt_3<juniper::array<t1215, c119>> {
            constexpr int32_t n = c119;
            return subtract<t1215, c119>(a, project<t1215, c119>(a, m));
        })());
    }
}

namespace CharList {
    template<int c120>
    juniper::records::recordt_0<juniper::array<uint8_t, c120>, uint32_t> toUpper(juniper::records::recordt_0<juniper::array<uint8_t, c120>, uint32_t> str) {
        return List::map<uint8_t, uint8_t, void, c120>(juniper::function<void, uint8_t(uint8_t)>([](uint8_t c) -> uint8_t { 
            return (((c >= ((uint8_t) 97)) && (c <= ((uint8_t) 122))) ? 
                (c - ((uint8_t) 32))
            :
                c);
         }), str);
    }
}

namespace CharList {
    template<int c121>
    juniper::records::recordt_0<juniper::array<uint8_t, c121>, uint32_t> toLower(juniper::records::recordt_0<juniper::array<uint8_t, c121>, uint32_t> str) {
        return List::map<uint8_t, uint8_t, void, c121>(juniper::function<void, uint8_t(uint8_t)>([](uint8_t c) -> uint8_t { 
            return (((c >= ((uint8_t) 65)) && (c <= ((uint8_t) 90))) ? 
                (c + ((uint8_t) 32))
            :
                c);
         }), str);
    }
}

namespace CharList {
    template<int c122>
    juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t> i32ToCharList(int32_t m) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t> {
            constexpr int32_t n = c122;
            return (([&]() -> juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t> {
                juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t> guid177 = (([&]() -> juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t> guid178;
                    guid178.data = (juniper::array<uint8_t, (1)+(c122)>().fill(((uint8_t) 0)));
                    guid178.length = ((uint32_t) 0);
                    return guid178;
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c122)>, uint32_t> ret = guid177;
                
                (([&]() -> juniper::unit {
                    
    int charsPrinted = 1 + snprintf((char *) &ret.data[0], n + 1, "%d", m);
    ret.length = charsPrinted >= (n + 1) ? (n + 1) : charsPrinted;
    
                    return {};
                })());
                return ret;
            })());
        })());
    }
}

namespace CharList {
    template<int c123>
    uint32_t length(juniper::records::recordt_0<juniper::array<uint8_t, c123>, uint32_t> s) {
        return (([&]() -> uint32_t {
            constexpr int32_t n = c123;
            return ((s).length - ((uint32_t) 1));
        })());
    }
}

namespace CharList {
    template<int c124, int c125, int c126>
    juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t> concat(juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c124)>, uint32_t> sA, juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c125)>, uint32_t> sB) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t> {
            constexpr int32_t aCap = c124;
            constexpr int32_t bCap = c125;
            constexpr int32_t retCap = c126;
            return (([&]() -> juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t> {
                uint32_t guid179 = ((uint32_t) 0);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                uint32_t j = guid179;
                
                uint32_t guid180 = length<(1)+(c124)>(sA);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                uint32_t lenA = guid180;
                
                uint32_t guid181 = length<(1)+(c125)>(sB);
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                uint32_t lenB = guid181;
                
                juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t> guid182 = (([&]() -> juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t>{
                    juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t> guid183;
                    guid183.data = (juniper::array<uint8_t, (1)+(c126)>().fill(((uint8_t) 0)));
                    guid183.length = ((lenA + lenB) + ((uint32_t) 1));
                    return guid183;
                })());
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c126)>, uint32_t> out = guid182;
                
                (([&]() -> juniper::unit {
                    uint32_t guid184 = ((uint32_t) 0);
                    uint32_t guid185 = (lenA - ((uint32_t) 1));
                    for (uint32_t i = guid184; i <= guid185; i++) {
                        (([&]() -> juniper::unit {
                            (((out).data)[j] = ((sA).data)[i]);
                            (j = (j + ((uint32_t) 1)));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                (([&]() -> juniper::unit {
                    uint32_t guid186 = ((uint32_t) 0);
                    uint32_t guid187 = (lenB - ((uint32_t) 1));
                    for (uint32_t i = guid186; i <= guid187; i++) {
                        (([&]() -> juniper::unit {
                            (((out).data)[j] = ((sB).data)[i]);
                            (j = (j + ((uint32_t) 1)));
                            return juniper::unit();
                        })());
                    }
                    return {};
                })());
                return out;
            })());
        })());
    }
}

namespace CharList {
    template<int c133, int c134>
    juniper::records::recordt_0<juniper::array<uint8_t, ((1)+(c133))+(c134)>, uint32_t> safeConcat(juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c133)>, uint32_t> sA, juniper::records::recordt_0<juniper::array<uint8_t, (1)+(c134)>, uint32_t> sB) {
        return (([&]() -> juniper::records::recordt_0<juniper::array<uint8_t, ((1)+(c133))+(c134)>, uint32_t> {
            constexpr int32_t aCap = c133;
            constexpr int32_t bCap = c134;
            return concat<c133, c134, (c133)+(c134)>(sA, sB);
        })());
    }
}

namespace Random {
    int32_t random_(int32_t low, int32_t high) {
        return (([&]() -> int32_t {
            int32_t guid188 = ((int32_t) 0);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            int32_t ret = guid188;
            
            (([&]() -> juniper::unit {
                ret = random(low, high);
                return {};
            })());
            return ret;
        })());
    }
}

namespace Random {
    juniper::unit seed(uint32_t n) {
        return (([&]() -> juniper::unit {
            randomSeed(n);
            return {};
        })());
    }
}

namespace Random {
    template<typename t1279, int c138>
    t1279 choice(juniper::records::recordt_0<juniper::array<t1279, c138>, uint32_t> lst) {
        return (([&]() -> t1279 {
            constexpr int32_t n = c138;
            return (([&]() -> t1279 {
                int32_t guid189 = random_(((int32_t) 0), u32ToI32((lst).length));
                if (!(true)) {
                    juniper::quit<juniper::unit>();
                }
                int32_t i = guid189;
                
                return List::nth<t1279, c138>(i32ToU32(i), lst);
            })());
        })());
    }
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> hsvToRgb(juniper::records::recordt_6<float, float, float> color) {
        return (([&]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> {
            juniper::records::recordt_6<float, float, float> guid190 = color;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float v = (guid190).v;
            float s = (guid190).s;
            float h = (guid190).h;
            
            float guid191 = (v * s);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float c = guid191;
            
            float guid192 = (c * toFloat<double>((1.000000 - Math::fabs_((Math::fmod_((toDouble<float>(h) / ((double) 60)), 2.000000) - 1.000000)))));
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float x = guid192;
            
            float guid193 = (v - c);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float m = guid193;
            
            juniper::tuple3<float,float,float> guid194 = (((0.000000 <= h) && (h < 60.000000)) ? 
                (juniper::tuple3<float,float,float>{c, x, 0.000000})
            :
                (((60.000000 <= h) && (h < 120.000000)) ? 
                    (juniper::tuple3<float,float,float>{x, c, 0.000000})
                :
                    (((120.000000 <= h) && (h < 180.000000)) ? 
                        (juniper::tuple3<float,float,float>{0.000000, c, x})
                    :
                        (((180.000000 <= h) && (h < 240.000000)) ? 
                            (juniper::tuple3<float,float,float>{0.000000, x, c})
                        :
                            (((240.000000 <= h) && (h < 300.000000)) ? 
                                (juniper::tuple3<float,float,float>{x, 0.000000, c})
                            :
                                (juniper::tuple3<float,float,float>{c, 0.000000, x}))))));
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float bPrime = (guid194).e3;
            float gPrime = (guid194).e2;
            float rPrime = (guid194).e1;
            
            float guid195 = ((rPrime + m) * 255.000000);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float r = guid195;
            
            float guid196 = ((gPrime + m) * 255.000000);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float g = guid196;
            
            float guid197 = ((bPrime + m) * 255.000000);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            float b = guid197;
            
            return (([&]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
                juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid198;
                guid198.r = toUInt8<float>(r);
                guid198.g = toUInt8<float>(g);
                guid198.b = toUInt8<float>(b);
                return guid198;
            })());
        })());
    }
}

namespace Color {
    uint16_t rgbToRgb565(juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> color) {
        return (([&]() -> uint16_t {
            juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid199 = color;
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            uint8_t b = (guid199).b;
            uint8_t g = (guid199).g;
            uint8_t r = (guid199).r;
            
            return ((((u8ToU16(r) & ((uint16_t) 248)) << ((uint32_t) 8)) | ((u8ToU16(g) & ((uint16_t) 252)) << ((uint32_t) 3))) | (u8ToU16(b) >> ((uint32_t) 3)));
        })());
    }
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> red = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid200;
        guid200.r = ((uint8_t) 255);
        guid200.g = ((uint8_t) 0);
        guid200.b = ((uint8_t) 0);
        return guid200;
    })());
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> green = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid201;
        guid201.r = ((uint8_t) 0);
        guid201.g = ((uint8_t) 255);
        guid201.b = ((uint8_t) 0);
        return guid201;
    })());
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> blue = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid202;
        guid202.r = ((uint8_t) 0);
        guid202.g = ((uint8_t) 0);
        guid202.b = ((uint8_t) 255);
        return guid202;
    })());
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> black = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid203;
        guid203.r = ((uint8_t) 0);
        guid203.g = ((uint8_t) 0);
        guid203.b = ((uint8_t) 0);
        return guid203;
    })());
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> white = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid204;
        guid204.r = ((uint8_t) 255);
        guid204.g = ((uint8_t) 255);
        guid204.b = ((uint8_t) 255);
        return guid204;
    })());
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> yellow = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid205;
        guid205.r = ((uint8_t) 255);
        guid205.g = ((uint8_t) 255);
        guid205.b = ((uint8_t) 0);
        return guid205;
    })());
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> magenta = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid206;
        guid206.r = ((uint8_t) 255);
        guid206.g = ((uint8_t) 0);
        guid206.b = ((uint8_t) 255);
        return guid206;
    })());
}

namespace Color {
    juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> cyan = (([]() -> juniper::records::recordt_4<uint8_t, uint8_t, uint8_t>{
        juniper::records::recordt_4<uint8_t, uint8_t, uint8_t> guid207;
        guid207.r = ((uint8_t) 0);
        guid207.g = ((uint8_t) 255);
        guid207.b = ((uint8_t) 255);
        return guid207;
    })());
}

namespace Blink {
    uint16_t ledPin = ((uint16_t) 5);
}

namespace Blink {
    uint16_t buttonAPin = ((uint16_t) 9);
}

namespace Blink {
    uint16_t buttonBPin = ((uint16_t) 8);
}

namespace Blink {
    juniper::unit setup() {
        return (([&]() -> juniper::unit {
            Io::setPinMode(ledPin, Io::output());
            Io::setPinMode(buttonAPin, Io::inputPullup());
            return Io::setPinMode(buttonBPin, Io::inputPullup());
        })());
    }
}

namespace Blink {
    juniper::unit loop() {
        return (([&]() -> juniper::unit {
            Io::pinState guid208 = Io::digRead(buttonAPin);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            Io::pinState buttonASig = guid208;
            
            Io::pinState guid209 = Io::digRead(buttonBPin);
            if (!(true)) {
                juniper::quit<juniper::unit>();
            }
            Io::pinState buttonBSig = guid209;
            
            return ((buttonASig == Io::low()) ? 
                Io::digWrite(ledPin, Io::high())
            :
                ((buttonBSig == Io::low()) ? 
                    Io::digWrite(ledPin, Io::low())
                :
                    juniper::unit()));
        })());
    }
}

void setup() {
    Blink::setup();
}
void loop() {
    Blink::loop();
}