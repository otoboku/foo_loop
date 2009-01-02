// foo_sli.cpp : Defines the exported functions for the DLL application.
//

// Licence: GPL v2 (check kirikiri 2 SDK License)

/*
see also:
	* http://devdoc.kikyou.info/tvp/docs/kr2doc/contents/LoopTuner.html
	* https://sv.kikyou.info/trac/kirikiri/browser/kirikiri2/trunk/license.txt
	* https://sv.kikyou.info/trac/kirikiri/browser/kirikiri2/trunk/kirikiri2/src/core/sound/WaveLoopManager.h
	* https://sv.kikyou.info/trac/kirikiri/browser/kirikiri2/trunk/kirikiri2/src/core/sound/WaveLoopManager.cpp
	* https://sv.kikyou.info/trac/kirikiri/browser/kirikiri2/trunk/kirikiri2/src/core/sound/WaveSegmentQueue.h
	* https://sv.kikyou.info/trac/kirikiri/browser/kirikiri2/trunk/kirikiri2/src/core/sound/WaveSegmentQueue.cpp
*/

#include "stdafx.h"

#define SLI_FLAGS 16
#define SLI_MIN_FLAG_VALUE 0
#define SLI_MAX_FLAG_VALUE 9999

namespace {
	typedef pfc::array_staticsize_t<int> t_flags_array;
	class loop_condition : public pfc::refcounted_object_root {
	public:
		const char * const confname;
		const char * const symbol;
		const bool is_valid;
		loop_condition(const char * confname, const char * symbol, bool is_valid) :
		  confname(confname), symbol(symbol), is_valid(is_valid) {}
		virtual bool check(unsigned int a, unsigned int b) = 0;
	};

	class loop_condition_no : public loop_condition {
	public:
		loop_condition_no() : loop_condition("no", NULL, false) {};
		virtual bool check(unsigned int a, unsigned int b) { return true; }
	};

#define DEFINE_LOOP_CONDITION(name, op) \
	class loop_condition_ ##name : public loop_condition { \
	public: \
		loop_condition_ ##name () : loop_condition( #name, #op, true ) {}; \
		virtual bool check(unsigned int a, unsigned int b) { return a op b; } \
	};

	DEFINE_LOOP_CONDITION(eq, ==);
	DEFINE_LOOP_CONDITION(ne, !=);
	DEFINE_LOOP_CONDITION(gt, >);
	DEFINE_LOOP_CONDITION(ge, >=);
	DEFINE_LOOP_CONDITION(lt, <);
	DEFINE_LOOP_CONDITION(le, <=);
