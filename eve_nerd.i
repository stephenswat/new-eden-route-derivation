%module eve_nerd

%feature("autodoc", "1");

%{
#include "universe.hpp"
#include "parameters.hpp"

namespace swig {
    template <typename T> swig_type_info *type_info();

    template <> swig_type_info *type_info<waypoint>() {
        return SWIGTYPE_p_waypoint;
    };
}
%}

%include <std_string.i>
%include <std_list.i>

%include "universe.hpp"
%include "parameters.hpp"

%template(WaypointList) std::list<struct waypoint>;

%extend Route {
%pythoncode {
    @property
    def start(self):
        return self.points[0].entity

    @property
    def end(self):
        return self.points[-1].entity

    def __repr__(self):
        return "<Route '%s' to '%s'>" % (self.start.name, self.end.name)
}
}

%extend System {
%pythoncode {
    def __str__(self):
        return self.name

    def __repr__(self):
        return "<System %d : '%s'>" % (self.id, self.name)
}
}

%extend Celestial {
%pythoncode {
    def __str__(self):
        return self.name

    def __repr__(self):
        return "<Celestial %d : '%s'>" % (self.id, self.name)
}
}

%extend waypoint {
%pythoncode {
    def __repr__(self):
        return "<Waypoint type %d to %s>" % (self.type, str(self.entity))
}
}
