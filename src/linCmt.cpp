#ifndef NDEBUG
#define NDEBUG // just in case
#endif
#define USE_FC_LEN_T
#define STRICT_R_HEADERS
#include "linCmt.h"

// Create linear compartment models for testing

using namespace Rcpp;

// [[Rcpp::export]]
RObject linCmtModelDouble(double dt,
                          double p1, double v1, double p2,
                          double p3, double p4, double p5,
                          double ka,
                          NumericVector alastNV, NumericVector rateNV,
                          const int ncmt, const int oral0, const int trans,
                          bool deriv) {

  stan::math::linCmtStan lc(ncmt, oral0, trans, deriv);
  Eigen::Matrix<double, -1, 1> theta;
  Eigen::Matrix<double, -1, 1> alast0 = as<Eigen::Matrix<double, -1, 1> >(alastNV);
  Eigen::Matrix<double, -1, 1> rate = as<Eigen::Matrix<double, -1, 1> >(rateNV);
  int nAlast = lc.getNalast();

  if (alast0.size() != nAlast) {
    Rcpp::stop("Alast0 size needs to be %d", nAlast);
  }
  theta.resize(lc.getNpars());

  switch (ncmt) {
  case 1:
    if (oral0 == 1) {
      theta << p1, v1, ka;
    } else {
      theta << p1, v1;
    }
    break;
  case 2:
    if (oral0 == 1) {
      theta << p1, v1, p2, p3, ka;
    } else {
      theta << p1, v1, p2, p3;
    }
    break;
  case 3:
    if (oral0 == 1) {
      theta << p1, v1, p2, p3, p4, p5, ka;
    } else {
      theta << p1, v1, p2, p3, p4, p5;
    }
    break;
  default:
    stop("Invalid number of compartments");
  }
  double *a = new double[nAlast];
  double *asave = new double[nAlast];
  double *r = new double[lc.getNrate()];
  lc.setPtr(a, r, asave);
  lc.setAlast(alast0, nAlast);
  lc.setRate(rate.data());
  lc.setDt(dt);
  List retList;
  if (deriv) {
    Eigen::Matrix<double, Eigen::Dynamic, 1> fx;
    Eigen::Matrix<double, -1, -1> J;
    stan::math::jacobian(lc, theta, fx, J);
    lc.saveJac(J);
    Eigen::Matrix<double, -1, 1> Jg = lc.getJacCp(J, fx, theta);
    double val = lc.adjustF(fx, theta);
    NumericVector Alast(nAlast);
    for (int i = 0; i < nAlast; i++) {
      Alast[i] = asave[i];
    }
    retList = List::create(_["val"] = wrap(val),
                           _["J"] = wrap(J),
                           _["Jg"] = wrap(Jg),
                           _["Alast"] = Alast);
  } else {
    Eigen::Matrix<double, Eigen::Dynamic, 1> fx;
    fx = lc(theta);
    double val = lc.adjustF(fx, theta);
    NumericVector Alast(nAlast);
    for (int i = 0; i < nAlast; i++) {
      Alast[i] = asave[i];
    }
    retList = List::create(_["val"] = wrap(val),
                           _["Alast"] = Alast);
  }
  delete[] a;
  delete[] r;
  delete[] asave;
  return retList;
}


