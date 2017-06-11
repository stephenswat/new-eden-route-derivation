%module eve_nerd
%{
#include "universe.hpp"
#include "parameters.hpp"
%}
%include <std_string.i>
%include <std_list.i>
%include "universe.hpp"
%include "parameters.hpp"
%template(WaypointList) std::list<struct waypoint>;
