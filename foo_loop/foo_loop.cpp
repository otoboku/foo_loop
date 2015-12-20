#include "stdafx.h"
#include "looping.h"


namespace {
	bool parse_looptype(const char * & p_content, pfc::string_base & p_type) {
		// first entity is type.
		pfc::string8 name, value;
		if (!parse_entity(p_content, name, value) || !!pfc::stringCompareCaseInsensitive(name, "type"))
			return false;
		p_type = value;
		return true;
	}
}

struct loop_type_prioritized_entry {
	t_uint8 priority;
	loop_type_entry::ptr ptr;
};

template<typename t_item1, typename t_item2>
inline int looptype_priority_compare(const t_item1 & p_item1, const t_item2 & p_item2);

template<>
inline int looptype_priority_compare(const loop_type_prioritized_entry & p_item1, const loop_type_prioritized_entry & p_item2) {
	return pfc::compare_t(p_item1.priority, p_item2.priority);
}

class input_loop : public input_loop_base
{
public:
	input_loop() : input_loop_base("loop_") {}
	void open_internal(file::ptr p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();//our input does not support retagging.
		m_loopfile = p_filehint;
		m_path = p_path;
		pfc::string8 p_content_basepath;

		t_size len_path		= pfc::strlen_t(p_path);
		t_size pos_filename = pfc::scan_filename(p_path);
		t_size pos_dot1		= pfc::string_find_last(p_path, '.');
		t_size pos_dot2		= pfc::string_find_last(p_path, '.', pos_dot1 - 1);
		t_size pos_question = pfc::string_find_first(p_path, '?', pos_filename);

		const t_size LEN_CONTENT_HEAD = tabsize(".$=[") - 1;

		// filename.ext.loop?LOOP_CONTENT
		if (pos_question != ~0)
		{
			p_content_basepath.set_string(p_path, pos_dot1);
			m_loopcontent.set_string(p_path + pos_question + 1, len_path - pos_question - 1);
			m_loopcontent.replace_char('?', ' ');
		}
		else
		// filename.ext.$=[LOOP_CONTENT].loop
		if (pos_dot2 != ~0 &&
			pos_dot2 > pos_filename &&
			p_path[pos_dot1 - 1] == ']' &&
			pfc::_strcmp_partial_ex(p_path + pos_dot2, LEN_CONTENT_HEAD, ".$=[", LEN_CONTENT_HEAD) == 0)
		{
			p_content_basepath.set_string(p_path, pos_dot2);
			m_loopcontent.set_string(p_path + pos_dot2 + LEN_CONTENT_HEAD, pos_dot1 - pos_dot2 - LEN_CONTENT_HEAD - 1);
		}
		// filename.ext.loop
		else
		{
			p_content_basepath.set_string(p_path, pos_dot1);
			try
			{
				input_open_file_helper(m_loopfile, p_path, p_reason, p_abort);
				bool is_utf8;
				text_file_loader::read(m_loopfile, p_abort, m_loopcontent, is_utf8);
			}
			catch (exception_io_not_found) {}
		}

		const char * p_content = m_loopcontent;
		pfc::string8 looptype;
		if (!parse_looptype(p_content, looptype))
		{
			p_content = m_loopcontent;
		}
		service_enum_t<loop_type_entry> e;
		loop_type_entry::ptr ptr;
		pfc::list_t<loop_type_prioritized_entry> ents;
		bool type_specified = !looptype.is_empty();
		while (e.next(ptr)) {
			// if type specified, use type only. otherwise try auto-probing
			if (type_specified ? ptr->is_our_type(looptype) : !ptr->is_explicit()) {
				loop_type_prioritized_entry ent = {100, ptr};
				loop_type_entry_v2::ptr v2ptr;
				if (ptr->service_query_t<loop_type_entry_v2>(v2ptr)) {
					ent.priority = v2ptr->get_priority();
					v2ptr.release();
				}
				ents.add_item(ent);
			}
		}
		if (ents.get_count() != 0) {
			pfc::array_staticsize_t<t_size> m_perm_by_prio(ents.get_count());
			order_helper::g_fill(m_perm_by_prio);
			ents.sort_get_permutation_t(looptype_priority_compare<loop_type_prioritized_entry, loop_type_prioritized_entry>, m_perm_by_prio.get_ptr());
			for (t_size i=0; i<m_perm_by_prio.get_size(); ++i) {
				ptr = ents.get_item(m_perm_by_prio[i]).ptr;
				loop_type::ptr instance = ptr->instantiate();
				if (instance->parse(p_content) && instance->open_path(NULL, p_content_basepath, p_reason, p_abort, true, false)) {
					m_loopentry = ptr;
					m_looptype = instance;
					break;
				}
			}
			ents.remove_all();
		}
		else if (type_specified)
		{
			throw exception_loop_unsupported_type();
		}

		if (m_looptype.is_empty()) {
			//console::formatter() << "loop parsing failed, resume to normal playback: \"" << file_path_display(p_path) << "\"";
			loop_type_entry::ptr ptr = new service_impl_t<loop_type_impl_t<loop_type_entire>>();
			loop_type::ptr instance = new service_impl_t<loop_type_entire>();
			if (instance->parse(p_content) && instance->open_path(NULL, p_content_basepath, p_reason, p_abort, true, false)) {
				m_loopentry = ptr;
				m_looptype = instance;
			}
			else
			{
				throw exception_loop_target_open_failed();
			}

			//PFC_ASSERT(m_looptype.is_valid()); // parse error on input_loop_type_entire !?
		}
	}

	static bool g_is_our_content_type(const char * p_content_type) {return false;}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {return stricmp_utf8(p_extension, "loop") == 0;}
};


static input_factory_ex_t<input_loop, input_entry::flag_redirect, input_decoder_v2> g_input_loop_factory;


DECLARE_COMPONENT_VERSION("Standard Loop Information Handler","0.4 alpha","Standard Looping Handler.\nThis includes .loop and .sli support.");
DECLARE_FILE_TYPE_EX("LOOP","Audio Loop Information File","Audio Loop Information Files");