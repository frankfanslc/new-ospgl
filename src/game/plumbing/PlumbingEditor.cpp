#include "PlumbingEditor.h"
#include <lua/libs/LuaNanoVG.h>

PlumbingEditor::PlumbingEditor()
{
	cam_center = glm::vec2(0.0f, 0.0f);
	zoom = 32;
	veh = nullptr;
	in_drag = false;
	in_selection = false;
	in_pipe_drag = false;
	allow_editing = false;
	allow_dragging = false;
	show_flow_direction = false;
	allow_tooltip = false;

}

void PlumbingEditor::draw_grid(NVGcontext* vg, glm::vec4 span) const
{

	nvgBeginPath(vg);

	int max_halfx = (int)((span.z / 2.0f) / (float)zoom);
	int max_halfy = (int)((span.w / 2.0f) / (float)zoom);

	nvgResetTransform(vg);
	glm::vec2 center = glm::vec2(span.x + span.z * 0.5f, span.y + span.w * 0.5f);
	nvgTranslate(vg, center.x-fmod(cam_center.x, 1.0f) * zoom, center.y-fmod(cam_center.y, 1.0f) * zoom);
	nvgScale(vg, (float)zoom, (float)zoom);
	nvgStrokeWidth(vg, 1.0f / (float)zoom);
	// Vertical lines
	for(int x = -max_halfx - 1; x < max_halfx + 1; x++)
	{
		nvgMoveTo(vg, (float)x, -(float)max_halfy - 1);
		nvgLineTo(vg, (float)x, (float)max_halfy + 1);
	}

	for(int y = -max_halfy - 1; y < max_halfy + 1; y++)
	{
		nvgMoveTo(vg, -(float)max_halfx - 1, (float)y);
		nvgLineTo(vg, (float)max_halfx + 1, (float)y);
	}
	nvgStroke(vg);
}


void PlumbingEditor::draw_machines(NVGcontext* vg, glm::vec4 span) const
{
	nvgResetTransform(vg);

	glm::vec2 center = glm::vec2(span.x + span.z * 0.5f, span.y + span.w * 0.5f);

	int i = 0;

	std::vector<PlumbingMachine*> elements = veh->plumbing.get_all_elements();

	for(PlumbingMachine* pb : elements)
	{
		bool is_selected = false;
		bool is_conflict = false;
		for (PlumbingMachine* elem : selected)
		{
			if (elem == pb)
			{
				is_selected = true;
				break;
			}
		}

		for (PlumbingMachine* elem: drag_conflicts)
		{
			if (elem == pb)
			{
				is_conflict = true;
				break;
			}
		}

		nvgStrokeColor(vg, skin->get_foreground_color(true));
		nvgFillColor(vg, skin->get_background_color(true));

		if (hovered == pb || is_selected)
		{
			nvgStrokeWidth(vg, 4.0f / (float) zoom);
		}
		else
		{
			nvgStrokeWidth(vg, 2.0f / (float) zoom);
		}

		if (is_selected)
		{
			nvgFillColor(vg, skin->get_highlight_color());
		}

		if (is_conflict)
		{
			nvgStrokeColor(vg, skin->get_error_color());
		}

		nvgResetTransform(vg);
		glm::vec2 ppos = pb->editor_position;

		// Preview drag
		if (in_machine_drag && is_selected)
		{
			glm::vec2 offset = mouse_current - mouse_start;
			ppos += glm::round(offset);
		}

		nvgTranslate(vg, center.x + (ppos.x - cam_center.x) * (float) zoom,
					 center.y + (ppos.y - cam_center.y) * (float) zoom);
		nvgScale(vg, (float) zoom, (float) zoom);
		glm::ivec2 size = pb->get_size(false, false);
		// The logical rotation simply changes x and y dimensions, which is similar
		// to tipping over the box
		if (pb->editor_rotation == 1)
		{
			nvgTranslate(vg, (float) size.y, 0.0f);
		}
		else if (pb->editor_rotation == 2)
		{
			nvgTranslate(vg, (float) size.x, (float) size.y);
		}
		else if (pb->editor_rotation == 3)
		{
			nvgTranslate(vg, 0.0f, (float) size.x);
		}
		nvgRotate(vg, glm::half_pi<float>() * (float)pb->editor_rotation);

		pb->draw_diagram((void *) vg, skin);

		// Draw the ports
		for (const FluidPort &port: pb->fluid_ports)
		{
			bool hover_port = hovered_port == &port && hovered == pb;
			draw_port(vg, port.pos, hover_port);
		}

	}
}

