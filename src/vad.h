#ifndef _VAD_H
#define _VAD_H
#include <stdio.h>

/* TODO: add the needed states */
typedef enum { ST_SILENCE, ST_VOICE, ST_INIT, MY_SILENCE, MY_VOICE} VAD_STATE;

/* Return a string label associated to each state */
const char *state2str(VAD_STATE st);

/* TODO: add the variables needed to control the VAD 
   (counts, thresholds, etc.) */

//Establim totes les variables necessàries per crear un objecte VAD_DATA
typedef struct {
  VAD_STATE state; //Estat de la trama
  VAD_STATE last_state;
  float sampling_rate; //Freqüència de mostreig de la trama
  unsigned int frame_length; //FNúmer de mostres de la trama
  float last_feature; /* for debuggin purposes */
  float alfa0; //Umbral de decisió de la trama
  float alfa1;
  float P0; //Potència umbral de la trama
  float P1;
  int indef;
  int num_init;
  float num_MV;
  float num_MS;
  float pot_total;
  float zcr;
  
  //Umbral K1 = K0 + alpha0
  // K0 = P0 + alpha1
} VAD_DATA;

/* Call this function before using VAD: 
   It should return allocated and initialized values of vad_data

   sampling_rate: ... the sampling rate */
VAD_DATA *vad_open(float sampling_rate, float alfa0, float alfa1, float num_init, float num_MV, float num_MS);

/* vad works frame by frame.
   This function returns the frame size so that the program knows how
   many samples have to be provided */
unsigned int vad_frame_size(VAD_DATA *);

/* Main function. For each 'time', compute the new state 
   It returns:
    ST_UNDEF   (0) : undefined; it needs more frames to take decission
    ST_SILENCE (1) : silence
    ST_VOICE   (2) : voice

    x: input frame
       It is assumed the length is frame_length */
VAD_STATE vad(VAD_DATA *vad_data, float *x, unsigned int t);

/* Free memory
   Returns the state of the last (undecided) states. */
VAD_STATE vad_close(VAD_DATA *vad_data);

/* Print actual state of vad, for debug purposes */
void vad_show_state(const VAD_DATA *, FILE *);

#endif
