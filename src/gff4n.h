/* -*- mode: c++ -*- */
#pragma once
#include "ntl.h"

const uint32_t F4 = 65537;

template<typename T>
class Poly;

template<typename T>
class GFF4N : public GF<T>
{
private:
  T unit;
  T q;
  T h;
  GF<uint32_t> *sub_field;
  bool check_n(int n);
  void init(void);

public:
  explicit GFF4N(int n);
  ~GFF4N();
  T card(void);
  T card_minus_one(void);
  bool check(T a);
  T neg(T a);
  T add(T a, T b);
  T sub(T a, T b);
  T mul(T a, T b);
  T div(T a, T b);
  T inv(T a);
  T exp(T a, T b);
  T log(T a, T b);
  T weak_rand_tuple(void);
  T weak_rand(void);
  T get_unit(void);
  T replicate(T a);
  T pack(T a);
  T pack(T a, uint32_t flag);
  compT<T> unpack(T a);
  T get_nth_root(T n);
  void compute_omegas(Vec<T> *W, int n, T w);
  GF<uint32_t> *get_sub_field();
};

template <typename T>
GFF4N<T>::GFF4N(int n) : GF<T>(65537, n)
{
  sub_field = new GFP<uint32_t>(65537);

  if (!check_n(n)) {
    // not supported yet
    assert("Input n is not supported for GFF4N");
  }

  unit = 0;
  q = 0;
  h = 0;
  init();
}

template <typename T>
GFF4N<T>::~GFF4N()
{
  delete sub_field;
}

/*
 * Each element of sub-field GFP(65537) is stored in 32bits
 * Hence, n <= sizeof(T) / 4
 */
template <typename T>
bool GFF4N<T>::check_n(int n)
{
  return (n <= sizeof(T) / 4);
}

template <typename T>
T GFF4N<T>::replicate(T a)
{
  T b = a;
  for (int i = 1; i < this->n; i++) {
    b += (a <<= 32);
  }
  return b;
}

template <typename T>
void GFF4N<T>::init(void)
{
  unit = replicate(1);
  q = replicate(65537);
  h = replicate(65536);
}

template <typename T>
T GFF4N<T>::card(void)
{
  return q;
}

template <typename T>
T GFF4N<T>::card_minus_one(void)
{
  return h;
}

template <typename T>
T GFF4N<T>::get_unit(void)
{
  return unit;
}

template <typename T>
T GFF4N<T>::neg(T a)
{
  return sub(0, a);
}

template <typename T>
T GFF4N<T>::add(T a, T b)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->add(a & mask, b & mask);
    tmp <<= shift;
    c += tmp;
    shift += 32;
    a >>= 32;
    b >>= 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::sub(T a, T b)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->sub(a & mask, b & mask);
    tmp <<= shift;
    c += tmp;
    shift += 32;
    a >>= 32;
    b >>= 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::mul(T a, T b)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->mul(a & mask, b & mask);
    tmp <<= shift;
    c += tmp;
    shift += 32;
    a >>= 32;
    b >>= 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::div(T a, T b)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->div(a & mask, b & mask);
    tmp <<= shift;
    c += tmp;
    shift += 32;
    a >>= 32;
    b >>= 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::inv(T a)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->inv(a & mask);
    tmp <<= shift;
    c += tmp;
    shift += 32;
    a >>= 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::exp(T a, T b)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->exp(a & mask, b & mask);
    tmp <<= shift;
    c += tmp;
    shift += 32;
    a >>= 32;
    b >>= 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::log(T a, T b)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->log(a & mask, b & mask);
    tmp <<= shift;
    c += tmp;
    shift += 32;
    a >>= 32;
    b >>= 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::weak_rand_tuple(void)
{
  T c = 0;
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = sub_field->weak_rand();
    tmp <<= shift;
    c += tmp;
    shift += 32;
  }
  return c;
}

template <typename T>
T GFF4N<T>::weak_rand(void)
{
  T c = weak_rand_tuple();
  return unpack(c).val;
}

/**
 * Pack of n numbers each of 16 bits into n numbers each of 32 bits
 */
template <typename T>
T GFF4N<T>::pack(T a)
{
  uint32_t mask = 0xffff;
  int shift = 0;
  T b = 0;
  for (int i = 0; i < this->n; i++) {
    b += ((a & mask) << shift);
    shift += 32;
    a >>= 16;
  }
  return b;
}

/**
 * Pack of n numbers each of 16 bits into n numbers each of 32 bits
 *  If flag contains 2^i, ith number == 65537
 */
template <typename T>
T GFF4N<T>::pack(T a, uint32_t flag)
{
  uint32_t mask = 0xffff;
  int shift = 0;
  T b = 0;
  for (int i = 0; i < this->n; i++) {
    if (flag % 2 == 1) {
      b += ((T)65536 << shift);
    } else {
      b += ((a & mask) << shift);
    }
    flag >>= 1;
    shift += 32;
    a >>= 16;
  }
  return b;
}


/**
 * Unpack of n numbers each of 32 bits into n numbers each of 16 bits
 *  If ith number == 65537, add 2^i to flag to mark it, and consider this number
 *  as zero
 */
template <typename T>
compT<T> GFF4N<T>::unpack(T a)
{
  compT<T> b = compT<T>();
  uint32_t mask = 0xffffffff;
  int shift = 0;
  for (int i = 0; i < this->n; i++) {
    T tmp = (a & mask);
    if (tmp == 65536) {
      b.flag += (1 << i);
    } else {
      b.val += (tmp << shift);
    }
    shift += 16;
    a >>= 32;
  }
  return b;
}

// Use for fft
template <typename T>
T GFF4N<T>::get_nth_root(T n)
{
  T sub_w = sub_field->get_nth_root(n);
  T w = replicate(sub_w);
  return w;
}

template <typename T>
bool GFF4N<T>::check(T a)
{
  return (a >= 0 && a < q);
}

/**
 * Compute the different powers of the root of unity into a vector
 *
 * @param W output vector (must be of length n+1)
 * @param n
 * @param w n-th root of unity
 */
template <typename T>
void GFF4N<T>::compute_omegas(Vec<T> *W, int n, T w)
{
  for (int i = 0; i <= n; i++) {
    W->set(i, this->exp(w, replicate(i)));
  }
}

template <typename T>
GF<uint32_t> *GFF4N<T>::get_sub_field()
{
  return sub_field;
}