# new-eden-route-derivation
An open-source advanced routing program for EVE Online.

## Instructions

To run the program, first download and unpack the required data files:

    wget https://www.fuzzwork.co.uk/dump/latest/mapDenormalize.csv.bz2
    wget https://www.fuzzwork.co.uk/dump/latest/mapJumps.csv.bz2
    bunzip2 mapDenormalize.csv.bz2
    bunzip2 mapJumps.csv.bz2

Then compile the program:

    make

Make sure the compiled shared library can be found by the interface object file
by adding it to the library path:

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`(cd lib; pwd)`

Finally, run the program:

    ./main --jump 7.0 -R 40091580:40308384 mapDenormalize.csv mapJumps.csv
