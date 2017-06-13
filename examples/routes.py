import eve_nerd

def print_route(v):
    print((" %s to %s " % (v.points[0].entity.system.name, v.points[-1].entity.system.name)).center(80, "-"))
    t = int(v.cost)
    print("Estimated travel time: %d:%02d:%02d" % (t / 3600, (t % 3600) / 60, t % 60))

    for x in range(len(v.points)):
        r = v.points[x]

        if r.type == eve_nerd.JUMP:
            print("Jump to %s in %s" % (r.entity.name, r.entity.system.name))
        elif r.type == eve_nerd.GATE:
            print("Gate into %s" % (r.entity.system.name))
        elif r.type == eve_nerd.WARP:
            print("Warp to %s" % (r.entity.name))
        elif r.type == eve_nerd.STRT:
            print("Start at %s in %s" % (r.entity.name, r.entity.system.name))

    print("")

# Load the universe
a = eve_nerd.Universe("mapDenormalize.csv", "mapJumps.csv")

# Go from D-P to G-M using a supercarrier preset
b = a.get_route(30003135, 30004696, eve_nerd.SUPERCARRIER)
print_route(b)

# Go from D-P to Period Basis using a Black Ops
b = a.get_route(30003135, 30004955, eve_nerd.BLACK_OPS)
print_route(b)

b = a.get_route(30003135, 30003632, eve_nerd.CARRIER)
print_route(b)

a.add_dynamic_bridge(40199046, 6.0)

b = a.get_route(30003135, 30004955, eve_nerd.BATTLECRUISER)
print_route(b)

a.add_static_bridge(40297263, 40297596)

b = a.get_route(30003135, 30004704, eve_nerd.BATTLECRUISER)
print_route(b)

c = eve_nerd.Parameters(8.0, 1.5, 10.0)
b = a.get_route(30003135, 30004704, c)
print_route(b)
