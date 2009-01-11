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

class input_loop : public input_loop_base
{
public:
	input_loop() : input_loop_base("loop_") {}
	void open_internal(file::ptr p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();//our input does not support retagging.
		m_loopfile = p_filehint;
		m_path = p_path;
		input_open_file_helper(m_loopfile,p_path,p_reason,p_abort);//if m_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		bool is_utf8;
		text_file_loader::read(m_loopfile,p_abort,m_loopcontent,is_utf8);
		const char * p_content = m_loopcontent;
		pfc::string8 looptype;
		parse_looptype(p_content, looptype);
		pfc::string8 p_content_basepath;
		p_content_basepath.set_string(p_path, pfc::strlen_utf8(p_path) - 5); // .loop
		service_enum_t<input_loop_type_entry> e;
		input_loop_type_entry::ptr ptr;
		while (e.next(ptr)) {
			if ((looptype.is_empty() && !ptr->is_explicit())|| ptr->is_our_type(looptype)) {
				input_loop_type::ptr instance = ptr->instantiate();
				if (instance->parse(p_content) && instance->open_path(NULL, p_content_basepath, p_abort, true, false)) {
					m_loopentry = ptr;
					m_looptype = instance;
					break;
				}
			}
		}

		if (m_looptype.is_empty()) {
			console::formatter() << "loop parsing failed, resume to normal playback: \"" << file_path_display(p_path) << "\"";
			input_loop_type_entry::ptr ptr = new service_impl_t<input_loop_type_impl_t<input_loop_type_none>>();
			input_loop_type::ptr instance = new service_impl_t<input_loop_type_none>();
			if (instance->parse(p_content) && instance->open_path(NULL, p_content_basepath, p_abort, true, false)) {
				m_loopentry = ptr;
				m_looptype = instance;
			}
			PFC_ASSERT(m_looptype.is_valid()); // parse error on input_loop_type_none !?
		}
	}

	static bool g_is_our_content_type(const char * p_content_type) {return false;}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {return stricmp_utf8(p_extension, "loop") == 0;}
};


static input_singletrack_factory_ex_t<input_loop, input_entry::flag_redirect, input_decoder_v2> g_input_loop_factory;


DECLARE_COMPONENT_VERSION("standard looping handler","0.1-dev",NULL);
DECLARE_FILE_TYPE_EX("LOOP","Audio Loop Information File","Audio Loop Information Files");