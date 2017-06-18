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
public:
    Parameters(float w, float a, float g, float j, float r) {
        this->warp_speed = w;
        this->align_time = a;
        this->gate_cost = g;
        this->jump_range = j;
        this->jump_range_reduction = r;
    }

    Parameters(float w, float a, float g, float j) {
        this->warp_speed = w;
        this->align_time = a;
        this->gate_cost = g;
        this->jump_range = j;
    }

    Parameters(float w, float a, float g) {
        this->warp_speed = w;
        this->align_time = a;
        this->gate_cost = g;
    }

    Parameters(float w, float a) {
        this->warp_speed = w;
        this->align_time = a;
    }

    float jump_range = NAN, warp_speed, align_time = 0.0, gate_cost = 14.0, jump_range_reduction = 0.0;
    enum fatigue_model fatigue_model = FATIGUE_REACTIVATION_COUNTDOWN;
};

static const Parameters FRIGATE = Parameters(5.0, 3.0, 10.0);
static const Parameters DESTROYER = Parameters(4.5, 4.0, 10.0);
static const Parameters INDUSTRIAL = Parameters(4.5, 4.0, 10.0, NAN, 0.9);
static const Parameters CRUISER = Parameters(3.0, 7.0, 10.0);
static const Parameters BATTLECRUISER = Parameters(2.7, 8.0, 10.0);
static const Parameters BATTLESHIP = Parameters(2.0, 12.0, 10.0);

static const Parameters BLACK_OPS = Parameters(2.2, 10.0, 10.0, 8.0, 0.75);

static const Parameters CARRIER = Parameters(1.5, 30.0, -1.0, 7.0);
static const Parameters DREADNOUGHT = Parameters(1.5, 40.0, -1.0, 7.0);
static const Parameters SUPERCARRIER = Parameters(1.5, 40.0, -1.0, 6.0);
static const Parameters TITAN = Parameters(1.37, 60.0, -1.0, 6.0);

static const Parameters RORQUAL = Parameters(1.5, 50.0, -1.0, 10.0, 0.9);
static const Parameters JUMP_FREIGHTER = Parameters(1.5, 40.0, 10.0, 10.0, 0.9);
