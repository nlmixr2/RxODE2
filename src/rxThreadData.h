#ifndef __RXTHREADDATA_H__
#define __RXTHREADDATA_H__

#if defined(__cplusplus)
extern "C" {
#endif

  void setIndPointersByThread(rx_solving_options_ind *ind);
  double *getAlagThread();
  double *getFThread();
  double *getRateThread();
  double *getDurThread();
  double *getInfusionRateThread();
#if defined(__cplusplus)
}
#endif

#endif // __RXTHREADDATA_H__
