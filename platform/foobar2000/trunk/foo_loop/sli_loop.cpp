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
#include "looping.h"

#define SLI_FLAGS 16
#define SLI_MIN_FLAG_VALUE 0
#define SLI_MAX_FLAG_VALUE 9999

typedef pfc::array_staticsize_t<int> t_flags_array;
class loop_condition {
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

class formula_operator {
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

class sli_link : public input_loop_event_point_baseimpl {
public:
	virtual bool set_smooth_samples(t_size samples) = 0;
	FB2K_MAKE_SERVICE_INTERFACE(sli_link, input_loop_event_point);
};

class sli_link_impl : public sli_link {
private:
	t_size smooth_samples;
protected:
	sli_link_impl() : from(0), to(0), smooth(false), condition(NULL), refvalue(0), condvar(0), smooth_samples(0) {}
	~sli_link_impl() {
		if (condition != NULL) delete condition;
	}
public:
	virtual t_uint64 get_position() const {return from;}
	virtual t_uint64 get_prepare_position() const {return from - smooth_samples;}
	virtual bool set_smooth_samples(t_size samples) {
		if (!smooth) return false;
		smooth_samples = (t_size) pfc::min_t((t_uint64) samples, pfc::min_t(from, to));
		return true;
	}
	t_uint64 from;
	t_uint64 to;
	bool smooth;
	loop_condition * condition;
	unsigned int refvalue;
	unsigned int condvar;

	virtual void get_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) {
		pfc::string8 name, buf;
		t_size prefixlen;
		name << p_prefix;
		prefixlen = name.get_length();

		name.truncate(prefixlen);
		name << "from";
		p_info.info_set(name, format_samples_ex(from, sample_rate));

		name.truncate(prefixlen);
		name << "to";
		p_info.info_set(name, format_samples_ex(to, sample_rate));

		name.truncate(prefixlen);
		name << "type";
		buf << "link";
		if (smooth) {
			buf << "; smooth";
		}
		if (condition != NULL && condition->is_valid) {
			buf << "; cond:";
			buf << "[" << condvar << "]";
			buf << condition->symbol << refvalue;
		}

		p_info.info_set(name, buf);	
	}
	virtual bool check(input_loop_type_base::ptr p_input);
	virtual bool process(input_loop_type_base::ptr p_input, t_uint64 p_start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort);
	virtual bool process(input_loop_type_base::ptr p_input, abort_callback & p_abort) {
		// this event do not process on no_looping
		if (p_input->get_no_looping() || !check(p_input)) return false;
		p_input->raw_seek(to, p_abort);
		return true;
	}
};

class sli_label : public input_loop_event_point_baseimpl {
public:
	virtual t_uint64 get_position() const {return position;}
	virtual t_uint64 get_prepare_position() const {return position;}
	t_uint64 position;
	pfc::string8 name;
	virtual void get_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) {
		pfc::string8 name, buf;
		t_size prefixlen;
		name << p_prefix;
		prefixlen = name.get_length();

		name.truncate(prefixlen);
		name << "type";
		p_info.info_set(name, "label");

		name.truncate(prefixlen);
		name << "pos";
		p_info.info_set(name, format_samples_ex(position, sample_rate));

		if (this->name) {
			if (this->name[0] == ':') {
				name.truncate(prefixlen);
				name << "formula";
				p_info.info_set(name, this->name.get_ptr() + 1);
			} else {
				name.truncate(prefixlen);
				name << "name";
				p_info.info_set(name, this->name);
			}
		}
	}
	virtual bool process(input_loop_type_base::ptr p_input, t_uint64 p_start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
		return process(p_input, p_abort);
	}
	virtual bool process(input_loop_type_base::ptr p_input, abort_callback & p_abort);
};

class sli_label_formula {
public:
	sli_label_formula() : oper(NULL) {}
	~sli_label_formula() {
		if (oper != NULL) delete oper;
	}
	unsigned int flag;
	formula_operator * oper;
	bool indirect;
	unsigned int value;
};

