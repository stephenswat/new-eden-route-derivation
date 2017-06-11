#pragma once

class Parameters {
public:
    Parameters(double j, double w, double a, double g) {
        this->jump_range = j;
        this->warp_speed = w;
        this->align_time = a;
        this->gate_cost = g;
    }
    double jump_range, warp_speed, align_time, gate_cost;
};

static const Parameters FRIGATE = Parameters(NAN, 5.0, 3.0, 10.0);
static const Parameters DESTROYER = Parameters(NAN, 4.5, 4.0, 10.0);
static const Parameters CRUISER = Parameters(NAN, 3.0, 7.0, 10.0);
static const Parameters BATTLECRUISER = Parameters(NAN, 2.7, 8.0, 10.0);
static const Parameters BATTLESHIP = Parameters(NAN, 2.0, 12.0, 10.0);

static const Parameters CARRIER = Parameters(7.0, 1.5, 30.0, -1.0);
static const Parameters DREADNOUGHT = Parameters(7.0, 1.5, 40.0, -1.0);
static const Parameters SUPERCARRIER = Parameters(6.0, 1.5, 40.0, -1.0);
static const Parameters TITAN = Parameters(6.0, 1.37, 60.0, -1.0);

static const Parameters RORQUAL = Parameters(10.0, 1.5, 50.0, -1.0);
static const Parameters JUMP_FREIGHTER = Parameters(10.0, 1.5, 40.0, -1.0);
