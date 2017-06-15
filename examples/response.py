import eve_nerd
import numpy as np
import matplotlib.pyplot as plt

a = eve_nerd.Universe("mapDenormalize.csv", "mapJumps.csv")
a.add_dynamic_bridge(40199046, 6.0)
b = a.get_all_distances(30003135, eve_nerd.JUMP_FREIGHTER)

sys = []
res = {}

for e, d in b.items():
    c = (e.system.id, e.system.x, e.system.z)
    if d < res.get(c, float("inf")):
        res[c] = d

        if c not in sys:
            sys.append(c)

plt.scatter([x[1] for x in sys], [x[2] for x in sys], c=[res[x] for x in sys], alpha=0.5, cmap='RdYlBu', vmin=0, vmax=1800, s=[256.0 if x[0] in [30003135, 30001162] else 16.0 for x in sys])
plt.show()