void PlumbingEditor::draw_tooltip(NVGcontext* vg, glm::vec4 span) const
{
	glm::vec2 ppos = hovered->editor_position;
	glm::vec2 center = glm::vec2(span.x + span.z * 0.5f, span.y + span.w * 0.5f);
	glm::vec2 mpos = glm::floor(get_mouse_pos(span)) + glm::vec2(0.8f);

	nvgResetTransform(vg);
	nvgTranslate(vg, center.x - cam_center.x * (float) zoom,
				 center.y - cam_center.y * (float) zoom);
	nvgStrokeColor(vg, skin->get_foreground_color(true));
	nvgFillColor(vg, skin->get_background_color(true));
	nvgStrokeWidth(vg, 2.0f);
	// TODO: Rotation

	if (hovered_port)
	{
		glm::vec2 pos = mpos * (float) zoom;
		float pressure = 0.0f;
		if(hovered_port->is_flow_port)
		{
			// TODO, read from the pipe or something
		}
		else
		{
			pressure = hovered->get_pressure(hovered_port->id);
		}
		float flow = 0.0f;
		int connected_pipe = veh->plumbing.find_pipe_connected_to(hovered_port);
		float size = (float)hovered_port->gui_name.length() * 8.5f;
		float height = 30.0f;
		std::stringstream pressure_stream;
		std::stringstream flow_stream;
		if (connected_pipe >= 0)
		{
			Pipe *p = &veh->plumbing.pipes[connected_pipe];
			flow = p->flow;
			if (p->b == hovered_port)
			{
				flow = -flow;
			}

			pressure_stream << "P: " << std::fixed << std::setprecision(2) << pressure / 101325.0 << " atm";
			flow_stream << "F: " << std::fixed << std::setprecision(2) << flow << " kg/s";
			size = std::max(pressure_stream.str().length(), flow_stream.str().length());
			size = std::max(size, (float) hovered_port->gui_name.length());
			size *= 8.0f;
			height = 70.0f;
		}
		nvgBeginPath(vg);
		nvgRect(vg, pos.x, pos.y, size, height);
		nvgFill(vg);
		nvgStroke(vg);

		nvgFontSize(vg, 16.0f);
		nvgFillColor(vg, skin->get_foreground_color());
		nvgText(vg, pos.x + 5.0f, pos.y + 20.0f, hovered_port->gui_name.c_str(), nullptr);
		if(connected_pipe >= 0)
		{
			nvgText(vg, pos.x + 5.0f, pos.y + 40.0f, pressure_stream.str().c_str(), nullptr);
			nvgText(vg, pos.x + 5.0f, pos.y + 60.0f, flow_stream.str().c_str(), nullptr);
		}
	}
}

bool PlumbingEditor::update_mouse(GUIInput* gui_input, glm::vec4 span)
{
	bool block = false;
	bool inside_and_not_blocked = is_inside_and_not_blocked(gui_input, span);

	// Click and drag
	if((inside_and_not_blocked || in_drag) && allow_dragging)
	{
		if (gui_input->mouse_down(1))
		{
			last_click = input->mouse_pos;
			last_center = cam_center;
			in_drag = true;
		}

		if(gui_input->mouse_up(1))
		{
			in_drag = false;
		}

		if(gui_input->mouse_pressed(1))
		{
			glm::vec2 delta = input->mouse_pos - last_click;
			cam_center = last_center - delta / (float)zoom;
			block = true;
		}

	}

	// Zooming in and out
	if(inside_and_not_blocked && allow_dragging)
	{
		block = true;
		double delta = input->mouse_scroll_delta;
		zoom += (double)zoom * delta * 0.1;
		zoom = glm::clamp(zoom, 24, 80);
	}

	return block;
}

