#include "llik2.h"

////////////////////////////////////////////////////////////////////////////////
// Negative Binomial 2
// R , sigma=scale
struct nbinom_llik {
  const Eigen::VectorXi y_;
  const Eigen::VectorXi N_;
  
  nbinom_llik(const Eigen::VectorXi& y, Eigen::VectorXi& N) : y_(y), N_(N) { }

  template <typename T>
  Eigen::Matrix<T, -1, 1> operator()(const Eigen::Matrix<T, -1, 1>& theta) const {
    T p = theta[0]; // prob = size/(size+mu); size/prob-size = mu
		
    Eigen::Matrix<T, -1, 1> lp(y_.size());
    for (int i = 0; i < y_.size(); ++i) {
      T mu = (double)(N_[i])*(1.0-p)/p;
      lp[i] = stan::math::neg_binomial_2_lpmf(y_[i], mu, N_[i]);
    }
    return lp;
  }
};


stanLl llik_nbinom(Eigen::VectorXi& y, Eigen::VectorXi& N, Eigen::VectorXd& params) {
  nbinom_llik f(y, N);
  Eigen::VectorXd fx;
  Eigen::Matrix<double, -1, -1> J;
  stan::math::jacobian(f, params, fx, J);
  stanLl ret;
  ret.fx = fx;
  ret.J  = J;
  return ret;
}

static inline void llikNbinomFull(double* ret, double x, double size, double prob) {
  if (ret[0] == isNbinom &&
      ret[1] == x &&
      ret[2] == size &&
      ret[3] == prob) {
    // Assume this is the same
    return;
  }
  if (!R_finite(x) || !R_finite(size) || !R_finite(prob)) {
    ret[0] = isNbinom;
    ret[1] = x;
    ret[2] = size;
    ret[3] = prob;
    ret[4] = NA_REAL;
    ret[5] = NA_REAL;
    return;
  }
  if (!llikNeedDeriv()) {
    ret[0] = isNbinom;
    ret[1] = x;
    ret[2] = size;
    ret[3] = prob;
    ret[4] = stan::math::neg_binomial_2_lpmf(x, (double)(size)*(1.0-prob)/prob, size);;
    ret[5] = NA_REAL;
  } else {
    Eigen::VectorXi y(1);
    Eigen::VectorXi N(1);
    Eigen::VectorXd params(1);
    y(0) = (int)(x);
    N(0) = (int)(size);
    params(0) = prob;
    stanLl ll = llik_nbinom(y, N, params);
    ret[0] = isNbinom;
    ret[1] = x;
    ret[2] = size;
    ret[3] = prob;
    ret[4] = ll.fx(0);
    ret[5] = ll.J(0, 0);
  }
  return;
}

//[[Rcpp::export]]
Rcpp::DataFrame llikNbinomInternal(Rcpp::NumericVector x, Rcpp::NumericVector size, Rcpp::NumericVector prob) {
  llikNeedDeriv_=1;
  NumericVector fx(x.size());
  NumericVector dProb(x.size());
  double cur[6];
  for (int j = x.size(); j--;) {
    llikNbinomFull(cur, x[j], size[j], prob[j]);
    fx[j]      = cur[4];
    dProb[j]     = cur[5];
  }
  return Rcpp::DataFrame::create(_["fx"]=fx,
                                 _["dProb"]=dProb);
}

extern "C" double rxLlikNbinom(double* ret, double x, double size, double prob) {
  llikNbinomFull(ret, x, size, prob);
  return ret[4];
}

extern "C" double rxLlikNbinomDprob(double* ret, double x, double size, double prob) {
  llikNbinomFull(ret, x, size, prob);
  return ret[5];
}

