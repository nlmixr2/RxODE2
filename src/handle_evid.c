#ifndef R_NO_REMAP
#define R_NO_REMAP
#endif
#define USE_FC_LEN_T
#define STRICT_R_HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "strncmp.h"
#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) dgettext ("rxode2", String)
/* replace pkg as appropriate */
#else
#define _(String) (String)
#endif
#include "../inst/include/rxode2.h"
#include <rxode2parseHandleEvid.h>


int handle_evidL(int evid, double *yp, double xout, int id, rx_solving_options_ind *ind) {
  if (ind->inLhs) {
    // In this case dosing to the extra compartments is OK so add it
    rx_solving_options *op = &op_global;
    return handle_evid(evid, op->neq + op->extraCmt, ind->BadDose,
		       ind->InfusionRate, ind->dose, yp,
		       xout, id, ind);

  } else {
    return isDose(evid);
  }
}

void handleTlast(double *time, rx_solving_options_ind *ind) {
  handleTlastInline(time, ind);
}

// Linear compartment models/functions
double _getDur(int l, rx_solving_options_ind *ind, int backward, unsigned int *p) {
  double dose = getDoseNumber(ind, l);
  if (backward==1 && l != 0){
    if (l <= 0) {
      Rf_errorcall(R_NilValue, _("could not find a start to the infusion #1"));
    }
    p[0] = l-1;
    while (p[0] > 0 && getDoseNumber(ind, p[0]) != -dose){
      p[0]--;
    }
    if (getDoseNumber(ind, p[0]) != -dose){
      Rf_errorcall(R_NilValue, _("could not find a start to the infusion #2"));
    }
    return getAllTimes(ind, ind->idose[l]) - getAllTimes(ind, ind->idose[p[0]]);
  } else {
    if (l >= ind->ndoses) {
      if (backward==2) return(NA_REAL);
      Rf_errorcall(R_NilValue, _("could not find an end to the infusion"));
    }
    p[0] = l+1;
    while (p[0] < ind->ndoses && getDoseNumber(ind, p[0]) != -dose){
      p[0]++;
    }
    if (getDoseNumber(ind, p[0]) != -dose){
      if (backward==2) return(NA_REAL);
      Rf_errorcall(R_NilValue, _("could not find an end to the infusion"));
    }
    return getAllTimes(ind, ind->idose[p[0]]) - getAllTimes(ind, ind->idose[l]);
  }
}
