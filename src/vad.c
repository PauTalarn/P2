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
  /*
   * Input: x[i] : i=0 .... N-1
   * Ouput: computed features
   */
  /*
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1
   */
  Features feat; // El tipo de datos de la variable features determina qué valores se pueden almacenar en la variable.
                 // Por ejemplo, si features se declara como una variable de tipo int,
                 // entonces solo se pueden almacenar valores enteros en la variable features.

  // Gener numeros aleatorios
  // feat.zcr = feat.p = feat.am = (float) rand()/RAND_MAX;
  feat.p = compute_power(x, N);
  //printf("feat.p=%f \n",feat.p);
  feat.zcr = compute_zcr(x, N, 16000);
  return feat;
}

/*
 * TODO: Init the values of vad_data
 Inicialitza la trama de dades creant un objecte vad_data i assignant-li una sèrie de
 característiques. Ho fa, a partir de la frqüència de mostreig i d'un cert
 umbral per decidir si és trama de so o no.
 */

VAD_DATA *vad_open(float rate, float alfa0, float alfa1, float num_init, int num_MV, int num_MS)
{
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));     // Assigna memòria per l'estructura VAS_DATA
  vad_data->state = ST_INIT;                         // Estableix que estem a l'estat inicial
  vad_data->sampling_rate = rate;                    // Estableix quina és la freüència de mostreig
  vad_data->frame_length = rate * FRAME_TIME * 1e-3; // Càlcul de la longitud de la trama a partir d'una longitud
                                                     // igual per a totes les trames (predifinida a dalt)
  vad_data->alfa0 = alfa0;                           // estableix el valor umbral de la senyal
  //printf("%d\n", vad_data->frame_length);
  vad_data->alfa1 = alfa1;
  vad_data->num_init = num_init;
  vad_data->num_MV=num_MV;
  vad_data->num_MS=num_MS;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data)
{
  /*
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->last_state; // Extraiem l'estat final de l'última trama
  printf("cont= %d\n", cont);

  free(vad_data); // la memoria asignada para la estructura VAD_DATA a
                  // través de malloc será liberada y se puede reutilizar para otros fines en el programa
  return state;
}

// Obtenir la mida de trama de la senyal
unsigned int vad_frame_size(VAD_DATA *vad_data)
{
  return vad_data->frame_length;
}

/*
 * TODO: Implement the Voice Activity Detection
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x, unsigned int t)
{

  /*
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  // f.p es la potencia de la trama

  Features f = compute_features(x, vad_data->frame_length); // Creem una variable de pas anomenada f en la qual a la
  //printf("f.p=%f \n",f.p);                                                          // seva funció ho calculem la potència a partir d'una
                                                            // senyal i el seu num de mostres
  vad_data->last_feature = f.p;                             /* save feature, in case you want to show */
                                                            // Ens guardem la potència en una variable per fer-la disponible en el programa principal
  // Es una variable de pas abans de decidir en quin estat es troba

  switch (vad_data->state)
  {
  case ST_INIT: // Es la primera trama
    //printf("t=%d \n", t);
    //printf("num_init=%d \n", vad_data->num_init);
    if (t < vad_data->num_init)
    {
      // printf("Hola");
       
      vad_data->pot_total += pow(10, f.p / 10); // Nou càlcul de la potència
       // printf("pot=%f \n", vad_data->pot_total);
      vad_data->zcr += f.zcr;
    }
    else
    {
      vad_data->P0 = 10 * log10(vad_data->pot_total / t) + vad_data->alfa0;
      vad_data->P1 = vad_data->P0 +  vad_data->alfa1;
      //vad_data->P1 = 10 * log10(vad_data->pot_total / t) + vad_data->alfa1;
    //printf("P0=%f \n", vad_data->P0);
     //printf("alfa0=%f \n", vad_data->alfa0);
     //printf("P1=%f \n", vad_data->P1);
     //printf("alfa1=%f \n", vad_data->alfa1);
    
      vad_data->zcr = vad_data->zcr / t; // A partir del num de creuaments per zero busquem la tassa
      printf("zcr=%f \n", vad_data->zcr);
      vad_data->state = ST_SILENCE;
    }
    /*vad_data->state = MY_SILENCE;
    vad_data->P0=f.p; //Quin serà el valor d epotència umbral de la trama
    break;*/
    break;
  case ST_SILENCE:
  //printf("Estic estat silenci\n");
   //printf("P0=\n");
    vad_data->indef = 0;
    if (f.p > vad_data->P0)
    { // Marcamos un umbral de ruido que no lo detecte
      vad_data->state = MY_VOICE;
    }
    // vad_data->P0=f.p;
    break;

  case ST_VOICE:
    // printf("Estic estat voice");
    //printf("%f\n", f.p);
    /*ad_data->indef = 0;

    if(f.p < vad_data->P1){
    printf("f.zcr= %f\n", f.zcr);
    }*/
    if (f.p < vad_data->P1 && vad_data->zcr > f.zcr)
    {
      vad_data->state = MY_SILENCE;
    }
    break;

  case MY_SILENCE:
     //printf("etsic estat potser silenci");
    vad_data->indef += 1;
    if (f.p < vad_data->P0)
    {
      vad_data->state = ST_SILENCE;
    }
    else if (f.p > vad_data->P1)
    {
      vad_data->state = ST_VOICE;
    }
    else if (vad_data->indef > vad_data->num_MS)
    {
      //printf("num_MS= %d\n", vad_data->num_MS);
      vad_data->state = ST_VOICE; // Si després de dos iteracions al maybe no ens dona cap resultat torem a la veu
    }
    break;

  case MY_VOICE:
    //printf("estic estat potser veu");
    vad_data->indef += 1;
    if (f.p > vad_data->P1)
    {
      vad_data->state = ST_VOICE;
    }
    else if (f.p < vad_data->P0)
    {
      vad_data->state = ST_SILENCE;
    }
    else if (vad_data->indef > vad_data->num_MV)
    {
       printf("num_MV= %d\n", vad_data->num_MV);
      //printf("Hem entrat a indef\n");
      cont++;
      vad_data->state = ST_SILENCE; // Si després de dos iteracions al maybe no ens dona cap resultat torem al silenci
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
  else //if (vad_data->state == ST_INIT)
  {
    return ST_SILENCE;
  }
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out)
{
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}


//Millor 10 3 5 --> Sense forçar umbral
//Millor 3 3 5 --> Forçant l'umbral