bool parse_sli_entity(const char * & ptr,pfc::string8 & name,pfc::string8 & value) {
	char delimiter = '\0';
	char tmp;
	t_size n = 0;
	while(isspace(*ptr)) ptr++;
	while(tmp = ptr[n], tmp && !isspace(tmp) && tmp != '=') n++;
	if (!ptr[n]) return false;
	name.set_string(ptr, n);
	ptr += n;
	while(isspace(*ptr)) ptr++;
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
		while(tmp = ptr[n], tmp && !isspace(tmp) && tmp != ';') n++;
	} else {
		while(tmp = ptr[n], tmp && tmp != delimiter) n++;
	}
	if (!ptr[n]) return false;
	value.set_string(ptr, n);
	ptr += n;
	if (*ptr == delimiter) ptr++;
	while(*ptr == ';' || isspace(*ptr)) ptr++;
	return true;
}

bool parse_sli_link(const char * & ptr,sli_link_impl &link) {
	// must point '{' , which indicates start of the block.
	if(*ptr != '{') return false;
	ptr++;

	while (*ptr) {
		if (isspace(*ptr)) {
			while (isspace(*ptr)) ptr++;
		} else if (*ptr == '}') {
			break;
		} else {
			pfc::string8 name, value;
			if (!parse_sli_entity(ptr, name, value)) return false;
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
		}
	}

	if (*ptr != '}') return false;
	ptr++;
	return true;
}

