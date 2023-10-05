#define USE_FC_LEN_T
#define STRICT_R_HEADERS
// [[Rcpp::interfaces(r,cpp)]]
//#undef NDEBUG
#include <RcppArmadillo.h>
#include "../inst/include/rxode2.h"
#include <R.h>
#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) dgettext ("rxode2", String)
/* replace pkg as appropriate */
#else
#define _(String) (String)
#endif
using namespace Rcpp;
using namespace arma;

extern "C" SEXP _rxode2_isNullZero(SEXP);

//[[Rcpp::export]]
LogicalVector isNullZero(RObject obj) {
  if (Rf_isNull(obj)) {
    return true;
  }
  int t = TYPEOF(obj);
  if (t == INTSXP || t == REALSXP) {
    // This tests for thetaMat
    if (obj.hasAttribute("dim")) {
      mat cur = as<arma::mat>(obj);
      if (cur.is_zero()) {
	return true;
      }
    }
  } else if (t == VECSXP) {
    List cur = as<List>(obj);
    for (int i = cur.size(); i--;) {
      RObject curs = cur[i];
      t = TYPEOF(wrap(curs));
      if (t == INTSXP || t == REALSXP) {
	if (curs.hasAttribute("dim")) {
	  mat curm = as<arma::mat>(curs);
	  if (curm.is_zero()) {
	    return true;
	  }
	} else {
	  return false;
	}
      } else {
	return false;
      }
    }
  }
  return false;
}

extern Function loadNamespace;
bool rxode2et_loaded = false;
Environment rxode2et;

extern "C" SEXP _rxode2_et_(SEXP x1, SEXP x2) {
  if (!rxode2et_loaded) {
    rxode2et_loaded = true;
    rxode2et = loadNamespace("rxode2et");
  }
  Function et_ = as<Function>(rxode2et[".et_"]);
  return et_(x1, x2);
}

extern "C" SEXP _rxode2_etUpdate(SEXP x1, SEXP x2, SEXP x3, SEXP x4) {
  if (!rxode2et_loaded) {
    rxode2et_loaded = true;
    rxode2et = loadNamespace("rxode2et");
  }
  Function etUp = as<Function>(rxode2et[".etUpdate"]);
  return etUp(x1, x2, x3, x4);
}

extern "C" SEXP _rxode2_etSeq_(SEXP x1, SEXP x2, SEXP x3, SEXP x4, SEXP x5,
                               SEXP x6, SEXP x7, SEXP x8, SEXP x9, SEXP x10,
                               SEXP x11) {
  if (!rxode2et_loaded) {
    rxode2et_loaded = true;
    rxode2et = loadNamespace("rxode2et");
  }
  Function etSeq = as<Function>(rxode2et[".etSeq"]);
  return etSeq(x1, x2, x3, x4, x5,
               x6, x7, x8, x9, x10,
               x11);
}

extern "C" SEXP _rxode2_etRep_(SEXP x1, SEXP x2, SEXP x3, SEXP x4, SEXP x5,
                               SEXP x6, SEXP x7) {
  if (!rxode2et_loaded) {
    rxode2et_loaded = true;
    rxode2et = loadNamespace("rxode2et");
  }
  Function etRep = as<Function>(rxode2et[".etRep"]);
  return etRep(x1, x2, x3, x4, x5,
               x6, x7);
}

extern "C" SEXP _rxode2_setEvCur(SEXP x1) {
  if (!rxode2et_loaded) {
    rxode2et_loaded = true;
    rxode2et = loadNamespace("rxode2et");
  }
  Function setEvCur = as<Function>(rxode2et[".setEvCur"]);
  return setEvCur(x1);
}

List cbindThetaOmega(RObject inputParametersRO, List &individualParameters) {
  if (!rxode2et_loaded) {
    rxode2et_loaded = true;
    rxode2et = loadNamespace("rxode2et");
  }
  Function f = as<Function>(rxode2et[".cbindThetaOmega"]);
  List ret = as<List>(f(wrap(inputParametersRO), wrap(individualParameters)));
  individualParameters=ret[1];
  return ret[0];
}

//[[Rcpp::export]]
NumericVector rxErf(NumericVector v) {
  NumericVector ret(v.size());
  for (int i = v.size(); i--;) {
    ret[i] = std::erf(v[i]);
  }
  return ret;
}

#define min2( a , b )  ( (a) < (b) ? (a) : (b) )
#define max2( a , b )  ( (a) > (b) ? (a) : (b) )

