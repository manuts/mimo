#ifndef FRAMING_H
#define FRAMING_H

#include <vector>
#include <fftw3.h>
#include <gnuradio/gr_complex.h>
#include <complex>
#include <liquid/liquid.h>

// callback
typedef void * (* mimo_callback) (void *);

  // receiver state
typedef enum {
  STATE_SEEK_PLATEAU = 0,
  STATE_SAVE_ACCESS_CODES,
  STATE_WAIT,
  STATE_MIMO
} framesync_states_t;

class framegen {
 private:
  // number of subcarriers
  unsigned int M;
  // cyclic prefix length
  unsigned int cp_len;
  // symbol length
  unsigned int symbol_len;
  // number of data streams
  unsigned int num_streams;
  // number of access codes
  unsigned int num_access_codes;
  // subcarrier allocation
  unsigned char * p;
  // number of null subcarriers
  unsigned int M_null;
  // number of pilot subcarriers
  unsigned int M_pilot;
  // number of data subcarriers
  unsigned int M_data;
  // frequency domain buffer
  std::vector<gr_complex *> X;
  // time domain buffer
  std::vector<gr_complex *> x;
  // scaling factors
  float g_data;
  // transmit beamformer
  gr_complex *** W;
  // transform object
  std::vector<fftwf_plan> ifft;
  // PLCP short
  gr_complex * S0;
  gr_complex * s0;
  // PLCP long
  std::vector<gr_complex *> S1;
  std::vector<gr_complex *> s1;

  // volk buffer for volk operations
  std::complex<float> * volk_buff_fc1;
  std::complex<float> * volk_buff_fc2;
  std::complex<float> * volk_buff_fc3;
  float * volk_buff_f1;

 public:
  // constructor
  framegen(unsigned int _M,
           unsigned int _cp_len,
           unsigned int _num_streams,
           unsigned int _num_access_codes,
           unsigned char * const &_p,
           msequence const &_ms_S0,
           std::vector<msequence> const &_ms_S1);
  // destructor
  ~framegen();
  void print();

  unsigned int
  write_sync_words(std::vector<std::complex<float> *> tx_buff);

  // set the precoding vector.
  void set_W(gr_complex *** const _W);
  gr_complex *** get_W();
  void compute_W(gr_complex *** G);
  unsigned int
  write_mimo_packet(std::vector<gr_complex *> tx_buff);
  unsigned int get_num_streams();
};

class framesync {
 private:
  // number of subcarriers
  unsigned int M;
  // cyclic prefix length
  unsigned int cp_len;
  // symbol length
  unsigned int symbol_len;
  // number of data streams
  unsigned int num_streams;
  // number of access codes
  unsigned int num_access_codes;
  // subcarrier allocation
  unsigned char * p;
  // number of null subcarriers
  unsigned int M_null;
  // number of pilot subcarriers
  unsigned int M_pilot;
  // number of data subcarriers
  unsigned int M_data;
  // frequency domain buffer
  std::vector<gr_complex *> X;
  // time domain buffer
  std::vector<gr_complex *> x;
  // CSI
  gr_complex *** G;
  // transform object
  std::vector<fftwf_plan> fft;
  // PLCP short
  gr_complex * S0;
  gr_complex * s0;
  // PLCP long
  std::vector<gr_complex *> S1;
  std::vector<gr_complex *> s1;

  // volk buffer for volk operations
  std::complex<float> * volk_buff_fc1;
  std::complex<float> * volk_buff_fc2;
  std::complex<float> * volk_buff_fc3;
  float * volk_buff_f1;

  void increment_state();

  // sync 
  unsigned long int sync_index;
  unsigned long long int num_samples_processed;
  void execute_sc_sync(gr_complex _x[]);
  void execute_save_access_codes(gr_complex _x[]);
  framesync_states_t state;
  void execute_mimo_decode(gr_complex _x[]);
  unsigned long int plateau_start;
  unsigned long int plateau_end;
 public:
  // constructor
  framesync(unsigned int _M,
            unsigned int _cp_len,
            unsigned int _num_streams,
            unsigned int _num_access_codes,
            unsigned char * const &_p,
            msequence const &_ms_S0,
            std::vector<msequence> const &_ms_S1,
	    mimo_callback _callback);
  // destructor
  ~framesync();
  void print();
  unsigned long int get_sync_index();
  void get_G(gr_complex *** _G);
  unsigned long long int get_num_samples_processed();
  framesync_states_t execute(std::vector<gr_complex *> const &in_buff,
			     unsigned int num_samples);
  unsigned long int get_plateau_start();
  unsigned long int get_plateau_end();
  void reset();
  void estimate_channel();
};

// initialize default subcarrier allocation
//  _M      :   number of subcarriers
//  _p      :   output subcarrier allocation vector, [size: _M x 1]
//
// key: '.' (null), 'P' (pilot), '+' (data)
// .+++P+++++++P.........P+++++++P+++
//
void ofdmframe_init_default_sctype(unsigned char *_p, unsigned int _M);

// validate subcarrier type (count number of null, pilot, and data
// subcarriers in the allocation)
//  _p          :   subcarrier allocation array, [size: _M x 1]
//  _M_null     :   output number of null subcarriers
//  _M_pilot    :   output number of pilot subcarriers
//  _M_data     :   output number of data subcarriers
void ofdmframe_validate_sctype(const unsigned char * _p,
                               unsigned int _M,
                               unsigned int * _M_null,
                               unsigned int * _M_pilot,
                               unsigned int * _M_data);

// print subcarrier allocation to screen
//
// key: '.' (null), 'P' (pilot), '+' (data)
// .+++P+++++++P.........P+++++++P+++
//
void ofdmframe_print_sctype(const unsigned char * _p, unsigned int _M);

// generate short sequence symbols
//  _p      :   subcarrier allocation array
//  _S0     :   output symbol (freq)
//  _s0     :   output symbol (time)
//  _M_S0   :   total number of enabled subcarriers in S0
//  ms      :   pn_sequence generator to generate S0
void ofdmframe_init_S0(const unsigned char * _p,
                       unsigned int _M,
                       std::complex<float> * _S0,
                       std::complex<float> * _s0,
                       msequence ms);

// generate long sequence symbols
//  _p      :   subcarrier allocation array
//  _S1     :   output symbol (freq)
//  _s1     :   output symbol (time)
//  _M_S1   :   total number of enabled subcarriers in S1
//  ms      :   pn_sequence generator to generate S1
void ofdmframe_init_S1(const unsigned char * _p,
                       unsigned int _M,
                       unsigned int _num_access_codes,
                       std::complex<float> * _S1,
                       std::complex<float> * _s1,
                       msequence ms);

gr_complex liquid_cexpjf(float theta);
float cabsf(gr_complex z);
float cargf(gr_complex z);
float fabsf(float x);
gr_complex conjf(gr_complex z);

#endif // FRAMING_H
