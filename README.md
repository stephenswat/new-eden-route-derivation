# new-eden-route-derivation
An open-source advanced routing program for EVE Online.

## Instructions

To run the program, first download and unpack the required data files:

    wget https://www.fuzzwork.co.uk/dump/latest/mapDenormalize.csv.bz2
    wget https://www.fuzzwork.co.uk/dump/latest/mapJumps.csv.bz2
    bunzip2 mapDenormalize.csv.bz2
    bunzip2 mapJumps.csv.bz2

Then compile the program:

    mkdir build
    cd build
    cmake ..
    make

Finally, run the program:

    ./eve_nerd --jump 7.0 -R 40091580:40308384 mapDenormalize.csv mapJumps.csv

Alternatively, in Python:

    import eve_nerd
    u = eve_nerd.Universe("../mapDenormalize.csv", "../mapJumps.csv")
    b = u.route(40004334, 40348191, eve_nerd.BATTLECRUISER)
