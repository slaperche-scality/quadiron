/* -*- mode: c++ -*- */
#pragma once

/**
 * GF_2^2^k+1 based RS (Fermat Number Transform)
 * As suggested by the paper:
 * FNT-based Reed-Solomon Erasure Codes
 * by Alexandre Soro and Jerome Lacan
 */
template<typename T>
class FECFNTRS : public FEC<T>
{
 private:
  FFTN<T> *fft = NULL;

 public:
  u_int l;
  u_int n;
  u_int r;

  FECFNTRS(GF<T> *gf, u_int word_size, u_int n_data, u_int n_parities) :
    FEC<T>(gf, FEC<T>::TYPE_2, word_size, n_data, n_parities)
  {
    u_int q = 65537;
    u_int R = 3;  // primitive root
    // order of a number is the lowest power of the number equals 1.
    // if q=65537 in GF_q 3 is a primitive root because is order is q-1:
    // 3^(q-1) = 1 mod q, and for each i such as 0 < i < q-1, 3^i mod q != 1
    assert(gf->_jacobi(R, q) == -1);

    // with this encoder we cannot exactly satisfy users request, we need to pad
    l = __gf64._log2(n_parities + n_data) + 1;
    n = __gf64._exp(2, l);

    // compute root of order n-1 such as r^(n-1) mod q == 1
    // formula given in the paper (very large numbers):
    mpz_class p = gf->_to_mpz_class(gf->p);
    mpz_class _r = __gfmpz._exp(R, __gfmpz._exp(2, 16-l)) % p;
    r = _r.get_ui();

    // std::cerr << "l=" << l << "\n";
    // std::cerr << "n=" << n << "\n";
    // std::cerr << "r=" << r << "\n";

    this->fft = new FFTN<T>(gf, n, r);
  }

  ~FECFNTRS()
  {
    delete fft;
  }

  int get_n_outputs()
  {
    return this->n;
  }

  /**
   * Encode vector
   *
   * @param output must be n
   * @param props must be exactly n
   * @param offset used to locate special values
   * @param words must be n_data
   */
  void encode(Vec<T> *output, std::vector<KeyValue*> props, off_t offset,
    Vec<T> *words)
  {
    VVec<T> vwords(words, n);
    fft->fft(output, &vwords);
    // check for 65536 value in output
    for (int i = 0; i < n; i++) {
      if (output->get(i) == 65536) {
        char buf[256];
        snprintf(buf, sizeof (buf), "%lu:%d", offset, i);
        assert(nullptr != props[i]);
        props[i]->insert(std::make_pair(buf, "65536"));
        output->set(i, 0);
      }
    }
  }

  void decode_add_data(int fragment_index, int row)
  {
    // not applicable
    assert(false);
  }

  void decode_add_parities(int fragment_index, int row)
  {
    // we can't anticipate here
  }

  void decode_build()
  {
    // nothing to do
  }

  /**
   * Perform a Lagrange interpolation to find the coefficients of the polynomial
   *
   * @note If all fragments are available ifft(words) is enough
   *
   * @param output must be exactly n_data
   * @param props special values dictionary must be exactly n_data
   * @param offset used to locate special values
   * @param fragments_ids unused
   * @param words v=(v_0, v_1, ..., v_k-1) k must be exactly n_data
   */
  void decode(Vec<T> *output, std::vector<KeyValue*> props, off_t offset,
    Vec<T> *fragments_ids, Vec<T> *words)
  {
    int k = this->n_data;  // number of fragments received
    // vector x=(x_0, x_1, ..., x_k-1)
    Vec<T> vx(this->gf, n);
    for (int i = 0; i < k; i++) {
      vx.set(i, this->gf->exp(r, fragments_ids->get(i)));
    }

    for (int i = 0; i < k; i++) {
      int j = fragments_ids->get(i);
      char buf[256];
      snprintf(buf, sizeof (buf), "%lu:%d", offset, j);
      if (nullptr != props[j]) {
        if (props[j]->is_key(buf))
          words->set(i, 65536);
      }
    }

    // Lagrange interpolation
    Poly<T> A(this->gf), _A(this->gf);

    // compute A(x) = prod_j(x-x_j)
    A.set(0, 1);
    for (int i = 0; i < k; i++) {
      Poly<T> _t(this->gf);
      _t.set(1, 1);
      _t.set(0, this->gf->sub(0, vx.get(i)));
      // _t.dump();
      A.mul(&_t);
    }
    // std::cout << "A(x)="; A.dump();

    // compute A'(x) since A_i(x_i) = A'_i(x_i)
    _A.copy(&A);
    _A.derivative();
    // std::cout << "A'(x)="; _A.dump();

    // evaluate n_i=v_i/A'_i(x_i)
    Vec<T> _n(this->gf, k);
    for (int i = 0; i < k; i++) {
      _n.set(i,
             this->gf->div(words->get(i),
                           _A.eval(vx.get(i))));
    }
    // std::cout << "_n="; _n.dump();

    // We have to find the numerator of the following expression:
    // P(x)/A(x) = sum_i=0_k-1(n_i/(x-x_i)) mod x^n
    // using Taylor series we rewrite the expression into
    // P(x)/A(x) = -sum_i=0_k-1(sum_j=0_n-1(n_i*x_i^(-j-1)*x^j))

    Poly<T> S2(this->gf);
    for (int i = 0; i <= k-1; i++) {
      Poly<T> S1(this->gf);
      for (int j = 0; j <= n-1; j++) {
        Poly<T> _t(this->gf);
        T tmp1 = this->gf->exp(vx.get(i), j+1);
        T tmp2 = this->gf->inv(tmp1);
        T tmp3 = this->gf->mul(_n.get(i), tmp2);
        _t.set(j, tmp3);
        // std::cout << "_t="; _t.dump();
        S1.add(&_t);
        // std::cout << "S1="; S1.dump();
      }
      // std::cout << "S1="; S1.dump();
      S2.add(&S1);
      // std::cout << "S2="; S2.dump();
    }
    S2.neg();
    S2.mul(&A);
    // std::cout << "xA => S2="; S2.dump();
    Poly<T> X(this->gf);
    X.set(n, 1);
    // std::cout << "X="; X.dump();
    S2.mod(&X);

    // output is n_data length
    for (int i = 0; i < this->n_data; i++)
      output->set(i, S2.get(i));
  }
};