bool parse_sli_label(const char * & ptr,sli_label &label) {
	// must point '{' , which indicates start of the block.
	if(*ptr != '{') return false;
	ptr++;

	while (*ptr) {
		if (isspace(*ptr)) {
			while (isspace(*ptr)) ptr++;
		} else if (*ptr == '}') {
			break;
		} else {
			pfc::string8 name, value;
			if (!parse_sli_entity(ptr, name, value)) return false;
			if (!pfc::stricmp_ascii(name, "Position")) {
				if (!pfc::string_is_numeric(value)) return false;
				label.position = pfc::atoui64_ex(value, strlen(value));
			} else if (!pfc::stricmp_ascii(name, "Name")) {
				label.name.set_string(value);
			} else {
				return false;
			}
		}
	}

	if (*ptr != '}') return false;
	ptr++;
	return true;
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

class input_loop_type_sli : public input_loop_type_impl_singlefile_base
{
private:
	input_decoder::ptr m_input;
	input_loop_event_point_list m_points;
	pfc::array_staticsize_t<int> m_flags;
	bool m_no_flags;
	t_size m_crossfade_samples_half;
public:
	static const char * g_get_name() {return "kirikiri-SLI";}
	static const char * g_get_short_name() {return "sli";}
	static bool g_is_our_type(const char * type) {return !pfc::stringCompareCaseInsensitive(type, "sli");}
	static bool g_is_explicit() {return true;}
	virtual bool parse(const char * ptr) {
		if (!*ptr) { return false; }
		m_points.remove_all();
		m_no_flags = true;
		if (*ptr != '#') {
			// v1
			const char * p_length = strstr(ptr, "LoopLength=");
			const char * p_start = strstr(ptr, "LoopStart=");
			if (!p_length || !p_start) return false;
			p_length += 11;
			p_start += 10;
			if (!pfc::char_is_numeric(*p_length) || !pfc::char_is_numeric(*p_start)) return false;
			sli_link_impl * link = new service_impl_t<sli_link_impl>();
			link->smooth = false;
			link->condition = new loop_condition_no();
			link->refvalue = 0;
			link->condvar = 0;
			__int64 start = _atoi64(p_start);
			link->from = start + _atoi64(p_length);
			link->to = start;
			m_points.add_item(link);

			return true;
		}
		if (!pfc::strcmp_partial(ptr, "#2.00")) {
			// v2
			while (*ptr) {
				if (*ptr == '#') {
					// FIXME: original source checks only beginning-of-line...
					while (*ptr && *ptr != '\n') ptr++;
				} else if (isspace(*ptr)) {
					while (isspace(*ptr)) ptr++;
				} else if (pfc::stricmp_ascii(ptr, "Link") && !pfc::char_is_ascii_alpha(ptr[4])) {
					ptr += 4;
					while (isspace(*ptr)) ptr++;
					if (!*ptr) return false;
					sli_link_impl * link = new service_impl_t<sli_link_impl>();
					if (!parse_sli_link(ptr, *link)) return false;
					if (m_no_flags && link->condition != NULL && link->condition->is_valid)
						m_no_flags = false;
					m_points.add_item(link);
				} else if (pfc::stricmp_ascii(ptr, "Label") && !pfc::char_is_ascii_alpha(ptr[5])) {
					ptr += 5;
					while (isspace(*ptr)) ptr++;
					if (!*ptr) return false;
					sli_label * label = new service_impl_t<sli_label>();
					if (!parse_sli_label(ptr, *label)) return false;
					m_points.add_item(label);
				} else {
					return false;
				}
			}
			return true;
		}
		return false;		
	}
	virtual t_size get_crossfade_samples_half () const {return m_crossfade_samples_half;}
	virtual bool open_path_internal(file::ptr p_filehint,const char * path,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints) {
		open_path_helper(m_input, p_filehint, path, p_abort, p_from_redirect,p_skip_hints);
		switch_input(m_input);
		return true;
	}
	virtual void open_decoding_internal(t_uint32 subsong, t_uint32 flags, abort_callback & p_abort) {
		m_crossfade_samples_half = MulDiv_Size(get_sample_rate(), 25, 1000);
		for (t_size i = m_points.get_count()-1; i != infinite_size; i--) {
			service_ptr_t<sli_link> link;
			if (m_points[i]->service_query_t<sli_link>(link)) {
				if (link->set_smooth_samples(m_crossfade_samples_half)) {
					set_is_raw_supported(false);
				}
			}
		}
		switch_points(m_points);
		if (!m_no_flags) {
			m_flags.set_size_discard(SLI_FLAGS);
			pfc::fill_array_t(m_flags, 0);
		}
		input_loop_type_impl_singlefile_base::open_decoding_internal(subsong, flags, p_abort);
	}
	virtual void get_info(t_uint32 subsong, file_info & p_info,abort_callback & p_abort) {
		get_input()->get_info(subsong, p_info, p_abort);
		get_info_for_points(p_info, m_points, get_info_prefix(), get_sample_rate());
	}

	virtual bool set_dynamic_info(file_info & p_out) {
		input_loop_type_impl_base::set_dynamic_info(p_out);
		if (!m_no_flags) {
			pfc::string8 buf;
			t_size num = m_flags.get_size();
			for ( t_size i = 0; i < num; i++ )
				buf << "/[" << i << "]=" << m_flags[i];
			p_out.info_set("sli_flags", buf+1);
		}
		return true;
	}

	virtual bool reset_dynamic_info(file_info & p_out) {
		return input_loop_type_impl_base::reset_dynamic_info(p_out) | p_out.info_remove("sli_flags");
	}
	bool check_condition(sli_link_impl & p_link) {
		if (m_no_flags || p_link.condition == NULL || !p_link.condition->is_valid) 
			return true; // invalid is always true
		return p_link.condition->check(m_flags[p_link.condvar], p_link.refvalue);
	}
	void process_formula(const char * p_formula) {
		if (m_no_flags) return;
		sli_label_formula formula;
		if (parse_sli_label_formula(p_formula, formula)) {
			t_size flagnum = m_flags.get_size();
			t_size flag = formula.flag;
			unsigned int value = formula.value;
			if (formula.indirect) {
				value = m_flags[value];
			}
			m_flags[flag] = formula.oper->calculate(m_flags[flag], value);
		}
	}
	FB2K_MAKE_SERVICE_INTERFACE(input_loop_type_sli, input_loop_type);
};

static input_loop_type_factory_t<input_loop_type_sli> g_input_loop_type_sli;

// {5EEA84FA-6765-4917-A800-791AE10809E1}
FOOGUIDDECL const GUID sli_link::class_guid = 
{ 0x5eea84fa, 0x6765, 0x4917, { 0xa8, 0x0, 0x79, 0x1a, 0xe1, 0x8, 0x9, 0xe1 } };

// {E697CCC0-0ABF-46bd-BA8F-B19096765368}
FOOGUIDDECL const GUID input_loop_type_sli::class_guid = 
{ 0xe697ccc0, 0xabf, 0x46bd, { 0xba, 0x8f, 0xb1, 0x90, 0x96, 0x76, 0x53, 0x68 } };

bool sli_link_impl::check(input_loop_type_base::ptr p_input) {
	input_loop_type_sli::ptr p_input_special;
	if (p_input->service_query_t<input_loop_type_sli>(p_input_special)) {
		return p_input_special->check_condition(*this);
	}
	return true;
}

bool sli_link_impl::process(input_loop_type_base::ptr p_input, t_uint64 p_start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
	// this event do not process on no_looping
	if (p_input->get_no_looping() || !check(p_input)) return false;
	t_size point = pfc::downcast_guarded<t_size>(from - p_start);
	input_loop_type_sli::ptr p_input_special;
	if (!smooth || !p_input->service_query_t<input_loop_type_sli>(p_input_special)) {
		truncate_chunk(p_chunk,p_raw,point);
		p_input->raw_seek(to, p_abort);
		return true;
	}
	// smooth
	PFC_ASSERT(p_raw == NULL);
	t_size smooth_first_samples = smooth_samples;
	t_size require_samples = point + p_input_special->get_crossfade_samples_half();
	t_size samples = p_chunk.get_sample_count();
	if (require_samples > samples) {
		samples += p_input->get_more_chunk(p_chunk, p_raw, p_abort, require_samples - samples);
	}
	if (require_samples < samples) {
		truncate_chunk(p_chunk, p_raw, require_samples);// only debug mode ?
	}
	t_size smooth_latter_samples = pfc::min_t(require_samples, samples) - point;
	// seek
	p_input->raw_seek(to - smooth_first_samples, p_abort);
	// get new samples
	audio_chunk_impl_temporary ptmp_chunk;
	samples = p_input->get_more_chunk(ptmp_chunk,NULL,p_abort, smooth_first_samples + smooth_latter_samples);
	smooth_latter_samples = pfc::min_t(samples - smooth_first_samples, smooth_latter_samples);
	// crossfading
	do_crossfade(
		p_chunk, point-smooth_first_samples,
		ptmp_chunk, 0,
		smooth_first_samples, 0, 50);
	do_crossfade(
		p_chunk, point,
		ptmp_chunk, smooth_first_samples,
		smooth_latter_samples, 50, 100);
	p_chunk.set_sample_count(point+smooth_latter_samples);
	t_size smooth_total_samples = smooth_first_samples + smooth_latter_samples;
	audio_chunk_partial_ref latter = audio_chunk_partial_ref(
		ptmp_chunk, smooth_total_samples, samples - smooth_total_samples);
	combine_audio_chunks(p_chunk, latter);
	return true;
}

bool sli_label::process(input_loop_type_base::ptr p_input, abort_callback & p_abort) {
	// this event do not process on no_looping
	if (p_input->get_no_looping()) return false;
	if (name[0] != ':') return false;
	input_loop_type_sli::ptr p_input_special;
	if (p_input->service_query_t<input_loop_type_sli>(p_input_special)) {
		p_input_special->process_formula(name.get_ptr() + 1);
	}
	return false;
}

class input_sli : public input_loop_base
{
public:
	input_sli() : input_loop_base("sli_") {}
	void open_internal(file::ptr p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();//our input does not support retagging.
		m_loopfile = p_filehint;
		m_path = p_path;
		input_open_file_helper(m_loopfile,p_path,p_reason,p_abort);//if m_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		bool is_utf8;
		text_file_loader::read(m_loopfile,p_abort,m_loopcontent,is_utf8);
		pfc::string8 p_content_basepath;
		p_content_basepath.set_string(p_path, pfc::strlen_utf8(p_path) - 4); // .sli
		input_loop_type_entry::ptr ptr = new service_impl_t<input_loop_type_impl_t<input_loop_type_sli>>();
		input_loop_type::ptr instance = new service_impl_t<input_loop_type_sli>();
		if (instance->parse(m_loopcontent) && instance->open_path(NULL, p_content_basepath, p_abort, true, false)) {
			m_loopentry = ptr;
			m_looptype = instance;
		} else {
			throw exception_io_data();
		}
		PFC_ASSERT(m_looptype.is_valid());
	}

	static bool g_is_our_content_type(const char * p_content_type) {return false;}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {return stricmp_utf8(p_extension, "sli") == 0;}
};


static input_singletrack_factory_ex_t<input_sli, input_entry::flag_redirect, input_decoder_v2> g_input_sli_factory;


DECLARE_COMPONENT_VERSION("sli loop manager","0.1-dev",NULL);
DECLARE_FILE_TYPE_EX("SLI", "SLI Loop Information File","SLI Loop Information Files");