bool PlumbingEditor::update_dragging(GUIInput *gui_input, glm::vec2 mpos)
{
	if(in_machine_drag)
	{
		mouse_current = mpos;
		glm::vec2 offset = glm::round(mouse_current - mouse_start);

		int prev_rotation = -1;
		double time_passed = glfwGetTime() - time_held;
		// Rotation
		if(input->mouse_up(0) && offset == glm::vec2(0, 0) && selected.size() == 1
			&& time_passed < max_time_for_rotation)
		{
			prev_rotation = selected[0]->editor_rotation;
			selected[0]->editor_rotation++;
			if(selected[0]->editor_rotation == 4)
			{
				selected[0]->editor_rotation = 0;
			}
		}

		drag_conflicts.clear();
		for(PlumbingMachine* elem : selected)
		{
			glm::ivec2 end_pos = elem->editor_position + glm::ivec2(offset);
			if(!veh->plumbing.grid_aabb_check(end_pos,end_pos + elem->get_size(true),
											  selected, true).empty())
			{
				drag_conflicts.push_back(elem);
			}
		}


		if(input->mouse_up(0))
		{
			if(drag_conflicts.empty())
			{
				for (PlumbingMachine* elem : selected)
				{
					elem->editor_position += glm::ivec2(offset);
				}
			}

			in_machine_drag = false;
			if(prev_rotation != -1 && !drag_conflicts.empty())
			{
				selected[0]->editor_rotation = prev_rotation;
			}
			drag_conflicts.clear();
		}
		return true;
	}
	else if(!in_pipe_drag && allow_editing)
	{
		bool hovered_selected = false;
		for (PlumbingMachine* elem : selected)
		{
			if (elem == hovered)
			{
				hovered_selected = true;
				break;
			}
		}

		if (gui_input->mouse_down(0) && hovered_selected)
		{
			in_machine_drag = true;
			time_held = glfwGetTime();
			mouse_start = mpos;
			mouse_current = mpos;
			return true;
		}

		return false;
	}

	return false;
}

void PlumbingEditor::handle_hovering(GUIInput *gui_input, glm::vec2 mpos)
{
	// First check if we are hovering any port

	auto hover_vec = veh->plumbing.grid_aabb_check(mpos, mpos);
	if (hover_vec.size() >= 1)
	{
		PlumbingMachine* elem = hover_vec[0];
		hovered = elem;
		if (gui_input->mouse_down(2))
		{
			on_middle_click(elem->in_machine);
		}
	}

	// Go over every port and check if it's hovered

}

bool PlumbingEditor::update_selection(GUIInput *gui_input, glm::vec4 span)
{
	bool block = false;
	bool dragged = false;
	bool inside_and_not_blocked = is_inside_and_not_blocked(gui_input, span);

	glm::vec2 mpos = get_mouse_pos(span);

	// Hovering
	if(inside_and_not_blocked)
	{
		handle_hovering(gui_input, mpos);
	}

	if(!in_selection && inside_and_not_blocked && !in_drag || in_machine_drag)
	{
		dragged = update_dragging(gui_input, mpos);
		block |= dragged;
	}

	// Left click + drag does box selection and unselects everything else
	// Shift + Left click adds to selection with box

	if(!in_drag && !in_selection && inside_and_not_blocked && !dragged && !in_pipe_drag && allow_editing)
	{
		if(gui_input->mouse_down(0))
		{
			// Check if we are over a port or a pipe, this will generate a new pipe

			in_selection = true;
			mouse_start = mpos;
		}
	}

	if(in_selection)
	{
		mouse_current = mpos;
		if(gui_input->mouse_up(0))
		{
			if(!input->key_pressed(GLFW_KEY_LEFT_SHIFT) && !input->key_pressed(GLFW_KEY_RIGHT_SHIFT))
			{
				selected.clear();
			}

			// AABB check
			if(mouse_start.x > mouse_current.x)
			{
				std::swap(mouse_start.x, mouse_current.x);
			}
			if(mouse_start.y > mouse_current.y)
			{
				std::swap(mouse_start.y, mouse_current.y);
			}
			auto new_sel = veh->plumbing.grid_aabb_check(mouse_start, mouse_current, selected);
			selected.insert(selected.end() , new_sel.begin(), new_sel.end());

			in_selection = false;
		}
	}

	return block;
}

