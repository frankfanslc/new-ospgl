#include "LuaVehicle.h"
#include "../../universe/vehicle/Vehicle.h"

void LuaVehicle::load_to(sol::table& table)
{
	table.new_usertype<Vehicle>("vehicle");	

	table.new_usertype<Piece>("piece",
		"rigid_body", &Piece::rigid_body,
		"welded", &Piece::welded,
		"get_global_transform", [](Piece& self)
		{
			return BulletTransform(self.get_global_transform());
		},
		"get_graphics_transform", [](Piece& self)
		{
			return BulletTransform(self.get_graphics_transform());
		},
		"get_local_transform", [](Piece& self)
		{
			return BulletTransform(self.get_local_transform());
		},
		"get_linear_velocity", [](Piece& self)
		{
			return to_dvec3(self.get_linear_velocity());
		},
		"get_angular_velocity", [](Piece& self)
		{
			return to_dvec3(self.get_angular_velocity());
		},
		"get_tangential_velocity", [](Piece& self)
		{
			return to_dvec3(self.get_tangential_velocity());
		},
		"get_relative_position", [](Piece& self)
		{
			return to_dvec3(self.get_relative_position());
		},
		"get_global_position", [](Piece& self)
		{
			return to_dvec3(self.get_global_transform().getOrigin());
		},
		"get_graphics_position", [](Piece& self)
		{
			return to_dvec3(self.get_graphics_transform().getOrigin());
		},
		"get_forward", &Piece::get_forward,
		"get_up", &Piece::get_up,
		"get_right", &Piece::get_right,
		"transform_axis", &Piece::transform_axis,
			
		"get_marker_position", &Piece::get_marker_position,
		"get_marker_rotation", &Piece::get_marker_rotation,
		"get_marker_transform", &Piece::get_marker_transform,
		"get_marker_forward", &Piece::get_marker_forward,
		"transform_point_to_rigidbody", &Piece::transform_point_to_rigidbody);

	table.new_usertype<Part>("part",
		"get_piece", &Part::get_piece,
		"get_machine", &Part::get_machine);

	table.new_usertype<Machine>("machine",
		"init_toml", &Machine::init_toml,
		"call", [](Machine& self, const std::string& fname, sol::variadic_args args)
		{
			auto result = LuaUtil::safe_call_function(self.lua_state, fname, "machine->machine call", args);	
			// safe_call_function only returns valid results so
			sol::reference as_ref = result;
			return as_ref;
		}
		
	);

}
