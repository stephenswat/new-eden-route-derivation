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