bool PlumbingEditor::update_pipes(GUIInput *gui_input, glm::vec4 span)
{
	glm::vec2 mpos = get_mouse_pos(span);
	glm::ivec2 round = glm::floor(mpos);
	// Hovering ports
	std::vector<PlumbingMachine*> all_elems = veh->plumbing.get_all_elements();
	for (PlumbingMachine* elem : all_elems)
	{
		for (FluidPort& port : elem->fluid_ports)
		{
			glm::vec2 port_pos = elem->correct_editor_pos(port.pos);
			// If in pipe drag we just need to be in the same square to hover the port
			if ((!in_pipe_drag && glm::distance(mpos, port_pos) <= port_radius * 1.15f) ||
					(in_pipe_drag && round == (glm::ivec2)glm::floor(port_pos)))
			{
				hovered_port = &port;
				hovered = elem;
				break;
			}
		}
	}

	if(in_pipe_drag)
	{
		Pipe* p = &veh->plumbing.pipes[hovering_pipe];
		selected.clear();
		if(gui_input->mouse_down(0))
		{
			if(hovered_port)
			{
				// If we end where we begin, we remove the pipe
				if(hovered_port == p->a || hovered_port == p->b)
				{
					veh->plumbing.pipes.erase(veh->plumbing.pipes.begin() + hovering_pipe);
				}
				else
				{
					p->b = hovered_port;
				}
				in_pipe_drag = false;
				hovering_pipe = -1;
				return true;
			}
			else
			{
				// Add a waypoint at mouse pos (if no pipe is already there), remove it if it's already there
				// TODO: maybe create a junction automatically?
				auto it = std::find(p->waypoints.begin(), p->waypoints.end(), round);
				if(it == p->waypoints.end())
				{
					// try to find it in another pipe
					bool found_in_other = false;
					for(Pipe& other_p : veh->plumbing.pipes)
					{
						if(&other_p == p)
							continue;

						for(size_t j = 0; j < other_p.waypoints.size(); j++)
						{
							if(other_p.waypoints[j] == round)
							{
								found_in_other = true;
								break;
							}
						}
					}

					if(found_in_other == false)
					{
						p->waypoints.push_back(round);
						return true;
					}
				}
				else
				{
					p->waypoints.erase(it);
					return true;
				}
			}
		}
	}
	else
	{
		// Creating new pipes, finishing unfinished ones, or modifying them
		if (hovered_port && gui_input->mouse_down(0))
		{
			int user_id = veh->plumbing.find_pipe_connected_to(hovered_port);
			if(user_id >= 0)
			{
				// Modify existing pipe, multiple conditions
				hovering_pipe = user_id;
				in_pipe_drag = true;

				Pipe* p = &veh->plumbing.pipes[user_id];
				if(hovered_port == p->a)
				{
					// Modify from the start (invert pipe)
					p->invert();
				}

				// Disconnect the pipe
				p->b = nullptr;
				return true;
			}
			else
			{
				// Create new pipe
				int n_pipe_id = veh->plumbing.create_pipe();
				Pipe* n_pipe = &veh->plumbing.pipes[n_pipe_id];
				n_pipe->a = hovered_port;
				hovering_pipe = n_pipe_id;
				in_pipe_drag = true;
				return true;
			}
		}
	}

	return false;
}

bool PlumbingEditor::is_inside_and_not_blocked(GUIInput *gui_input, glm::vec4 span)
{
	return gui_input->mouse_inside(span) &&
		!gui_input->mouse_blocked && !gui_input->ext_mouse_blocked &&
		!gui_input->scroll_blocked && !gui_input->ext_scroll_blocked;
}


glm::vec2 PlumbingEditor::get_mouse_pos(glm::vec4 span) const
{
	glm::vec2 window_relative = input->mouse_pos;
	glm::vec2 span_relative;
	span_relative.x = (window_relative.x - span.x) / span.z;
	span_relative.y = (window_relative.y - span.y) / span.w;
	glm::vec2 center_relative = span_relative - glm::vec2(0.5f, 0.5f);
	center_relative.x *= span.z;
	center_relative.y *= span.w;

	return cam_center + center_relative / (float)zoom;

}

void PlumbingEditor::draw_selection(NVGcontext *vg, glm::vec4 span) const
{
	if(in_selection)
	{
		nvgResetTransform(vg);
		glm::vec2 center = glm::vec2(span.x + span.z * 0.5f, span.y + span.w * 0.5f);
		nvgTranslate(vg, center.x - cam_center.x * (float) zoom, center.y - cam_center.y * (float) zoom);
		nvgScale(vg, (float) zoom, (float) zoom);

		nvgStrokeColor(vg, skin->get_highlight_color());
		nvgFillColor(vg, nvgTransRGBA(skin->get_highlight_color(), 128));
		nvgStrokeWidth(vg, 1.0f / (float)zoom);

		nvgBeginPath(vg);
		nvgRect(vg, mouse_start.x, mouse_start.y,
				mouse_current.x - mouse_start.x, mouse_current.y - mouse_start.y);
		nvgStroke(vg);
		nvgFill(vg);
	}
}