extern "C" double linCmtA(rx_solve *rx, int id,
                          int trans, int ncmt, int oral0,
                          int which,
                          double _t,
                          double p1, double v1,
                          double p2, double p3,
                          double p4, double p5,
                          // Oral parameters
                          double ka) {
  rx_solving_options_ind *ind = &(rx->subjects[id]);
  rx_solving_options *op = rx->op;
  int idx = ind->idx;
  double t = _t - ind->curShift;
  if (ind->linCmtDt != t) {
    Eigen::Matrix<double, -1, 1> theta;

    stan::math::linCmtStan lc(ncmt, oral0, trans, false);
    int nAlast = lc.getNalast();
    int nPars =  lc.getNpars();
    theta.resize(nPars);
    switch (ncmt) {
    case 1:
      if (oral0 == 1) {
        theta << p1, v1, ka;
      } else {
        theta << p1, v1;
      }
      break;
    case 2:
      if (oral0 == 1) {
        theta << p1, v1, p2, p3, ka;
      } else {
        theta << p1, v1, p2, p3;
      }
      break;
    case 3:
      if (oral0 == 1) {
        theta << p1, v1, p2, p3, p4, p5, ka;
      } else {
        theta << p1, v1, p2, p3, p4, p5;
      }
      break;
    default:
      return NA_REAL;
    }
    double *aLastPtr = getAdvan(idx);
    lc.setPtr(aLastPtr, ind->linCmtRate, ind->linCmtSave);
    Eigen::Matrix<double, Eigen::Dynamic, 1> Alast(nAlast);
    std::copy(aLastPtr, aLastPtr + nAlast, Alast.data());
    lc.setAlast(Alast, nAlast);
    lc.setRate(ind->linCmtRate);
    lc.setDt(_t - ind->curShift);
    Eigen::Matrix<double, Eigen::Dynamic, 1> fx;
    fx = lc(theta);
    ind->linCmtSave[0] = lc.adjustF(fx, theta);
    ind->linCmtDt = _t;
  }
  if (which < 0) {
    double ret = ind->linCmtSave[oral0];
    if (trans != 10 || ncmt == 1) {
      ret = ret / v1;
    } else if (ncmt == 2) {
      ret = ret / (v1 + p3);
    } else if (ncmt == 3) {
      ret = ret / (v1 + p3 + p5);
    }
    return ret;
  } else {
    return ind->linCmtSave[which];
  }
}


extern "C" double linCmtB(rx_solve *rx, int id,
                          int trans, int ncmt, int oral0,
                          int which1, int which2,
                          double _t,
                          double p1, double v1,
                          double p2, double p3,
                          double p4, double p5,
                          // Oral parameters
                          double ka) {
  rx_solving_options_ind *ind = &(rx->subjects[id]);
  rx_solving_options *op = rx->op;
  int idx = ind->idx;
  double t = _t - ind->curShift;
  Eigen::Matrix<double, Eigen::Dynamic, 1> fx;
  Eigen::Matrix<double, -1, -1> J;
  Eigen::Matrix<double, -1, 1> theta;
  stan::math::linCmtStan lc(ncmt, oral0, trans, true);
  int nAlast = lc.getNalast();
  int nPars =  lc.getNpars();
  theta.resize(nPars);
  switch (ncmt) {
  case 1:
    if (oral0 == 1) {
      theta << p1, v1, ka;
    } else {
      theta << p1, v1;
    }
    break;
  case 2:
    if (oral0 == 1) {
      theta << p1, v1, p2, p3, ka;
    } else {
      theta << p1, v1, p2, p3;
    }
    break;
  case 3:
    if (oral0 == 1) {
      theta << p1, v1, p2, p3, p4, p5, ka;
    } else {
      theta << p1, v1, p2, p3, p4, p5;
    }
    break;
  default:
    return NA_REAL;
  }

  double *aLastPtr = getAdvan(idx);
  lc.setPtr(aLastPtr, ind->linCmtRate, ind->linCmtSave);
  Eigen::Matrix<double, Eigen::Dynamic, 1> Alast(nAlast);
  std::copy(aLastPtr, aLastPtr + nAlast, Alast.data());
  lc.setAlast(Alast, nAlast);
  lc.setRate(ind->linCmtRate);
  lc.setDt(_t - ind->curShift);
  if (ind->linCmtDt != _t) {
    stan::math::jacobian(lc, theta, fx, J);
    lc.saveJac(J);
    // Eigen::Matrix<double, -1, 1> Jg = lc.getJacCp(J, fx, theta);
    ind->linCmtDt = _t;
  } else {
    fx = lc.restoreFx(aLastPtr);
    J = lc.restoreJac(aLastPtr);
  }
  if (which1 >= 0 && which2 >= 0) {
    // w1, w2 are > 0
    return J(which1, which2);
  } else if (which1 == -1 && which2 == -1) {
    // -1, -1 is the function value
    double ret = fx(oral0);
    if (trans != 10 || ncmt == 1) {
      ret = ret / v1;
    } else if (ncmt == 2) {
      ret = ret / (v1 + p3);
    } else if (ncmt == 3) {
      ret = ret / (v1 + p3 + p5);
    }
    return ret;
  } else if (which1 >= 0 && which2 == -2) {
    // w2 < 0
    return fx(which1);
  } else if (which1 == -2 && which2 >= 0) {
    Eigen::Matrix<double, -1, 1> Jg = lc.getJacCp(J, fx, theta);
    return Jg(which2);
  }
  return NA_REAL;
}
