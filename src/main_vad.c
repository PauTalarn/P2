#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sndfile.h>

#include "vad.h"
#include "vad_docopt.h"

#define DEBUG_VAD 0x1

int main(int argc, char *argv[])
{
  int verbose = 0; /* To show internal state of vad: verbose = DEBUG_VAD; */

  SNDFILE *sndfile_in, *sndfile_out = 0;
  SF_INFO sf_info;
  FILE *vadfile;
  int n_read = 0, i;

  VAD_DATA *vad_data;
  VAD_STATE state, last_state;

  float *buffer, *buffer_zeros;
  int frame_size;         /* in samples */
  float frame_duration;   /* in seconds */
  unsigned int t, last_t; /* in frames */
  float alfa0;
  float alfa1;
  float num_init;
  int num_MS;
  int num_MV;

  char *input_wav, *output_vad, *output_wav;

  DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "2.0");
  // La línea de código que has proporcionado usa la biblioteca Docopt para analizar los argumentos de línea de comando en un programa de C.
  // La función docopt se utiliza para
  //  analizar los argumentos y devuelve una estructura DocoptArgs que contiene los valores de los argumentos.
  // argc i agrv porten la informació a llegir

  // Extraiem totes les variables possibles de la trama que ens hem guardat a args

  verbose = args.verbose ? DEBUG_VAD : 0;
  // La variable args.verbose --> Si el argumento verbose está presente, args.verbose será true, de lo contrario será false.
  // si el argumento verbose está presente, el nivel de verbosidad se establecerá en DEBUG_VAD, de lo contrario se establecerá en 0.
  input_wav = args.input_wav;
  output_vad = args.output_vad;
  output_wav = args.output_wav;
  alfa0 = atof(args.alfa0); // Convertim d'un string llegit a la línia de comandamenrs a un float
  //printf("%f\n", alfa0);
  alfa1 = atof(args.alfa1);
  //printf("%f\n", alfa1);
  num_init = atof(args.num_init);
  num_MS=atof(args.num_MS);
  num_MV=atof(args.num_MV);

  // Error en la lectura
  if (input_wav == 0 || output_vad == 0)
  {
    fprintf(stderr, "%s\n", args.usage_pattern);
    return -1;
  }

  /* Open input sound file */
  if ((sndfile_in = sf_open(input_wav, SFM_READ, &sf_info)) == 0)
  {
    fprintf(stderr, "Error opening input file %s (%s)\n", input_wav, strerror(errno));
    return -1;
  }

  if (sf_info.channels != 1)
  {
    fprintf(stderr, "Error: the input file has to be mono: %s\n", input_wav);
    return -2;
  }

  /* Open vad file */
  if ((vadfile = fopen(output_vad, "wt")) == 0)
  {
    fprintf(stderr, "Error opening output vad file %s (%s)\n", output_vad, strerror(errno));
    return -1;
  }

  /* Open output sound file, with same format, channels, etc. than input */
  if (output_wav)
  {
    if ((sndfile_out = sf_open(output_wav, SFM_WRITE, &sf_info)) == 0)
    {
      fprintf(stderr, "Error opening output wav file %s (%s)\n", output_wav, strerror(errno));
      return -1;
    }
  }

  // Després d'analitzar que no tinguessim errors a l'obrir la trama
  vad_data = vad_open(sf_info.samplerate, alfa0, alfa1, num_init, num_MV, num_MS); // apliquem la funció per crear una nova variable vad_data
  /* Allocate memory for buffers */
  frame_size = vad_frame_size(vad_data);
  //printf("%d\n", frame_size);                           // N'extreiem la info de tamany
  buffer = (float *)malloc(frame_size * sizeof(float)); // Reservem un espai de memòria pel buffer. El tamany de
                                                        // element s'especifica amb sizeof(float) que ho converteix a bytes
                                                        // i passem el resultat finalemt a un puntero de tipus float.
                                                        // El número d'espai en bytses per poder-lo guardar es passa a flotant
  buffer_zeros = (float *)malloc(frame_size * sizeof(float));

  for (i = 0; i < frame_size; ++i)
  {
    buffer_zeros[i] = 0.0F; // Posem tots els valors a 0 del buffer_zero[i].
  }
  frame_duration = (float)frame_size / (float)sf_info.samplerate;
  //printf("Duració de la trama: %f \n", frame_duration);
  last_state = MY_SILENCE; // Inicialment marquem com estat anterior coma ST_UNDEF

  for (t = last_t = 0;; t++)
  { /* For each frame ... */
    /* End loop when file has finished (or there is an error) */
    // printf("%d\n",t);
    if ((n_read = sf_read_float(sndfile_in, buffer, frame_size)) != frame_size)
    {
      //printf("%d\n%d\n", n_read, frame_size);
      break;
    }

    if (sndfile_out != 0)
    {
      sf_write_float(sndfile_out, buffer, frame_size);
    }

    state = vad(vad_data, buffer, t);

    // SI verbose és del tipis DEBUG_VAD mostrem estat en què estem
    if (verbose & DEBUG_VAD)
    {
      vad_show_state(vad_data, stdout);
    }
    /* TODO: print only SILENCE and VOICE labels */
    /* As it is, it prints UNDEF segments but is should be merge to the proper value */
    if (state != last_state)
    {
      //printf("%d\n", t);
      if (state == ST_VOICE)
      {
        last_state = ST_VOICE;
        vad_data->last_state = ST_VOICE; // Per tal que l'últim valor que surti al fer close no sigui una maybe
      }
      else if (state == ST_SILENCE)
      {
        last_state = ST_SILENCE;
        vad_data->last_state = ST_SILENCE;
      }

      if (t != last_t)
      {
        fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration, state2str(last_state));
       // printf("%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration, state2str(last_state));
        last_state = state;
        last_t = t;
      }
    }

    if (sndfile_out != 0 && state == ST_SILENCE)
    {
      sf_seek(sndfile_out, -frame_size, SEEK_CUR);
      sf_write_float(sndfile_out, buffer_zeros, frame_size);
    }
  }

  state = vad_close(vad_data);
  /* TODO: what do you want to print, for last frames? */
  if (t != last_t)
  {
    //printf("%d\n", t);
   //printf("%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration, state2str(last_state));
    fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration + n_read / (float)sf_info.samplerate, state2str(state));
  }
  /* clean up: free memory, close open files */
  free(buffer);
  free(buffer_zeros);
  sf_close(sndfile_in);
  fclose(vadfile);
  if (sndfile_out)
    sf_close(sndfile_out);
  return 0;
}
