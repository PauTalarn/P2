#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "pav_analysis.h"
#include "vad.h"

const float FRAME_TIME = 20.0F; /* in ms. */
int cont=0;

/*
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
    "S",
    "V",
    "INIT",
    "MS",
    "MV",
};

const char *state2str(VAD_STATE st)
{
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct
{
  float zcr;
  float p;
  float am;
} Features;

/*
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N)
{

  Features feat;

  feat.p = compute_power(x, N);
  feat.am = compute_am(x, N);
  feat.zcr = compute_zcr(x, N, 16000);
  return feat;
}

/*
 * TODO: Init the values of vad_data
 Inicialitza la trama de dades creant un objecte vad_data i assignant-li una sèrie de
 característiques. Ho fa, a partir de la frqüència de mostreig i d'un cert
 umbral per decidir si és trama de so o no.
 */

VAD_DATA *vad_open(float rate, float alfa0, float alfa1, float num_init, float num_MV, float num_MS)
{
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));     // Assigna memòria per l'estructura VAS_DATA
  vad_data->state = ST_INIT;                         // Estableix que estem a l'estat inicial
  vad_data->sampling_rate = rate;                    // Estableix quina és la freüència de mostreig
  vad_data->frame_length = rate * FRAME_TIME * 1e-3; // Càlcul de la longitud de la trama a partir d'una longitud
                                                  
  vad_data->alfa0 = alfa0;                          

  vad_data->alfa1 = alfa1;
  vad_data->num_init = num_init;
  vad_data->num_MV=num_MV;
  vad_data->num_MS=num_MS;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data)
{
  VAD_STATE state = vad_data->state;
  if(state == MY_SILENCE) state = ST_VOICE;
  if(state == MY_VOICE) state = ST_SILENCE;
 

  free(vad_data); 
  return state;
}

// Obtenir la mida de trama de la senyal
unsigned int vad_frame_size(VAD_DATA *vad_data)
{
  return vad_data->frame_length;
}



VAD_STATE vad(VAD_DATA *vad_data, float *x, unsigned int t)
{
  // f.p es la potencia de la trama

  Features f = compute_features(x, vad_data->frame_length);
                           
                                                        
  vad_data->last_feature = f.p;                            
                                                           

  switch (vad_data->state)
  {
  case ST_INIT: // Es la primera trama
    if (t < vad_data->num_init)
    {
       
      vad_data->pot_total += pow(10, f.p / 10); // Nou càlcul de la potència
      vad_data->zcr += f.zcr;
    }
    else
    {
      vad_data->P0 = 10 * log10(vad_data->pot_total / t) + vad_data->alfa0;
      vad_data->P1 = 10 * log10(vad_data->pot_total / t) + vad_data->alfa1;
      vad_data->zcr = vad_data->zcr / t; // A partir del num de creuaments per zero busquem la tassa
      vad_data->state = ST_SILENCE;
    }
    break;
  case ST_SILENCE:

    vad_data->indef = 0;
    if (f.p > vad_data->P0)
    { // Marcamos un umbral de ruido que no lo detecte
      vad_data->state = MY_VOICE;
      vad_data->indef++;
    }
    break;

  case ST_VOICE:

    vad_data->indef = 0;
   
    if (f.p < vad_data->P1 && vad_data->zcr > f.zcr-30)
    {
      vad_data->indef++;
      vad_data->state = MY_SILENCE;
      
    }
    break;

  case MY_SILENCE:

  if (f.p < vad_data->P0 && vad_data->zcr > f.zcr-30){

      if(vad_data->indef*vad_data->frame_length/vad_data->sampling_rate < vad_data->num_MS){
        vad_data->indef++;

      }
      else{
      vad_data->state = ST_SILENCE;
       vad_data->indef= 0;

      }
    }else if (f.p > vad_data->P0){
      vad_data->indef= 0;
      vad_data->state = ST_VOICE;
 
    }

    break;

  case MY_VOICE:

    if (f.p > vad_data->P1){
   
      if(vad_data->indef*vad_data->frame_length/vad_data->sampling_rate < vad_data->num_MV){
        vad_data->indef++;
    }else{
      vad_data->state = ST_VOICE;
      vad_data->indef= 0;
      }
    } else if (f.p < vad_data->P1){
      vad_data->indef = 0;
      vad_data->state = ST_SILENCE;
    }

    break;
  }

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE ||
      vad_data->state == MY_SILENCE ||
      vad_data->state == MY_VOICE)
  {
    return vad_data->state;
  }
  else
  {
    return ST_SILENCE;
  }
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out)
{
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}


//scripts/run_vad.sh 10 3 5 0.08 0.02--> Sense forçar umbral
//bin/vad -i pav2152.wav -o pav2152.vad -0 19 -1 9 -n 7 -j 0.08 -f 0.02
//Millor 3 3 5 --> Forçant l'umbral