//[[Rcpp::export]]
NumericVector binomProbs_(NumericVector x, NumericVector probs, bool naRm, int nIn,
                          int cont) {
  double oldM = 0.0, newM = 0.0;
  int n = 0;
  for (int i = 0; i <  x.size(); ++i) {
    double cur = x[i];
    if (ISNA(cur)) {
      if (naRm) {
        continue;
      } else  {
        NumericVector ret(4+probs.size());
        for (int j = 0; j < ret.size(); ++j) {
          ret[j] = NA_REAL;
        }
        return ret;
      }
    }
    n++;
    if (n == 1) {
      oldM = newM = cur;
    } else {
      newM = oldM + (cur - oldM)/n;
      oldM = newM;
    }
  }
  int nC;
  if (nIn == 0) {
    nC = n;
  } else {
    nC = nIn;
  }
  double var=0;
  double sd=0;
  if (n > 1) {
    var = oldM*(1.0-oldM);
    sd = sqrt(var);
  } else {
    var = 0.0;
    sd = 0.0;
  }
  // mean, var, sd
  NumericVector ret(4+probs.size());
  ret[0] = oldM;
  ret[1] = var;
  ret[2] = sd;
  ret[3] = (double)n;

  for (int i = 0; i < probs.size(); ++i) {
    double p = probs[i];
    std::string str = std::to_string(p*100) + "%";
    if (p == 0) {
      ret[i+4] = 0;
    } else if (p == 1) {
      ret[i+4] = 1;
    } else if (p == 0.5) {
      ret[i+4] = oldM;
    } else {
      double z;
      if (p > 0.5) {
        z = Rf_qnorm5(p, 0.0, 1.0, 1, 0);
      } else {
        z = Rf_qnorm5(1.0-p, 0.0, 1.0, 1, 0);
      }
      double z2 = z*z;
      double c1, c2, c3;
#define contWilsonContinuityCorrection 0
#define contWilson 1
#define contWald 2
#define contAgrestiCoull 3
      switch (cont) {
      case contWilsonContinuityCorrection:
        c1 = 2*nC*oldM + z2;
        c2 = z*sqrt(z2 - 1.0/nC + 4*nC*var + (4*oldM-2))+1.0;
        c3 = 2*(nC+z2);
        if (p < 0.5) {
          if (p == 0.0) {
            ret[i+4] = 0.0;
          } else {
            ret[i+4] = max2(0, (c1-c2)/c3);
          }
        } else {
          if (p == 1.0) {
            ret[i+4] = 1.0;
          } else {
            ret[i+4] = min2(1, (c1+c2)/c3);
          }
        }
        break;
      case contWilson:
        c1 = 1.0/(1.0+z2/nC)*(oldM+z2/(2.0*nC));
        c2 = z/(1.0+z2/nC)*sqrt(oldM*(1.0-oldM)/nC + z2/(4.0*nC*nC));
        if (p < 0.5) {
          ret[i+4] = c1-c2;
        } else {
          ret[i+4] = c1+c2;
        }
        break;
      case contWald:
        c1 = oldM;
        c2 = z*sqrt(oldM*(1-oldM)/nC);
        if (p < 0.5) {
          ret[i+4] = c1-c2;
        } else {
          ret[i+4] = c1+c2;
        }
        break;
      case contAgrestiCoull:
        // p = ns/n; p*n = ns
        c1 = oldM*nC;
        nC = nC + z2;
        c1 = 1.0/nC*(c1+z2/2.0);
        c2 = z*sqrt(c1/nC*(1-c1));
        if (p < 0.5) {
          ret[i+4] = c1-c2;
        } else {
          ret[i+4] = c1+c2;
        }
        break;
      }
    }
  }
  return ret;
}

//[[Rcpp::export]]
NumericVector meanProbs_(NumericVector x, NumericVector probs, bool naRm, bool useT,
                         bool pred, int nIn) {
  double oldM = 0.0, newM = 0.0,
    oldS = 0.0, newS = 0.0, mx=R_NegInf, mn=R_PosInf;
  int n = 0;
  for (int i = 0; i <  x.size(); ++i) {
    double cur = x[i];
    if (ISNA(cur)) {
      if (naRm) {
        continue;
      } else  {
        NumericVector ret(6+probs.size());
        for (int j = 0; j < ret.size(); ++j) {
          ret[j] = NA_REAL;
        }
        return ret;
      }
    }
    n++;
    mn = min2(cur, mn);
    mx = max2(cur, mx);
    if (n == 1) {
      oldM = newM = cur;
      oldS = 0.0;
    } else {
      newM = oldM + (cur - oldM)/n;
      newS = oldS + (cur - oldM)*(cur - newM);
      oldM = newM;
      oldS = newS;
    }
  }
  double var;
  if (n > 1) {
    var = newS/(n-1);
  } else {
    var = 0.0;
  }
  double sd = sqrt(var);
  // mean, var, sd, min, max
  NumericVector ret(6+probs.size());
  ret[0] = oldM;
  ret[1] = var;
  ret[2] = sd;
  ret[3] = mn;
  ret[4] = mx;
  ret[5] = (double)n;
  double c;
  int nC;
  if (nIn == 0) {
    nC = n;
  } else {
    nC = nIn;
  }
  if (pred) {
    c = sd*sqrt(1.0+(1.0/nC));
  } else {
    c = sd/sqrt((double)(nC));
  }
  for (int i = 0; i < probs.size(); ++i) {
    double p = probs[i];
    std::string str = std::to_string(p*100) + "%";
    if (p == 0) {
      ret[i+6] = mn;
    } else if (p == 1) {
      ret[i+6] = mx;
    } else if (p == 0.5) {
      ret[i+6] = oldM;
    } else if (useT) {
      ret[i+6] = oldM + c * Rf_qt(p, (double)(nC-1), 1, 0);
    } else {
      ret[i+6] = oldM + c * Rf_qnorm5(p, 0.0, 1.0, 1, 0);
    }
  }
  return ret;
}
