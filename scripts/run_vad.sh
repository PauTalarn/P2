#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 alfa0"
    exit -1
fi

# Be sure that this file has execution permissions:
# Use the nautilus explorer or chmod +x run_vad.sh

# Write here the name and path of your program and database
DIR_P2=$HOME/PAV/P2
DB=$DIR_P2/db.v4
CMD="$DIR_P2/bin/vad -0 $1"

for filewav in $DB/*/*wav; do
#    echo
    echo "**************** $filewav ****************"
    if [[ ! -f $filewav ]]; then 
	    echo "Wav file not found: $filewav" >&2 #si el fitxer no existeix
	    exit 1
    fi

    filevad=${filewav/.wav/.vad} #coge la variable de entorno filewav y susituye el .wav por el .vad

    $CMD -i $filewav -o $filevad || exit 1   

# Alternatively, uncomment to create output wave files
#    filewavOut=${filewav/.wav/.vad.wav}
#    $CMD $filewav $filevad $filewavOut || exit 1

done

scripts/vad_evaluation.pl $DB/*/*lab


