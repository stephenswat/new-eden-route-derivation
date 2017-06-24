#pragma once

enum fatigue_model {
    /*
     * Horrible inaccurate fatigue model that does not assign a fatigue-related
     * cost at all.
     */
    FATIGUE_IGNORE,

    /*
     * Uses the reactivation timer as the cost for a jump. Doesn't account for
     * fatigue and may incurr large amount of fatigue.
     */
    FATIGUE_REACTIVATION_COST,

    /*
     * Uses the fatigue timer itself (minus 10 minutes) as the cost. This is a
     * very conservative model and assumes completely waiting out fatigue.
     */
    FATIGUE_FATIGUE_COST,

    FATIGUE_REACTIVATION_COUNTDOWN,
    FATIGUE_FATIGUE_COUNTDOWN,

    FATIGUE_FULL
};

class Parameters {
    #ifdef SWIG
    %feature("kwargs") Parameters;
    #endif

public:
    Parameters(float warp_speed=3.0, float align_time=5.0, float gate_cost=14.0, float jump_range=NAN, float jump_range_reduction=0.0) {
        this->warp_speed = warp_speed;
        this->align_time = align_time;
        this->gate_cost = gate_cost;
        this->jump_range = jump_range;
        this->jump_range_reduction = jump_range_reduction;
    }

    float jump_range = NAN, warp_speed, align_time, gate_cost, jump_range_reduction;
    enum fatigue_model fatigue_model = FATIGUE_FATIGUE_COUNTDOWN;
};

static const Parameters FRIGATE = Parameters(5.0, 3.0);
static const Parameters DESTROYER = Parameters(4.5, 4.0);
static const Parameters INDUSTRIAL = Parameters(4.5, 4.0, 14.0, NAN, 0.9);
static const Parameters CRUISER = Parameters(3.0, 7.0);
static const Parameters BATTLECRUISER = Parameters(2.7, 8.0);
static const Parameters BATTLESHIP = Parameters(2.0, 12.0);

static const Parameters BLACK_OPS = Parameters(2.2, 10.0, 14.0, 8.0, 0.75);

static const Parameters CARRIER = Parameters(1.5, 30.0, NAN, 7.0);
static const Parameters DREADNOUGHT = Parameters(1.5, 40.0, NAN, 7.0);
static const Parameters SUPERCARRIER = Parameters(1.5, 40.0, NAN, 6.0);
static const Parameters TITAN = Parameters(1.37, 60.0, NAN, 6.0);

static const Parameters RORQUAL = Parameters(1.5, 50.0, NAN, 10.0, 0.9);
static const Parameters JUMP_FREIGHTER = Parameters(1.5, 40.0, 14.0, 10.0, 0.9);