#undef DEFINE_LOOP_CONDITION

	class formula_operator : public pfc::refcounted_object_root {
	public:
		const char * const symbol;
		const bool require_operand;
		formula_operator(const char * symbol, bool require_operand) :
			symbol(symbol), require_operand(require_operand) {}
		virtual unsigned int calculate(unsigned int original, unsigned int operand) = 0;
	};

	class formula_operator_set : public formula_operator {
	public:
		formula_operator_set() : formula_operator("=", true) {}
		virtual unsigned int calculate(unsigned int original, unsigned int operand) {
			return operand;
		}
	};

	class formula_operator_add : public formula_operator {
	public:
		formula_operator_add() : formula_operator("+=", true) {}
		virtual unsigned int calculate(unsigned int original, unsigned int operand) {
			return pfc::min_t<unsigned int>(original + operand, SLI_MAX_FLAG_VALUE);
		}
	};

	class formula_operator_sub : public formula_operator {
	public:
		formula_operator_sub() : formula_operator("-=", true) {}
		virtual unsigned int calculate(unsigned int original, unsigned int operand) {
			if (original >= operand) return original - operand;
			else return 0;
		}
	};

	class formula_operator_inc : public formula_operator {
	public:
		formula_operator_inc() : formula_operator("++", false) {}
		virtual unsigned int calculate(unsigned int original, unsigned int operand) {
			if (original == SLI_MAX_FLAG_VALUE) return original;
			else return original + 1;
		}
	};

	class formula_operator_dec : public formula_operator {
	public:
		formula_operator_dec() : formula_operator("--", false) {}
		virtual unsigned int calculate(unsigned int original, unsigned int operand) {
			if (original >= 1) return original - 1;
			else return 0;
		}
	};
	
	struct sli_link {
		unsigned __int64 from;
		unsigned __int64 to;
		bool smooth;
		pfc::refcounted_object_ptr_t<loop_condition> condition;
		unsigned int refvalue;
		unsigned int condvar;

		bool operator<(const sli_link& p_other) const {return this->from < p_other.from;}
		bool operator<(const unsigned __int64 other) const {return this->from < other;}
		bool operator>(const sli_link& p_other) const {return this->from > p_other.from;}
		bool operator>(const unsigned __int64 other) const {return this->from > other;}
		bool operator<=(const sli_link& p_other) const {return this->from <= p_other.from;}
		bool operator<=(const unsigned __int64 other) const {return this->from <= other;}
		bool operator>=(const sli_link& p_other) const {return this->from >= p_other.from;}
		bool operator>=(const unsigned __int64 other) const {return this->from >= other;}
	};

	struct sli_label {
		unsigned __int64 position;
		pfc::string8 name;

		bool operator<(const sli_label& p_other) const {return this->position < p_other.position;}
		bool operator<(const unsigned __int64 other) const {return this->position < other;}
		bool operator>(const sli_label& p_other) const {return this->position > p_other.position;}
		bool operator>(const unsigned __int64 other) const {return this->position > other;}
		bool operator<=(const sli_label& p_other) const {return this->position <= p_other.position;}
		bool operator<=(const unsigned __int64 other) const {return this->position <= other;}
		bool operator>=(const sli_label& p_other) const {return this->position >= p_other.position;}
		bool operator>=(const unsigned __int64 other) const {return this->position >= other;}
	};

	struct sli_label_formula {
		unsigned int flag;
		pfc::refcounted_object_ptr_t<formula_operator> oper;
		bool indirect;
		unsigned int value;
	};

	class t_sli_link_list : public pfc::list_t<sli_link> {
	public:
		void sort() {
			return this->sort_t(pfc::compare_t<sli_link, sli_link>);
		}
		bool bsearch(unsigned __int64 from,t_size &p_index) {
			return this->bsearch_t(pfc::compare_t<sli_link, unsigned __int64>, from, p_index);
		}
	};

	class t_sli_label_list : public pfc::list_t<sli_label> {
	public:
		void sort() {
			return this->sort_t(pfc::compare_t<sli_label, sli_label>);
		}
		bool bsearch(unsigned __int64 from,t_size &p_index) {
			return this->bsearch_t(pfc::compare_t<sli_label, unsigned __int64>, from, p_index);
		}
	};

	bool parse_sli_entity(const char * & parseptr,pfc::string8 & name,pfc::string8 & value) {
		char delimiter = '\0';
		char tmp;
		t_size ptr = 0;
		while(tmp = parseptr[ptr], tmp && !isspace(tmp) && tmp != '=') ptr++;
		if (!parseptr[ptr]) return false;
		name.set_string(parseptr, ptr);
		parseptr += ptr;
		while (isspace(*parseptr)) parseptr++;
		if (*parseptr != '=') return false;
		parseptr++;
		// check delimiter
		if (*parseptr == '\'') {
			delimiter = *parseptr;
			parseptr++;
		}
		if (!*parseptr) false;

		ptr = 0;
		if (delimiter == '\0') {
			while (tmp = parseptr[ptr], tmp && !isspace(tmp) && tmp != ';') ptr++;
		} else {
			while (tmp = parseptr[ptr], tmp && tmp != delimiter) ptr++;
		}
		if (!parseptr[ptr]) return false;
		value.set_string(parseptr, ptr);
		parseptr += ptr;
		if (*parseptr == delimiter) parseptr++;
		return true;
	}

	bool parse_sli_link(const char * & parseptr,sli_link &link) {
		// must point '{' , which indicates start of the block.
		if(*parseptr != '{') return false;
		parseptr++;

		while (*parseptr) {
			if (isspace(*parseptr)) {
				while (isspace(*parseptr)) parseptr++;
			} else if (*parseptr == '}') {
				break;
			} else {
				pfc::string8 name, value;
				if (!parse_sli_entity(parseptr, name, value)) return false;
				if (!pfc::stricmp_ascii(name, "From")) {
					if (!pfc::string_is_numeric(value)) return false;
					link.from = pfc::atoui64_ex(value, strlen(value));
				} else if (!pfc::stricmp_ascii(name, "To")) {
					if (!pfc::string_is_numeric(value)) return false;
					link.to = pfc::atoui64_ex(value, strlen(value));
				} else if (!pfc::stricmp_ascii(name, "Smooth")) {
					if (!pfc::stricmp_ascii(value, "True")) {
						link.smooth = true;
					} else if (!pfc::stricmp_ascii(value, "False")) {
						link.smooth = false;
					} else if (!pfc::stricmp_ascii(value, "Yes")) {
						link.smooth = true;
					} else if (!pfc::stricmp_ascii(value, "No")) {
						link.smooth = false;
					} else {
						// parse error
						return false;
					}
				} else if (!pfc::stricmp_ascii(name, "Condition")) {
					if (!pfc::stricmp_ascii(value, "no")) {
						link.condition = new loop_condition_no();
					} else if (!pfc::stricmp_ascii(value, "eq")) {
						link.condition = new loop_condition_eq();
					} else if (!pfc::stricmp_ascii(value, "ne")) {
						link.condition = new loop_condition_ne();
					} else if (!pfc::stricmp_ascii(value, "gt")) {
						link.condition = new loop_condition_gt();
					} else if (!pfc::stricmp_ascii(value, "ge")) {
						link.condition = new loop_condition_ge();
					} else if (!pfc::stricmp_ascii(value, "lt")) {
						link.condition = new loop_condition_lt();
					} else if (!pfc::stricmp_ascii(value, "le")) {
						link.condition = new loop_condition_le();
					} else {
						return false;
					}
				} else if (!pfc::stricmp_ascii(name, "RefValue")) {
					if (!pfc::string_is_numeric(value)) return false;
					link.refvalue = pfc::clip_t(atoi(value), SLI_MIN_FLAG_VALUE, SLI_MAX_FLAG_VALUE);
				} else if (!pfc::stricmp_ascii(name, "CondVar")) {
					if (!pfc::string_is_numeric(value)) return false;
					link.condvar = pfc::clip_t(atoi(value), 0, SLI_FLAGS-1);
				} else {
					return false;
				}
				while (isspace(*parseptr)) parseptr++;
				if (*parseptr != ';') return false;
				parseptr++;
			}
		}

		if (*parseptr != '}') return false;
		parseptr++;
		return true;
	}
	
	bool parse_sli_label(const char * & parseptr,sli_label &label) {
		// must point '{' , which indicates start of the block.
		if(*parseptr != '{') return false;
		parseptr++;

		while (*parseptr) {
			if (isspace(*parseptr)) {
				while (isspace(*parseptr)) parseptr++;
			} else if (*parseptr == '}') {
				break;
			} else {
				pfc::string8 name, value;
				if (!parse_sli_entity(parseptr, name, value)) return false;
				if (!pfc::stricmp_ascii(name, "Position")) {
					if (!pfc::string_is_numeric(value)) return false;
					label.position = pfc::atoui64_ex(value, strlen(value));
				} else if (!pfc::stricmp_ascii(name, "Name")) {
					label.name.set_string(value);
				} else {
					return false;
				}
				while (isspace(*parseptr)) parseptr++;
				if (*parseptr != ';') return false;
				parseptr++;
			}
		}

		if (*parseptr != '}') return false;
		parseptr++;
		return true;
	}
	
	bool parse_sli(const char * p_slitext,t_sli_link_list & p_outlinks,t_sli_label_list &p_outlabels) {
		const char * parseptr = p_slitext;
		if (!*parseptr) { return false; }
		if (*parseptr != '#') {
			// v1
			const char * p_length = strstr(parseptr, "LoopLength=");
			const char * p_start = strstr(parseptr, "LoopStart=");
			if (!p_length || !p_start) return false;
			p_length += 11;
			p_start += 10;
			if (!pfc::char_is_numeric(*p_length) || !pfc::char_is_numeric(*p_start)) return false;
			sli_link link;
			link.smooth = false;
			link.condition = new loop_condition_no();
			link.refvalue = 0;
			link.condvar = 0;
			__int64 start = _atoi64(p_start);
			link.from = start + _atoi64(p_length);
			link.to = start;
			p_outlinks.add_item(link);

			return true;
		}
		if (!pfc::strcmp_partial(parseptr, "#2.00")) {
			// v2
			while (*parseptr) {
				if (*parseptr == '#') {
					// FIXME: original source checks only beginning-of-line...
					while (*parseptr && *parseptr != '\n') parseptr++;
				} else if (isspace(*parseptr)) {
					while (isspace(*parseptr)) parseptr++;
				} else if (pfc::stricmp_ascii(parseptr, "Link") && !pfc::char_is_ascii_alpha(parseptr[4])) {
					parseptr += 4;
					while (isspace(*parseptr)) parseptr++;
					if (!*parseptr) return false;
					sli_link link;
					if (!parse_sli_link(parseptr, link)) return false;
					p_outlinks.add_item(link);
				} else if (pfc::stricmp_ascii(parseptr, "Label") && !pfc::char_is_ascii_alpha(parseptr[5])) {
					parseptr += 5;
					while (isspace(*parseptr)) parseptr++;
					if (!*parseptr) return false;
					sli_label label;
					if (!parse_sli_label(parseptr, label)) return false;
					p_outlabels.add_item(label);
				} else {
					return false;
				}
			}
			return true;
		}
		return false;		
	}

	bool parse_sli_label_formula(const char * p_formula, sli_label_formula & p_out) {
		const char * p = p_formula;
		t_size ptr;
		// starts with '['
		if (*p != '[') return false;
		p++;
		ptr = 0;
		while (pfc::char_is_numeric(p[ptr])) ptr++;
		if (ptr == 0) return false;
		p_out.flag = pfc::clip_t<unsigned int>(pfc::atoui_ex(p, ptr), 0, SLI_FLAGS-1);
		p += ptr;
		// after flag, should be ']'
		if (*p != ']') return false;
		p++;
		bool sign = false;
		switch (*p) {
			case '=':
				p_out.oper = new formula_operator_set();
				break;
			case '+':
				sign = true;
			case '-':
				if (*p == *(p+1)) {
					if (sign) 
						p_out.oper = new formula_operator_inc();
					else
						p_out.oper = new formula_operator_dec();
					p++;
					break;
				} else if (*(p+1) == '=') {
					if (sign)
						p_out.oper = new formula_operator_add();
					else 
						p_out.oper = new formula_operator_sub();
					p++;
					break;
				}
			default:
				// unknown operator
				return false;
		}
		p++;
		if (!p_out.oper->require_operand) return true;
		p_out.indirect = false;
		if (*p == '[') {
			p_out.indirect = true;
			p++;
		}
		ptr = 0;
		while (pfc::char_is_numeric(p[ptr])) ptr++;
		if (ptr == 0) return false;
		p_out.value = pfc::clip_t<unsigned int>(pfc::atoui_ex(p, ptr), SLI_MIN_FLAG_VALUE,
			(p_out.indirect ? SLI_FLAGS-1 : SLI_MAX_FLAG_VALUE));
		p += ptr;
		if (p_out.indirect) {
			if (*p != ']') return false;
			p++;
		}
		if (*p) return false;
		return true;
	}

	void combine_audio_chunks(audio_chunk & p_first,const audio_chunk & p_second) {
		if (p_first.is_empty()) {
			p_first = p_second;
			return;
		}

		// sanity check
		if (p_first.get_sample_rate() != p_second.get_sample_rate() ||
			p_first.get_channel_config() != p_second.get_channel_config() ||
			p_first.get_channels() != p_second.get_channels()) {
			throw exception_unexpected_audio_format_change();
		}
		int nch = p_first.get_channels();
		t_size first_samples = p_first.get_sample_count();
		t_size offset = first_samples * nch;
		t_size second_samples = p_second.get_sample_count();
		t_size size = second_samples * nch;
		p_first.set_data_size(offset + size);
		pfc::memcpy_t(p_first.get_data()+offset,p_second.get_data(),size);
		p_first.set_sample_count(first_samples + second_samples);
	}

	void do_crossfade(audio_sample * p_dest, const audio_sample * p_src1, const audio_sample * p_src2,
		int nch, t_size samples, t_uint ratiostart, t_uint ratioend) {
		audio_sample blend_step =
			(audio_sample)((ratioend - ratiostart) / 100.0 / samples);
		audio_sample ratio = (audio_sample)ratiostart / 100;
		while (samples) {
			for (int ch = nch-1; ch >= 0; ch--) {
				*p_dest = *p_src1 + (*p_src2 - *p_src1) * ratio;
				p_dest++; p_src1++; p_src2++;
			}
			samples--;
			ratio += blend_step;
		}
	}

	void do_crossfade(audio_chunk & p_dest,t_size destpos,const audio_chunk & p_src1,t_size src1pos,
		const audio_chunk & p_src2,t_size src2pos,t_size samples,t_uint ratiostart,t_uint ratioend)
	{
		if(samples == 0) return; // nothing to do

		// sanity check
		if (p_src1.get_srate() != p_src2.get_srate() ||
			p_src1.get_channel_config() != p_src2.get_channel_config() ||
			p_src1.get_channels() != p_src2.get_channels() ||
			p_dest.get_srate() != p_src1.get_srate() ||
			p_dest.get_channel_config() != p_src1.get_channel_config() ||
			p_dest.get_channels() != p_src1.get_channels()) {
			throw exception_unexpected_audio_format_change();
		}
		// length check
		if (p_src1.get_sample_count() < (src1pos + samples) ||
			p_src2.get_sample_count() < (src2pos + samples)) {
			throw exception_io("p_src1 or p_src2 unsufficient sample");
		}
		p_dest.pad_with_silence(destpos + samples);
		int nch = p_dest.get_channels();
		audio_sample * pd = p_dest.get_data() + (destpos*nch);
		const audio_sample * ps1 = p_src1.get_data() + (src1pos*nch);
		const audio_sample * ps2 = p_src2.get_data() + (src2pos*nch);
		do_crossfade(pd, ps1, ps2, nch, samples, ratiostart, ratioend);
	}

	void do_crossfade(audio_chunk & p_dest,t_size destpos,const audio_chunk & p_src,t_size srcpos,
		t_size samples,t_uint ratiostart,t_uint ratioend)
	{
		if(samples == 0) return; // nothing to do

		// sanity check
		if (p_dest.get_srate() != p_src.get_srate() ||
			p_dest.get_channel_config() != p_src.get_channel_config() ||
			p_dest.get_channels() != p_src.get_channels()) {
			throw exception_unexpected_audio_format_change();
		}
		// length check
		if (p_dest.get_sample_count() < (destpos + samples) ||
			p_src.get_sample_count() < (srcpos + samples)) {
			throw exception_io("p_dest or p_src unsufficient sample");
		}
		int nch = p_dest.get_channels();
		audio_sample * pd = p_dest.get_data() + (destpos*nch);
		const audio_sample * ps = p_src.get_data() + (srcpos*nch);
		do_crossfade(pd, pd, ps, nch, samples, ratiostart, ratioend);
	}

	class format_samples_ex {
	private:
		pfc::string_fixed_t<193> m_buffer;
	public:
		format_samples_ex(t_uint64 p_samples,t_uint32 p_sample_rate,unsigned p_extra = 3) {
			m_buffer << pfc::format_time_ex(audio_math::samples_to_time(p_samples,p_sample_rate),p_extra);
			m_buffer << " (";
			m_buffer << pfc::format_int(p_samples);
			m_buffer << ")";
		};
		const char * get_ptr() const {return m_buffer;}
		operator const char * () const {return m_buffer;}
	};
}