void PlumbingEditor::draw_pipe_cap(NVGcontext *vg, glm::vec2 pos) const
{
	float r = port_radius / sqrtf(2.0f);
	nvgMoveTo(vg, pos.x, pos.y);
	nvgLineTo(vg, pos.x + r, pos.y + r);
	nvgMoveTo(vg, pos.x, pos.y);
	nvgLineTo(vg, pos.x - r, pos.y - r);
	nvgMoveTo(vg, pos.x, pos.y);
	nvgLineTo(vg, pos.x - r, pos.y + r);
	nvgMoveTo(vg, pos.x, pos.y);
	nvgLineTo(vg, pos.x + r, pos.y - r);
}

void PlumbingEditor::pipe_line_to(NVGcontext *vg, glm::vec2 pos, glm::vec2 old_pos, float flow) const
{
	// Draw the line
	nvgMoveTo(vg, old_pos.x, old_pos.y);
	nvgLineTo(vg, pos.x, pos.y);

	if(show_flow_direction)
	{
		float dist = glm::distance(pos, old_pos);
		glm::vec2 dir = glm::normalize(pos - old_pos);
		glm::vec2 norm = glm::vec2(-dir.y, dir.x);
		float sign = (flow > 0.0f) ? 1.0f : -1.0f;
		float size = 0.15f;
		if(glm::abs(flow) < 0.1f)
		{
			size *= glm::abs(flow) * 10.0f;
		}

		for(float offset = 1.0f; offset <= dist - 1.0f; offset += 1.0f)
		{
			glm::vec2 cur_pos = old_pos + dir * offset;
			glm::vec2 off1 = dir * size * sign + norm * size;
			glm::vec2 off2 = dir * size * sign + norm * -size;
			// Draw a little arrow
			nvgMoveTo(vg, cur_pos.x, cur_pos.y);
			nvgLineTo(vg, cur_pos.x + off1.x, cur_pos.y + off1.y);
			nvgMoveTo(vg, cur_pos.x, cur_pos.y);
			nvgLineTo(vg, cur_pos.x + off2.x, cur_pos.y + off2.y);
		}

	}

}

void PlumbingEditor::draw_pipes(NVGcontext *vg, glm::vec4 span) const
{
	// TODO: Add a guard for non interactive editor?
	glm::vec2 mpos = get_mouse_pos(span);

	nvgResetTransform(vg);
	glm::vec2 center = glm::vec2(span.x + span.z * 0.5f, span.y + span.w * 0.5f);
	nvgTranslate(vg, center.x - cam_center.x * (float) zoom, center.y - cam_center.y * (float) zoom);
	nvgScale(vg, (float) zoom, (float) zoom);

	for(size_t pipe_id = 0; pipe_id < veh->plumbing.pipes.size(); pipe_id++)
	{
		nvgStrokeColor(vg, skin->get_foreground_color());
		float size = 1.0f;
		// If we hover a port, the joined pipe gets in bold
		if(veh->plumbing.find_pipe_connected_to(hovered_port) == pipe_id)
		{
			size = 2.0f;
		}
		nvgStrokeWidth(vg, size / (float)zoom);
		nvgBeginPath(vg);

		glm::vec2 pos;

		Pipe* p = &veh->plumbing.pipes[pipe_id];
		// We start on the first port, or first waypoint if such is not present
		if(p->a != nullptr)
		{
			pos = p->a->in_machine->correct_editor_pos(p->a->pos);
			if(in_machine_drag && std::find(selected.begin(), selected.end(), p->a->in_machine) != selected.end())
			{
				pos += glm::round(mpos - mouse_start);
			}
		}
		else
		{
			pos = (glm::vec2)p->waypoints[0] + glm::vec2(0.5f);
		}

		draw_pipe_cap(vg, pos);
		nvgMoveTo(vg, pos.x, pos.y);
		glm::vec2 prev = pos;

		for(auto waypoint : p->waypoints)
		{
			pipe_line_to(vg, (glm::vec2)waypoint + glm::vec2(0.5f), prev, p->flow);
			prev = (glm::vec2)waypoint + glm::vec2(0.5f);
		}

		glm::vec2 end_pos;
		if(p->b != nullptr)
		{
			end_pos = p->b->in_machine->correct_editor_pos(p->b->pos);
			if(in_machine_drag && std::find(selected.begin(), selected.end(), p->b->in_machine) != selected.end())
			{
				end_pos += glm::round(mpos - mouse_start);
			}
		}
		else if(in_pipe_drag && hovering_pipe == pipe_id)
		{
			end_pos = mpos;
		}
		else
		{
			// An unfinished pipe must have an end waypoint
			end_pos = p->waypoints[p->waypoints.size() - 1];
		}

		pipe_line_to(vg, end_pos, prev, p->flow);
		draw_pipe_cap(vg, end_pos);
		nvgStroke(vg);


		// While dragging, waypoints must be visible to allow modification or junction creation
		if(in_pipe_drag)
		{
			if(pipe_id == hovering_pipe)
			{
				nvgFillColor(vg, skin->get_foreground_color());
				nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 0));
			}
			else
			{
				nvgFillColor(vg, skin->get_background_color());
				nvgStrokeColor(vg, nvgTransRGBA(skin->get_foreground_color(), 255));
			}

			for(size_t i = 0; i < p->waypoints.size(); i++)
			{
				// Draw waypoints as dots so the user knows where they are
				nvgBeginPath(vg);
				nvgCircle(vg, p->waypoints[i].x + 0.5f, p->waypoints[i].y + 0.5f, 4.0f / (float)zoom);
				nvgFill(vg);
				nvgStroke(vg);
			}
		}
	}

}

