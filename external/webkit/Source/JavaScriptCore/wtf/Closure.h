/*
 * Copyright (C) 2011, NVIDIA CORPORATION
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Closure_h
#define Closure_h

#include <wtf/Lambda.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Assertions.h>
#include <wtf/TypeTraits.h>

namespace WTF {

template<typename T>
inline PassOwnArrayPtr<T> makeLambdaArrayArg(const T* array, size_t count)
{
    COMPILE_ASSERT(IsPod<T>::value, WTF_IsPod_T_in_makeLambdaArrayArg);
    if (!array || !count)
        return nullptr;

    T* newArray = new T[count];
    memcpy(newArray, array, sizeof(array[0]) * count);
    return adoptArrayPtr(newArray);
}

inline PassOwnArrayPtr<char> makeLambdaArrayArg(const void* array, size_t count)
{
    return makeLambdaArrayArg(reinterpret_cast<const char*>(array), count);
}

template<typename F>
struct Closure;

template<typename R, typename F>
class ClosureBase : public ReturnLambda<R> {
public:
    virtual void call()
    {
        retval = reinterpret_cast<Closure<F>*>(this)->callInternal();
    }
    virtual R ret()
    {
        return retval;
    }
private:
    R retval;
};

template<typename F>
class ClosureBase<void, F> : public Lambda {
public:
    virtual void call()
    {
        reinterpret_cast<Closure<F>*>(this)->callInternal();
    }
};

template<typename T> struct ClosureStorage {
    typedef T t;
};

template<typename T> inline T& closureArg(T& arg)
{
    return arg;
}

template<typename R>
class Closure<R (*)()> : public ClosureBase<R, R (*)()> {
public:
    Closure(R (*const func_)())
        : func(func_) { }
    inline R callInternal()
    {
        return func();
    }
private:
    R (*const func)();
};

template<typename R, typename A>
class Closure<R (*)(A)> : public ClosureBase<R, R (*)(A)> {
public:
    Closure(R (*const func_)(A), typename ClosureStorage<A>::t const arg1_)
        : func(func_), arg1(arg1_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1));
    }
private:
    R (*const func)(A);
    typename ClosureStorage<A>::t const arg1;
};

template<typename R, typename A, typename B>
class Closure<R (*)(A, B)> : public ClosureBase<R, R (*)(A, B)> {
public:
    Closure(R (*const func_)(A, B), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_)
        : func(func_), arg1(arg1_), arg2(arg2_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2));
    }
private:
    R (*const func)(A, B);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
};

template<typename R, typename A, typename B, typename C>
class Closure<R (*)(A, B, C)> : public ClosureBase<R, R (*)(A, B, C)> {
public:
    Closure(R (*const func_)(A, B, C), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3));
    }
private:
    R (*const func)(A, B, C);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
};

template<typename R, typename A, typename B, typename C, typename D>
class Closure<R (*)(A, B, C, D)> : public ClosureBase<R, R (*)(A, B, C, D)> {
public:
    Closure(R (*const func_)(A, B, C, D), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4));
    }
private:
    R (*const func)(A, B, C, D);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
};

template<typename R, typename A, typename B, typename C, typename D, typename E>
class Closure<R (*)(A, B, C, D, E)> : public ClosureBase<R, R (*)(A, B, C, D, E)> {
public:
    Closure(R (*const func_)(A, B, C, D, E), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5));
    }
private:
    R (*const func)(A, B, C, D, E);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F>
class Closure<R (*)(A, B, C, D, E, F)> : public ClosureBase<R, R (*)(A, B, C, D, E, F)> {
public:
    Closure(R (*const func_)(A, B, C, D, E, F), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6));
    }
private:
    R (*const func)(A, B, C, D, E, F);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
class Closure<R (*)(A, B, C, D, E, F, G)> : public ClosureBase<R, R (*)(A, B, C, D, E, F, G)> {
public:
    Closure(R (*const func_)(A, B, C, D, E, F, G), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7));
    }
private:
    R (*const func)(A, B, C, D, E, F, G);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
class Closure<R (*)(A, B, C, D, E, F, G, H)> : public ClosureBase<R, R (*)(A, B, C, D, E, F, G, H)> {
public:
    Closure(R (*const func_)(A, B, C, D, E, F, G, H), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8));
    }
private:
    R (*const func)(A, B, C, D, E, F, G, H);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
class Closure<R (*)(A, B, C, D, E, F, G, H, I)> : public ClosureBase<R, R (*)(A, B, C, D, E, F, G, H, I)> {
public:
    Closure(R (*const func_)(A, B, C, D, E, F, G, H, I), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9));
    }
private:
    R (*const func)(A, B, C, D, E, F, G, H, I);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
class Closure<R (*)(A, B, C, D, E, F, G, H, I, J)> : public ClosureBase<R, R (*)(A, B, C, D, E, F, G, H, I, J)> {
public:
    Closure(R (*const func_)(A, B, C, D, E, F, G, H, I, J), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_, typename ClosureStorage<J>::t const arg10_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_), arg10(arg10_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9), closureArg(arg10));
    }
private:
    R (*const func)(A, B, C, D, E, F, G, H, I, J);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
    typename ClosureStorage<J>::t const arg10;
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
class Closure<R (*)(A, B, C, D, E, F, G, H, I, J, K)> : public ClosureBase<R, R (*)(A, B, C, D, E, F, G, H, I, J, K)> {
public:
    Closure(R (*const func_)(A, B, C, D, E, F, G, H, I, J, K), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_, typename ClosureStorage<J>::t const arg10_, typename ClosureStorage<K>::t const arg11_)
        : func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_), arg10(arg10_), arg11(arg11_) { }
    inline R callInternal()
    {
        return func(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9), closureArg(arg10), closureArg(arg11));
    }
private:
    R (*const func)(A, B, C, D, E, F, G, H, I, J, K);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
    typename ClosureStorage<J>::t const arg10;
    typename ClosureStorage<K>::t const arg11;
};

template<typename R, typename T>
class Closure<R (T::*)()> : public ClosureBase<R, R (T::*)()> {
public:
    Closure(T* const ptr_, R (T::*const func_)())
        : ptr(ptr_), func(func_) { }
    inline R callInternal()
    {
        return (ptr->*func)();
    }
private:
    T* const ptr;
    R (T::*const func)();
};

template<typename R, typename T, typename A>
class Closure<R (T::*)(A)> : public ClosureBase<R, R (T::*)(A)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A), typename ClosureStorage<A>::t const arg1_)
        : ptr(ptr_), func(func_), arg1(arg1_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1));
    }
private:
    T* const ptr;
    R (T::*const func)(A);
    typename ClosureStorage<A>::t const arg1;
};

template<typename R, typename T, typename A, typename B>
class Closure<R (T::*)(A, B)> : public ClosureBase<R, R (T::*)(A, B)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
};

template<typename R, typename T, typename A, typename B, typename C>
class Closure<R (T::*)(A, B, C)> : public ClosureBase<R, R (T::*)(A, B, C)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
};

template<typename R, typename T, typename A, typename B, typename C, typename D>
class Closure<R (T::*)(A, B, C, D)> : public ClosureBase<R, R (T::*)(A, B, C, D)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E>
class Closure<R (T::*)(A, B, C, D, E)> : public ClosureBase<R, R (T::*)(A, B, C, D, E)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D, E), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D, E);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F>
class Closure<R (T::*)(A, B, C, D, E, F)> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
class Closure<R (T::*)(A, B, C, D, E, F, G)> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
class Closure<R (T::*)(A, B, C, D, E, F, G, H)> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
class Closure<R (T::*)(A, B, C, D, E, F, G, H, I)> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H, I)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
class Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J)> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H, I, J)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_, typename ClosureStorage<J>::t const arg10_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_), arg10(arg10_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9), closureArg(arg10));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
    typename ClosureStorage<J>::t const arg10;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
class Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J, K)> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H, I, J, K)> {
public:
    Closure(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J, K), typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_, typename ClosureStorage<J>::t const arg10_, typename ClosureStorage<K>::t const arg11_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_), arg10(arg10_), arg11(arg11_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9), closureArg(arg10), closureArg(arg11));
    }
private:
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J, K);
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
    typename ClosureStorage<J>::t const arg10;
    typename ClosureStorage<K>::t const arg11;
};

template<typename R, typename T>
class Closure<R (T::*)() const> : public ClosureBase<R, R (T::*)()> {
public:
    Closure(const T* const ptr_, R (T::*const func_)() const)
        : ptr(ptr_), func(func_) { }
    inline R callInternal()
    {
        return (ptr->*func)();
    }
private:
    const T* const ptr;
    R (T::*const func)() const;
};

template<typename R, typename T, typename A>
class Closure<R (T::*)(A) const> : public ClosureBase<R, R (T::*)(A)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A) const, typename ClosureStorage<A>::t const arg1_)
        : ptr(ptr_), func(func_), arg1(arg1_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1));
    }
private:
    const T* const ptr;
    R (T::*const func)(A) const;
    typename ClosureStorage<A>::t const arg1;
};

template<typename R, typename T, typename A, typename B>
class Closure<R (T::*)(A, B) const> : public ClosureBase<R, R (T::*)(A, B)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
};

template<typename R, typename T, typename A, typename B, typename C>
class Closure<R (T::*)(A, B, C) const> : public ClosureBase<R, R (T::*)(A, B, C)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
};

template<typename R, typename T, typename A, typename B, typename C, typename D>
class Closure<R (T::*)(A, B, C, D) const> : public ClosureBase<R, R (T::*)(A, B, C, D)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E>
class Closure<R (T::*)(A, B, C, D, E) const> : public ClosureBase<R, R (T::*)(A, B, C, D, E)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D, E) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F>
class Closure<R (T::*)(A, B, C, D, E, F) const> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
class Closure<R (T::*)(A, B, C, D, E, F, G) const> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
class Closure<R (T::*)(A, B, C, D, E, F, G, H) const> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
class Closure<R (T::*)(A, B, C, D, E, F, G, H, I) const> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H, I)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
class Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J) const> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H, I, J)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_, typename ClosureStorage<J>::t const arg10_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_), arg10(arg10_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9), closureArg(arg10));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
    typename ClosureStorage<J>::t const arg10;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
class Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J, K) const> : public ClosureBase<R, R (T::*)(A, B, C, D, E, F, G, H, I, J, K)> {
public:
    Closure(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J, K) const, typename ClosureStorage<A>::t const arg1_, typename ClosureStorage<B>::t const arg2_, typename ClosureStorage<C>::t const arg3_, typename ClosureStorage<D>::t const arg4_, typename ClosureStorage<E>::t const arg5_, typename ClosureStorage<F>::t const arg6_, typename ClosureStorage<G>::t const arg7_, typename ClosureStorage<H>::t const arg8_, typename ClosureStorage<I>::t const arg9_, typename ClosureStorage<J>::t const arg10_, typename ClosureStorage<K>::t const arg11_)
        : ptr(ptr_), func(func_), arg1(arg1_), arg2(arg2_), arg3(arg3_), arg4(arg4_), arg5(arg5_), arg6(arg6_), arg7(arg7_), arg8(arg8_), arg9(arg9_), arg10(arg10_), arg11(arg11_) { }
    inline R callInternal()
    {
        return (ptr->*func)(closureArg(arg1), closureArg(arg2), closureArg(arg3), closureArg(arg4), closureArg(arg5), closureArg(arg6), closureArg(arg7), closureArg(arg8), closureArg(arg9), closureArg(arg10), closureArg(arg11));
    }
private:
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J, K) const;
    typename ClosureStorage<A>::t const arg1;
    typename ClosureStorage<B>::t const arg2;
    typename ClosureStorage<C>::t const arg3;
    typename ClosureStorage<D>::t const arg4;
    typename ClosureStorage<E>::t const arg5;
    typename ClosureStorage<F>::t const arg6;
    typename ClosureStorage<G>::t const arg7;
    typename ClosureStorage<H>::t const arg8;
    typename ClosureStorage<I>::t const arg9;
    typename ClosureStorage<J>::t const arg10;
    typename ClosureStorage<K>::t const arg11;
};

template<typename R>
struct LambdaType {
    typedef PassOwnPtr<ReturnLambda<R> > t;
};
template<>
struct LambdaType<void> {
    typedef PassOwnPtr<Lambda> t;
};

template<typename F>
struct LambdaLifter;

template<typename R>
struct LambdaLifter<R (*)()> {
    inline LambdaLifter(R (*const func_)()) : func(func_) { }
    inline typename LambdaType<R>::t operator()() const
    {
        return adoptPtr(new Closure<R (*)()>(func));
    };
    R (*const func)();
};

template<typename R, typename A>
struct LambdaLifter<R (*)(A)> {
    inline LambdaLifter(R (*const func_)(A)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1) const
    {
        return adoptPtr(new Closure<R (*)(A)>(func, arg1));
    };
    R (*const func)(A);
};

template<typename R, typename A, typename B>
struct LambdaLifter<R (*)(A, B)> {
    inline LambdaLifter(R (*const func_)(A, B)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2) const
    {
        return adoptPtr(new Closure<R (*)(A, B)>(func, arg1, arg2));
    };
    R (*const func)(A, B);
};

template<typename R, typename A, typename B, typename C>
struct LambdaLifter<R (*)(A, B, C)> {
    inline LambdaLifter(R (*const func_)(A, B, C)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C)>(func, arg1, arg2, arg3));
    };
    R (*const func)(A, B, C);
};

template<typename R, typename A, typename B, typename C, typename D>
struct LambdaLifter<R (*)(A, B, C, D)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D)>(func, arg1, arg2, arg3, arg4));
    };
    R (*const func)(A, B, C, D);
};

template<typename R, typename A, typename B, typename C, typename D, typename E>
struct LambdaLifter<R (*)(A, B, C, D, E)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D, E)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D, E)>(func, arg1, arg2, arg3, arg4, arg5));
    };
    R (*const func)(A, B, C, D, E);
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F>
struct LambdaLifter<R (*)(A, B, C, D, E, F)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D, E, F)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D, E, F)>(func, arg1, arg2, arg3, arg4, arg5, arg6));
    };
    R (*const func)(A, B, C, D, E, F);
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
struct LambdaLifter<R (*)(A, B, C, D, E, F, G)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D, E, F, G)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D, E, F, G)>(func, arg1, arg2, arg3, arg4, arg5, arg6, arg7));
    };
    R (*const func)(A, B, C, D, E, F, G);
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
struct LambdaLifter<R (*)(A, B, C, D, E, F, G, H)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D, E, F, G, H)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D, E, F, G, H)>(func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
    };
    R (*const func)(A, B, C, D, E, F, G, H);
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
struct LambdaLifter<R (*)(A, B, C, D, E, F, G, H, I)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D, E, F, G, H, I)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D, E, F, G, H, I)>(func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));
    };
    R (*const func)(A, B, C, D, E, F, G, H, I);
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
struct LambdaLifter<R (*)(A, B, C, D, E, F, G, H, I, J)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D, E, F, G, H, I, J)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9, typename ClosureStorage<J>::t const arg10) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D, E, F, G, H, I, J)>(func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10));
    };
    R (*const func)(A, B, C, D, E, F, G, H, I, J);
};

template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
struct LambdaLifter<R (*)(A, B, C, D, E, F, G, H, I, J, K)> {
    inline LambdaLifter(R (*const func_)(A, B, C, D, E, F, G, H, I, J, K)) : func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9, typename ClosureStorage<J>::t const arg10, typename ClosureStorage<K>::t const arg11) const
    {
        return adoptPtr(new Closure<R (*)(A, B, C, D, E, F, G, H, I, J, K)>(func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11));
    };
    R (*const func)(A, B, C, D, E, F, G, H, I, J, K);
};

template<typename R, typename T>
struct LambdaLifter<R (T::*)()> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)()) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()() const
    {
        return adoptPtr(new Closure<R (T::*)()>(ptr, func));
    };
    T* const ptr;
    R (T::*const func)();
};

template<typename R, typename T, typename A>
struct LambdaLifter<R (T::*)(A)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1) const
    {
        return adoptPtr(new Closure<R (T::*)(A)>(ptr, func, arg1));
    };
    T* const ptr;
    R (T::*const func)(A);
};

template<typename R, typename T, typename A, typename B>
struct LambdaLifter<R (T::*)(A, B)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B)>(ptr, func, arg1, arg2));
    };
    T* const ptr;
    R (T::*const func)(A, B);
};

template<typename R, typename T, typename A, typename B, typename C>
struct LambdaLifter<R (T::*)(A, B, C)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C)>(ptr, func, arg1, arg2, arg3));
    };
    T* const ptr;
    R (T::*const func)(A, B, C);
};

template<typename R, typename T, typename A, typename B, typename C, typename D>
struct LambdaLifter<R (T::*)(A, B, C, D)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D)>(ptr, func, arg1, arg2, arg3, arg4));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D);
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E>
struct LambdaLifter<R (T::*)(A, B, C, D, E)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D, E)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E)>(ptr, func, arg1, arg2, arg3, arg4, arg5));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D, E);
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F)>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F);
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G)>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G);
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H)>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H);
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H, I)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H, I)>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I);
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H, I, J)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9, typename ClosureStorage<J>::t const arg10) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J)>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J);
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H, I, J, K)> {
    inline LambdaLifter(T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J, K)) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9, typename ClosureStorage<J>::t const arg10, typename ClosureStorage<K>::t const arg11) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J, K)>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11));
    };
    T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J, K);
};

template<typename R, typename T>
struct LambdaLifter<R (T::*)() const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)() const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()() const
    {
        return adoptPtr(new Closure<R (T::*)() const>(ptr, func));
    };
    const T* const ptr;
    R (T::*const func)() const;
};

template<typename R, typename T, typename A>
struct LambdaLifter<R (T::*)(A) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1) const
    {
        return adoptPtr(new Closure<R (T::*)(A) const>(ptr, func, arg1));
    };
    const T* const ptr;
    R (T::*const func)(A) const;
};

template<typename R, typename T, typename A, typename B>
struct LambdaLifter<R (T::*)(A, B) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B) const>(ptr, func, arg1, arg2));
    };
    const T* const ptr;
    R (T::*const func)(A, B) const;
};

template<typename R, typename T, typename A, typename B, typename C>
struct LambdaLifter<R (T::*)(A, B, C) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C) const>(ptr, func, arg1, arg2, arg3));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D>
struct LambdaLifter<R (T::*)(A, B, C, D) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D) const>(ptr, func, arg1, arg2, arg3, arg4));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E>
struct LambdaLifter<R (T::*)(A, B, C, D, E) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D, E) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E) const>(ptr, func, arg1, arg2, arg3, arg4, arg5));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F) const>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G) const>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H) const>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H, I) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H, I) const>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H, I, J) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9, typename ClosureStorage<J>::t const arg10) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J) const>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J) const;
};

template<typename R, typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
struct LambdaLifter<R (T::*)(A, B, C, D, E, F, G, H, I, J, K) const> {
    inline LambdaLifter(const T* const ptr_, R (T::*const func_)(A, B, C, D, E, F, G, H, I, J, K) const) : ptr(ptr_), func(func_) { }
    inline typename LambdaType<R>::t operator()(typename ClosureStorage<A>::t const arg1, typename ClosureStorage<B>::t const arg2, typename ClosureStorage<C>::t const arg3, typename ClosureStorage<D>::t const arg4, typename ClosureStorage<E>::t const arg5, typename ClosureStorage<F>::t const arg6, typename ClosureStorage<G>::t const arg7, typename ClosureStorage<H>::t const arg8, typename ClosureStorage<I>::t const arg9, typename ClosureStorage<J>::t const arg10, typename ClosureStorage<K>::t const arg11) const
    {
        return adoptPtr(new Closure<R (T::*)(A, B, C, D, E, F, G, H, I, J, K) const>(ptr, func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11));
    };
    const T* const ptr;
    R (T::*const func)(A, B, C, D, E, F, G, H, I, J, K) const;
};

template<typename F>
inline LambdaLifter<F> makeLambda(F func)
{
    return LambdaLifter<F>(func);
}

template<typename T, typename F>
inline LambdaLifter<F> makeLambda(T* const ptr, F func)
{
    return LambdaLifter<F>(ptr, func);
}

template<typename T, typename F>
inline LambdaLifter<F> makeLambda(const T* const ptr, F func)
{
    return LambdaLifter<F>(ptr, func);
}

} // namespace WTF

#endif

