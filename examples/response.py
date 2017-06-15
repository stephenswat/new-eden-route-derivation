import eve_nerd
import numpy as np
import matplotlib.pyplot as plt

LY_TO_M = 9460730472580800.0

a = eve_nerd.Universe("mapDenormalize.csv", "mapJumps.csv")
# a.add_dynamic_bridge(40199046, 6.0)

cases = [
    ("Frigates", eve_nerd.FRIGATE),
    ("Cruisers", eve_nerd.CRUISER),
    ("Battlecruisers", eve_nerd.BATTLECRUISER),
    ("Battleships", eve_nerd.BATTLESHIP),
    ("Carriers", eve_nerd.CARRIER),
    ("Supercarriers", eve_nerd.SUPERCARRIER),
]
fig = plt.figure(figsize=(10, 10), dpi=300)

for i, (n, shiptype) in enumerate(cases):
    b = a.get_all_distances(30003135, shiptype)

    sys = []
    res = {}

    for e, _d in b.items():
        c = (e.system.id, e.system.x / LY_TO_M, e.system.z / LY_TO_M)
        d = _d / 60
        if d < res.get(c, float("inf")):
            res[c] = d

            if c not in sys:
                sys.append(c)


    ax = fig.add_subplot(3, 2, i + 1)
    ax.set_aspect('equal', 'datalim')
    ax.set_title(n)

    p = ax.scatter(
        [x[1] for x in sys],
        [x[2] for x in sys],
        c=[res[x] for x in sys],
        alpha=0.5, cmap='RdYlBu', vmin=0, vmax=30,
        s = 1.0
    )

    cbar = fig.colorbar(p)
    cbar.solids.set_edgecolor("face")

plt.savefig('foo.png')
