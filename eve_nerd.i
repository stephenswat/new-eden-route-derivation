%module eve_nerd

%feature("autodoc", "1");

%{
#include "universe.hpp"
#include "parameters.hpp"
%}

%include <std_string.i>
%include <std_list.i>

%include "universe.hpp"
%include "parameters.hpp"

%template(WaypointList) std::list<struct waypoint>;

%extend Route {
%pythoncode {
    def __iter__(self):
        for x in range(len(self.points)):
            yield self.points[x]
}
}