class input_sli
{
public:
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();//our input does not support retagging.
		m_slifile = p_filehint;
		m_path = p_path;
		input_open_file_helper(m_slifile,p_path,p_reason,p_abort);//if m_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		{
			t_sli_link_list sli_links;
			t_sli_label_list sli_labels;
			bool is_utf8;
			text_file_loader::read(m_slifile,p_abort,m_slicontent,is_utf8);

			if (!parse_sli(m_slicontent, sli_links, sli_labels)) {
				throw exception_io_unsupported_format();
			}
			m_links = sli_links;
			m_links.sort();
			m_labels = sli_labels;
			m_labels.sort();
			{
				no_flags = true;
				t_size num = m_links.get_count();
				for ( t_size i = 0; i < num; i++ ) {
					loop_condition * cond = m_links[i].condition.get_ptr();
					if (cond && cond->is_valid) {
						no_flags = false;
						break;
					}
				}
			}
			if (!no_flags) {
				m_flags.set_size_discard(SLI_FLAGS);
				pfc::fill_array_t(m_flags, 0);
			}
		}
		pfc::string8 p_content_path;
		p_content_path.set_string(p_path, pfc::strlen_utf8(p_path) - 4); // .sli
		in.open_path(NULL, p_content_path, p_abort, false, false);
		file_info_impl p_info;
		in.get_info(0, p_info, p_abort);
		sample_rate = (t_uint32) p_info.info_get_int("samplerate");
		crossfade_samples_half = MulDiv_Size(sample_rate, 25 /* ms */, 1000);
	}

	void get_info(file_info & p_info,abort_callback & p_abort) {
		in.get_info(0, p_info, p_abort);
		p_info.info_set("sli_content", m_slicontent);
		for (t_size n = 0, m = m_links.get_count(); n < m; ++n ) {
			sli_link link = m_links[n];
			pfc::string8 name;
			pfc::string8 buf;
			t_size prefixlen;
			name << "sli_link_" << pfc::format_int(n, 2) << "_";
			prefixlen = name.get_length();

			name.truncate(prefixlen);
			name << "from";
			p_info.info_set(name, format_samples_ex(link.from, sample_rate));

			name.truncate(prefixlen);
			name << "to";
			p_info.info_set(name, format_samples_ex(link.to, sample_rate));

			name.truncate(prefixlen);
			name << "extra";
			if (link.smooth) {
				buf << "smooth";
			}
			if (link.condition->is_valid) {
				if (!buf.is_empty())
					buf << "; ";
				buf << "cond:";
				buf << "[" << link.condvar << "]";
				buf << link.condition->symbol << link.refvalue;
			}

			if (!buf.is_empty())
				p_info.info_set(name, buf);
		}
		for (t_size n = 0, m = m_labels.get_count(); n < m; ++n ) {
			sli_label label = m_labels[n];
			pfc::string8 name;
			t_size prefixlen;
			name << "sli_label_" << pfc::format_int(n, 2) << "_";
			prefixlen = name.get_length();

			name.truncate(prefixlen);
			name << "pos";
			p_info.info_set(name, format_samples_ex(label.position, sample_rate));

			if (label.name) {
				if (label.name[0] == ':') {
					name.truncate(prefixlen);
					name << "formula";
					p_info.info_set(name, label.name.get_ptr() + 1);
				} else {
					name.truncate(prefixlen);
					name << "name";
					p_info.info_set(name, label.name);
				}
			}
		}
		// FIXME
	}
	t_filestats get_file_stats(abort_callback & p_abort) {return m_slifile->get_stats(p_abort);}

	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		no_looping = (p_flags & input_flag_simpledecode) != 0;
		in.open_decoding(0,p_flags,p_abort);
		if (!no_looping) {
			cur = 0;
			schedule_nextevent(cur);
			check_event(p_abort);
		}
	}

	t_size get_nearest_link(t_uint64 pos) {
		t_size nums = m_links.get_count();
		if (!nums) return infinite_size;
		t_size index;
		m_links.bsearch(pos, index);
		if (index < nums) {
			return index;
		} else {
			return infinite_size;
		}
	}

	t_size get_nearest_label(t_uint64 pos) {
		t_size nums = m_labels.get_count();
		if (!nums) return infinite_size;
		t_size index;
		m_labels.bsearch(pos, index);
		if (index < nums) {
			return index;
		} else {
			return infinite_size;
		}
	}

	inline t_uint64 link_get_process_start_samples(sli_link & link) {
		if (!link.smooth) return link.from;
		else if (link.from > crossfade_samples_half) return link.from - crossfade_samples_half;
		else return 0;
	}

	void schedule_nextevent(t_uint64 pos) {
		t_size index;
		if (no_looping) return;
		
		index = get_nearest_link(pos);
		if (index != infinite_size) {
			sli_link link = m_links[index];
			m_nextlinkpos = link_get_process_start_samples(link);
		} else {
			m_nextlinkpos = infinite64;
		}

		if (no_flags) return;
		index = get_nearest_label(pos);
		if (index != infinite_size) {
			m_nextlabelpos = m_labels[index].position;
		} else {
			m_nextlabelpos = infinite64;
		}
	}

	inline void check_event(abort_callback & p_abort) {
		if (no_looping) return;
		if (m_nextlinkpos == cur) {
			// seeking (ignore crossfade)
			t_size num = m_links.get_count();
			t_size index = get_nearest_link(cur);
			while (index < num) {
				sli_link nextlink = m_links[index];
				if (nextlink.from != cur) break;
				loop_condition * cond = nextlink.condition.get_ptr();
				if (!cond || !cond->is_valid || cond->check(m_flags[nextlink.condvar], nextlink.refvalue)) {
					in.seek(audio_math::samples_to_time(nextlink.to,sample_rate),p_abort);
					cur = nextlink.to;
					schedule_nextevent(cur);
					check_event(p_abort);
					return;
				}
				index++;
			}
		}
	}

	inline bool process_labels(t_uint64 p_start, t_uint64 p_end) {
		if (no_looping || no_flags) return false;
		if (p_start == cur && m_nextlabelpos > p_end) {
			// use cached result and no match
			return false;
		}
		t_size num = m_labels.get_count();
		t_size index = get_nearest_label(p_start);
		while (index < num) {
			sli_label label = m_labels[index];
			if (label.position > p_end) break;
			if (!label.name.is_empty() && label.name[0] == ':') {
				sli_label_formula formula;
				if (parse_sli_label_formula(label.name.get_ptr() + 1, formula)) {
					t_size flagnum = m_flags.get_size();
					t_size flag = formula.flag;
					unsigned int value = formula.value;
					if (formula.indirect) {
						value = m_flags[value];
					}
					m_flags[flag] = formula.oper->calculate(m_flags[flag], value);
				}
			}
			index++;
		}
		return true;
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		if (no_looping) return in.run(p_chunk,p_abort);
		check_event(p_abort);
		bool succ = in.run(p_chunk,p_abort);
		t_size samples = p_chunk.get_sample_count();
		t_uint64 end = cur + samples;
		if (m_nextlinkpos <= end) {
			process_link(succ, p_chunk, p_abort);
		} else {
			if (process_labels(cur, end)) {
				schedule_nextevent(end);
			}
			cur = end;
		}
		check_event(p_abort);
		return succ;
	}

	void process_link(bool & succ,audio_chunk & p_chunk,abort_callback & p_abort) {
		// FIXME: be able to dispatch first matched event only
		t_size num = m_links.get_count();
		t_size index = get_nearest_link(cur);
		t_size samples = p_chunk.get_sample_count();
		t_uint64 end = cur + samples;
		t_uint64 labelprocessed = cur;
		while (index < num) {
			sli_link nextlink = m_links[index];
			if (link_get_process_start_samples(nextlink) > end) break;
			// process labels until nextlink
			process_labels(labelprocessed, nextlink.from);
			labelprocessed = nextlink.from;
			loop_condition * cond = nextlink.condition.get_ptr();
			if (cond && cond->is_valid && !cond->check(m_flags[nextlink.condvar], nextlink.refvalue)) {
				// condition failed. dispatch next event
				index++;
				continue;
			}
			// must not occured event on start point (processed in previous decode)
			PFC_ASSERT(cur != nextlink.from);
			// ok; continue..
			if (!nextlink.smooth) {
				// tiny seeking
				in.seek(audio_math::samples_to_time(nextlink.to,sample_rate),p_abort);
				// cut tail
				p_chunk.set_sample_count(pfc::downcast_guarded<t_size>(nextlink.from - cur));
				cur = end = nextlink.to;
				break;
			}
			// crossfading
			t_size cf_samples_first = pfc::downcast_guarded<t_size>(
				nextlink.from - link_get_process_start_samples(nextlink));
			t_size cf_samples_latter = crossfade_samples_half;
			t_size src_cf_center_samples = pfc::downcast_guarded<t_size>(nextlink.from - cur);
			t_size candidate = src_cf_center_samples + cf_samples_latter;
			t_size tmpsize;
			while (succ && candidate > p_chunk.get_sample_count()) {
				// more data
				audio_chunk_impl_temporary ptmp_chunk;
				succ = in.run(ptmp_chunk,p_abort);
				combine_audio_chunks(p_chunk, ptmp_chunk);
			}
			tmpsize = p_chunk.get_sample_count();
			if (candidate > tmpsize) {
				// encount eof
				cf_samples_latter = tmpsize - src_cf_center_samples;
			}
			t_uint64 dest_first = nextlink.to;
			if (cf_samples_first > dest_first) {
				cf_samples_first = static_cast<t_size>(dest_first);
				dest_first = 0;
			} else {
				dest_first -= cf_samples_first;
			}
			// seeking to destination
			in.seek(audio_math::samples_to_time(dest_first,sample_rate),p_abort);
			succ = true;
			audio_chunk_impl_temporary ptmp_dest_chunk;
			candidate = cf_samples_first + cf_samples_latter;
			while (succ && candidate > ptmp_dest_chunk.get_sample_count()) {
				// get data
				audio_chunk_impl_temporary ptmp_chunk;
				succ = in.run(ptmp_chunk,p_abort);
				combine_audio_chunks(ptmp_dest_chunk,ptmp_chunk);
			}
			t_size destsamples = ptmp_dest_chunk.get_sample_count();
			if (destsamples < candidate) {
				cf_samples_latter = destsamples - cf_samples_first;
			}
			t_size cf_samples = cf_samples_first + cf_samples_latter;
			do_crossfade(
				p_chunk, src_cf_center_samples-cf_samples_first,
				ptmp_dest_chunk, 0,
				cf_samples_first, 0, 50);
			do_crossfade(
				p_chunk, src_cf_center_samples,
				ptmp_dest_chunk, cf_samples_first,
				cf_samples_latter, 50, 100);
			p_chunk.set_sample_count(src_cf_center_samples+cf_samples_latter);
			audio_chunk_partial_ref latter = audio_chunk_partial_ref(
				ptmp_dest_chunk, cf_samples, destsamples - cf_samples);
			combine_audio_chunks(p_chunk, latter);
			samples = p_chunk.get_sample_count();
			cur = end = nextlink.to + destsamples - cf_samples_first;
			process_labels(nextlink.to, cur);
			break;
		}
		if (cur != end) {
			// dispatch labels (because all link not matched)
			process_labels(labelprocessed, end);
		}
		schedule_nextevent(end);
	}

	void decode_seek(double p_seconds,abort_callback & p_abort) {
		in.seek(p_seconds,p_abort);
		if (no_looping) return;
		cur = audio_math::time_to_samples(p_seconds,sample_rate);
		schedule_nextevent(cur);
		check_event(p_abort);
	}

	bool decode_can_seek() {return in.can_seek();}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
		bool ret = in.get_dynamic_info(p_out, p_timestamp_delta);
		if (!no_looping && (cur < m_lastupdatedynamic || cur - m_lastupdatedynamic > (sample_rate / 2))) {
			if (p_timestamp_delta == 0) {
				p_timestamp_delta = 0.5;
			} else {
				p_timestamp_delta = pfc::min_t<double>(0.5, p_timestamp_delta);
			}
			p_out.info_set("sli_current", format_samples_ex(cur, sample_rate));
			p_out.info_set("sli_wait_link_pos", (m_nextlinkpos != infinite64) ? 
				format_samples_ex(m_nextlinkpos, sample_rate) : "None");
			if (!no_flags) {
				p_out.info_set("sli_wait_label_pos", (m_nextlabelpos != infinite64) ? 
					format_samples_ex(m_nextlabelpos, sample_rate) : "None");
				pfc::string8 buf;
				t_size num = m_flags.get_size();
				for ( t_size i = 0; i < num; i++ ) {
					if (!buf.is_empty())
						buf << "/";
					buf << "[" << i << "]=" << m_flags[i];
				}
				p_out.info_set("sli_flags", buf);
			}
			m_lastupdatedynamic = cur;
			ret = true;
		}
		return ret;
	}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
		return in.get_dynamic_info_track(p_out, p_timestamp_delta);
	}

	void decode_on_idle(abort_callback & p_abort) {in.on_idle(p_abort);}

	void retag(const file_info & p_info,abort_callback & p_abort) {throw exception_io_unsupported_format();}

	void set_logger(event_logger::ptr ptr) { in.set_logger(ptr);}

	static bool g_is_our_content_type(const char * p_content_type) {return false;}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {return stricmp_utf8(p_extension, "sli") == 0;}

private:
	t_uint64 m_nextlabelpos;
	t_uint64 m_nextlinkpos;
	t_uint64 m_lastupdatedynamic;
	service_ptr_t<file> m_slifile;
	pfc::string8 m_path;
	pfc::string8 m_slicontent;
	t_uint64 cur;
	t_size crossfade_samples_half;
	bool no_looping;
	bool no_flags;
	t_sli_link_list m_links;
	t_sli_label_list m_labels;
	pfc::array_staticsize_t<int> m_flags;

	t_uint32 sample_rate;

	input_helper in;
};


static input_singletrack_factory_t<input_sli> g_input_sli_factory;


DECLARE_COMPONENT_VERSION("sli repeator","0.1-test",NULL);
DECLARE_FILE_TYPE("sli loop information file","*.SLI");