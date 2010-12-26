
#include <stdafx.h>
#include "looping.h"


namespace loop_helper {
	void console_complain_looping(const char * what, const char * msg) {
		console_looping_formatter() << what << ": " << msg;
	}

	void open_path_helper(input_decoder::ptr & p_input, file::ptr p_file, const char * path,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints) {
		p_abort.check();
		p_input.release();

		TRACK_CODE("input_entry::g_open_for_decoding",
			input_entry::g_open_for_decoding(p_input,p_file,path,p_abort,p_from_redirect)
			);

		
		if (!p_skip_hints) {
			try {
				static_api_ptr_t<metadb_io>()->hint_reader(p_input.get_ptr(),path,p_abort);
			} catch(exception_io_data) {
				//Don't fail to decode when this barfs, might be barfing when reading info from another subsong than the one we're trying to decode etc.
				p_input.release();
				if (p_file.is_valid()) p_file->reopen(p_abort);
				TRACK_CODE("input_entry::g_open_for_decoding",
					input_entry::g_open_for_decoding(p_input,p_file,path,p_abort,p_from_redirect)
					);
			}
		}
	}

	bool parse_entity(const char * & ptr,pfc::string8 & name,pfc::string8 & value) {
		char delimiter = '\0';
		char tmp;
		t_size n = 0;
		while(isspace((unsigned char) *ptr)) ptr++;
		while(tmp = ptr[n], tmp && !isspace((unsigned char) tmp) && tmp != '=') n++;
		if (!ptr[n]) return false;
		name.set_string(ptr, n);
		ptr += n;
		while(isspace((unsigned char) *ptr)) ptr++;
		if (*ptr != '=') return false;
		ptr++;
		// check delimiter
		if (*ptr == '\'' || *ptr == '"') {
			delimiter = *ptr;
			ptr++;
		}
		if (!*ptr) false;

		n = 0;
		if (delimiter == '\0') {
			while(tmp = ptr[n], tmp && !isspace((unsigned char) tmp) && tmp != ';') n++;
		} else {
			while(tmp = ptr[n], tmp && tmp != delimiter) n++;
		}
		if (!ptr[n]) return false;
		value.set_string(ptr, n);
		ptr += n;
		if (*ptr == delimiter) ptr++;
		while(*ptr == ';' || isspace((unsigned char) *ptr)) ptr++;
		return true;
	}

	t_filestats merge_filestats(const t_filestats & p_src1, const t_filestats & p_src2, int p_merge_type) {
		t_filestats dest;
		dest.m_timestamp = pfc::max_t(p_src1.m_timestamp, p_src2.m_timestamp);
		if (p_merge_type == merge_filestats_sum) {
			dest.m_size = p_src1.m_size + p_src2.m_size;
		} else if (p_merge_type == merge_filestats_max) {
			dest.m_size = pfc::max_t(p_src1.m_size, p_src2.m_size);
		} else {
			throw pfc::exception_not_implemented();
		}
		return dest;
	}

	// {E50D5DE0-6F95-4b1c-9165-63D80415ED1B}
	FOOGUIDDECL const GUID loop_type_base::guid_cfg_branch_loop = 
	{ 0xe50d5de0, 0x6f95, 0x4b1c, { 0x91, 0x65, 0x63, 0xd8, 0x4, 0x15, 0xed, 0x1b } };
	// {69B1AEBB-E2A6-4c73-AEA2-5037A79B1B62}
	static const GUID guid_cfg_loop_debug = 
	{ 0x69b1aebb, 0xe2a6, 0x4c73, { 0xae, 0xa2, 0x50, 0x37, 0xa7, 0x9b, 0x1b, 0x62 } };
	// {9208BA62-AFBE-450e-A468-72792DAE5193}
	static const GUID guid_cfg_loop_disable = 
	{ 0x9208ba62, 0xafbe, 0x450e, { 0xa4, 0x68, 0x72, 0x79, 0x2d, 0xae, 0x51, 0x93 } };

	static advconfig_branch_factory cfg_loop_branch("Looping",loop_type_base::guid_cfg_branch_loop,advconfig_branch::guid_branch_decoding,0);
	advconfig_checkbox_factory cfg_loop_debug("Loop Debugging",guid_cfg_loop_debug,loop_type_base::guid_cfg_branch_loop,0,false);
	advconfig_checkbox_factory cfg_loop_disable("Loop Disable",guid_cfg_loop_disable,loop_type_base::guid_cfg_branch_loop,0,false);

}

#pragma region GUIDs
//// {C9E7AF50-FDF8-4a2f-99A6-8DE4D2B49D0C}
FOOGUIDDECL const GUID loop_type::class_guid = 
{ 0xc9e7af50, 0xfdf8, 0x4a2f, { 0x99, 0xa6, 0x8d, 0xe4, 0xd2, 0xb4, 0x9d, 0xc } };

//// {CA8E32C1-1A2D-4679-87AB-03292A97D890}
FOOGUIDDECL const GUID loop_type_base::class_guid = 
{ 0xca8e32c1, 0x1a2d, 0x4679, { 0x87, 0xab, 0x3, 0x29, 0x2a, 0x97, 0xd8, 0x90 } };

//// {D751AD10-1EC1-4711-8698-22ED1C900503}
//FOOGUIDDECL const GUID loop_type_base_v2::class_guid = 
//{ 0xd751ad10, 0x1ec1, 0x4711, { 0x86, 0x98, 0x22, 0xed, 0x1c, 0x90, 0x5, 0x3 } };

//// {2910A6A6-A12B-414f-971B-90A65F79439B}
FOOGUIDDECL const GUID loop_event_point::class_guid = 
{ 0x2910a6a6, 0xa12b, 0x414f, { 0x97, 0x1b, 0x90, 0xa6, 0x5f, 0x79, 0x43, 0x9b } };

//// {566BCC79-7370-48c0-A7CB-5E47C4C17A86}
FOOGUIDDECL const GUID loop_type_entry::class_guid = 
{ 0x566bcc79, 0x7370, 0x48c0, { 0xa7, 0xcb, 0x5e, 0x47, 0xc4, 0xc1, 0x7a, 0x86 } };

//// {399E8435-5341-4549-8C9D-176979EC4300}
FOOGUIDDECL const GUID loop_type_entry_v2::class_guid = 
{ 0x399e8435, 0x5341, 0x4549, { 0x8c, 0x9d, 0x17, 0x69, 0x79, 0xec, 0x43, 0x0 } };

#pragma endregion