void PlumbingEditor::draw_collisions(NVGcontext* vg, glm::vec4 span) const
{
	if (in_machine_drag && !drag_conflicts.empty())
	{
		glm::vec2 center = glm::vec2(span.x + span.z * 0.5f, span.y + span.w * 0.5f);

		int i = 0;

		std::vector<PlumbingMachine*> elems = veh->plumbing.get_all_elements();
		for(PlumbingMachine* elem : elems)
		{
			// If it's selected, we don't check collision!
			if(std::find(selected.begin(), selected.end(), elem) != selected.end())
			{
				continue;
			}

			nvgStrokeWidth(vg, 2.0f / (float)zoom);
			nvgStrokeColor(vg, skin->get_error_color());
			nvgResetTransform(vg);

			glm::vec2 ppos = elem->editor_position;

			nvgTranslate(vg, center.x + (ppos.x - cam_center.x) * (float)zoom,
						 center.y + (ppos.y - cam_center.y) * (float)zoom);
			nvgScale(vg, (float)zoom, (float)zoom);

			// Draw the collision envelope, this is expanded
			glm::ivec2 size = elem->get_size();
			nvgBeginPath(vg);
			nvgRect(vg, -1.0, -1.0, size.x + 2.0, size.y + 2.0);
			nvgStroke(vg);
		}
	}
}

void PlumbingEditor::draw_port(NVGcontext *vg, glm::vec2 pos, bool hovered_port) const
{
	if(hovered_port)
	{
		nvgFillColor(vg, skin->get_foreground_color());
	}
	else
	{
		nvgFillColor(vg, skin->get_background_color());
	}

	nvgBeginPath(vg);
	nvgCircle(vg, pos.x, pos.y, port_radius);
	nvgFill(vg);
	nvgStroke(vg);
}

void PlumbingEditor::prepare(GUIInput *gui_input, glm::vec4 span)
{
	if(veh == nullptr)
	{
		logger->warn("Plumbing editor prepared without assigned vehicle");
		return;
	}

	bool block = false;
	hovered = nullptr;
	hovered_port = nullptr;
	block |= update_mouse(gui_input, span);
	block |= update_pipes(gui_input, span);
	block |= update_selection(gui_input, span);

	if(block)
	{
		gui_input->ext_mouse_blocked = true;
		gui_input->ext_scroll_blocked = true;
		gui_input->mouse_blocked = true;
		gui_input->scroll_blocked = true;
	}
}

void PlumbingEditor::do_editor(NVGcontext *vg, glm::vec4 span, GUISkin* skin)
{
	this->skin = skin;
	nvgSave(vg);
	nvgScissor(vg, span.x, span.y, span.z, span.w);

	nvgStrokeColor(vg, skin->get_background_color(true));
	draw_grid(vg, span);
	draw_machines(vg, span);
	draw_pipes(vg, span);
	draw_selection(vg, span);
	draw_collisions(vg, span);
	if(allow_tooltip && hovered)
	{
		// The tooltip is not scissored
		nvgResetScissor(vg);
		draw_tooltip(vg, span);
	}

	nvgRestore(vg);
}



