#include "stdafx.h"
#include "looping.h"

struct thbgm_entry {
	t_filesize offset, headlength, bodylength;
	pfc::string8 title;
};

typedef pfc::list_t<thbgm_entry, pfc::alloc_fast> thbgm_entry_list;

// private input_decoder
class input_raw_thbgm : public input_decoder_v2
{
public:
	enum {
		bits_per_sample = 16,
		channels = 2,
		sample_rate = 44100,

		bytes_per_sample = bits_per_sample / 8,
		total_sample_width = bytes_per_sample * channels,
	};
	void open(file::ptr p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();
		m_file = p_filehint;
		m_path = p_path;
		input_open_file_helper(m_file,m_path,p_reason,p_abort);
		m_filesize = m_file->get_size(p_abort);
	}

	inline void set_bgmtable(thbgm_entry_list & p_list) {
		m_list = p_list;
	}

	inline void set_title(pfc::string_base & p_title) {
		m_title = p_title;
	}

	t_uint32 get_subsong_count() {
		return m_list.get_size();
	}

	t_uint32 get_subsong(t_uint32 p_subsong) {
		assert(p_subsong < get_subsong_count());
		return p_subsong + 1;
	}

	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
		p_info.info_set_int("samplerate", sample_rate);
		p_info.info_set_int("channels", channels);
		p_info.info_set_int("bitspersample", bits_per_sample);
		p_info.info_set("encoding","lossless");
		p_info.info_set("codec","PCM");
		p_info.info_set_bitrate((bits_per_sample * channels * sample_rate + 500 /* rounding for bps to kbps */) / 1000);
		if (!m_title.is_empty())
			p_info.meta_set("album", m_title);
		if (p_subsong > 0) {
			thbgm_entry ent = m_list[p_subsong-1];
			p_info.set_length(audio_math::samples_to_time(length_to_samples(ent.headlength + ent.bodylength), sample_rate));
			p_info.info_set_int("loop_start", length_to_samples(ent.headlength));
			p_info.info_set_int("loop_length", length_to_samples(ent.bodylength));
			p_info.meta_set("title", ent.title);
			p_info.meta_set("tracknumber", pfc::format_int(p_subsong));
		} else {
			if (m_filesize != filesize_invalid)
				p_info.set_length(audio_math::samples_to_time(length_to_samples(m_filesize), sample_rate));
		}
	}

	t_filestats get_file_stats(abort_callback & p_abort) {
		return m_file->get_stats(p_abort);
	}

	inline t_uint64 length_to_samples(t_filesize p_length) {
		return p_length / total_sample_width;
	}

	inline t_filesize samples_to_length(t_uint64 p_samples) {
		return p_samples * total_sample_width;
	}

	void initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) {
		if (p_subsong > 0) {
			m_entry = m_list[p_subsong-1];
		} else {
			thbgm_entry ent;
			ent.headlength = m_filesize;
			ent.bodylength = 0;
			ent.offset = 0;
			m_entry = ent;
		}
		m_seeksize = m_entry.headlength + m_entry.bodylength;
		seek(0,p_abort);
	}

	bool run_common(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
		enum {
			deltaread = 1024,
		};
		if (m_filepos >= m_seeksize)
			return false;
		t_size deltaread_size = pfc::downcast_guarded<t_size>(pfc::min_t(samples_to_length(deltaread), m_seeksize - m_filepos));
		m_buffer.set_size(deltaread_size);
		t_size deltaread_done = m_file->read(m_buffer.get_ptr(),deltaread_size,p_abort);
		if (deltaread_done == 0) return false;

		p_chunk.set_data_fixedpoint(m_buffer.get_ptr(),deltaread_done,sample_rate,channels,bits_per_sample,audio_chunk::g_guess_channel_config(channels));

		if (p_raw != NULL) {
			p_raw->set(m_buffer.get_ptr(), deltaread_done);
		}

		m_filepos += deltaread_done;

		return true;
	}

	bool run(audio_chunk & p_chunk,abort_callback & p_abort) {
		return run_common(p_chunk,NULL,p_abort);
	}

	bool run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
		return run_common(p_chunk,&p_raw,p_abort);
	}

	void seek(double p_seconds,abort_callback & p_abort) {
		m_file->ensure_seekable();
		t_uint64 samples = audio_math::time_to_samples(p_seconds, sample_rate);
		m_filepos = samples * total_sample_width;
		if (m_filepos > m_seeksize)
			m_filepos = m_seeksize;
		m_file->seek(m_entry.offset + m_filepos, p_abort);
	}

	bool can_seek() {return true;}
	bool get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {return false;}
	bool get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {return false;}

	void on_idle(abort_callback & p_abort) {}

	void set_logger(event_logger::ptr ptr) {}


protected:
	file::ptr m_file;
	thbgm_entry_list m_list;
	thbgm_entry m_entry;
	t_filesize m_filepos;
	t_filesize m_seeksize;
	t_filesize m_filesize;
	pfc::string8 m_title;
	pfc::string8 m_path;
	pfc::array_t<t_uint8> m_buffer;
};

unsigned read_hex(char c)
{
	if (c>='0' && c<='9') return (unsigned)c - '0';
	else if (c>='a' && c<='f') return 0xa + (unsigned)c - 'a';
	else if (c>='A' && c<='F') return 0xa + (unsigned)c - 'A';
	else return 0;
}

template<typename t_inttype>
bool parse_cs_hex(const char * & ptr,t_inttype & value) {
	char tmp;
	t_size n = 0;
	while(isspace((unsigned char) *ptr)) ptr++;
	while(tmp = ptr[n], tmp && !isspace((unsigned char) tmp) && tmp != ',') n++;
	if (n == 0) return false;
	value = 0;
	for (n--;n!=(t_size)-1;n--) {
		value = value << 4 | read_hex(*ptr++);
	}
	if (*ptr == ',') ptr++;
	return true;
}

class loop_type_thbgm : public loop_type_impl_singleinput_base
{
private:
	thbgm_entry_list m_list;
	loop_event_point_list m_points;
	input_decoder::ptr m_input;
	pfc::string8 m_title;
	pfc::string8 m_path;

public:
	static const char * g_get_name() {return "THBGM.DAT";}
	static const char * g_get_short_name() {return "thbgm";}
	static bool g_is_our_type(const char * type) {return !pfc::stringCompareCaseInsensitive(type, "thbgm");}
	static bool g_is_explicit() {return true;}
	virtual bool parse(const char * ptr) {
		assert(ptr != NULL);
		m_list.remove_all();
		m_title.reset();
		m_path.reset();
		while (*ptr) {
			if (*ptr == '#') {
				while (*ptr && *ptr != '\n') ptr++;
				if (*ptr == '\n') ptr++;
			} else if (isspace((unsigned char) *ptr)) {
				while(isspace((unsigned char) *ptr)) ptr++;
			} else if (*ptr == '@') {
				// FIXME: handle path and name
				t_size n = 0;
				t_size end = 0;
				ptr++; // @
				char tmp;
				while(isspace((unsigned char) *ptr)) ptr++;
				while(tmp = ptr[n], tmp != ',')
					if (!tmp) return false;
					else n++;
				end = n;
				while(n!=(t_size)-1 && isspace((unsigned char) ptr[n-1])) n--;
				m_path.set_string(ptr, n);
				ptr+=end;
				ptr++; // ,
				n=0;
				while(isspace((unsigned char) *ptr)) ptr++;
				while(tmp = ptr[n], tmp && tmp != '\n') n++;
				end = n;
				while(n!=(t_size)-1 && isspace((unsigned char) ptr[n-1])) n--;
				m_title.set_string(ptr, n);
				ptr+=end;
				if (*ptr == '\n') ptr++;
			} else if ((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'a' && *ptr <= 'f') || (*ptr >= 'A' && *ptr <= 'F')) {
				thbgm_entry ent;
				if (!parse_cs_hex(ptr, ent.offset)) return false;
				if (!parse_cs_hex(ptr, ent.headlength)) return false;
				if (!parse_cs_hex(ptr, ent.bodylength)) return false;
				t_size n = 0;
				t_size end = 0;
				char tmp;
				while(isspace((unsigned char) *ptr)) ptr++;
				while(tmp = ptr[n], tmp && tmp != '\n') n++;
				end = n;
				while(n!=(t_size)-1 && isspace((unsigned char) ptr[n-1])) n--;
				ent.title.set_string(ptr, n);
				m_list.add_item(ent);
				ptr+=end;
				if (*ptr == '\n') ptr++;
			}
		}
		return true;
	}
	virtual bool open_path_internal(file::ptr p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints) {
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();//our input does not support retagging.
		p_abort.check();
		m_input.release();

		input_raw_thbgm * ptr = new service_impl_t<input_raw_thbgm>();
		ptr->set_bgmtable(m_list);
		ptr->set_title(m_title);
		ptr->open(p_filehint,p_path,p_reason,p_abort);
		m_input = ptr;
		switch_input(m_input);
		return true;
	}
	virtual void open_decoding_internal(t_uint32 subsong, t_uint32 flags, abort_callback & p_abort) {
		loop_type_impl_singleinput_base::open_decoding_internal(subsong,flags,p_abort);
		get_point_list(subsong,m_points,p_abort);
		switch_points(m_points);
	}
	virtual void get_point_list(t_uint32 p_subsong, loop_event_point_list & p_list, abort_callback & p_abort) {
		p_list.remove_all();
		if (p_subsong > 0) {
			file_info_impl info;
			get_input()->get_info(p_subsong, info, p_abort);
			loop_event_point_simple * point = new service_impl_t<loop_event_point_simple>();
			t_uint64 start = info.info_get_int("loop_start");
			t_uint64 length = info.info_get_int("loop_length");
			point->from = start + length;
			point->to = start;
			p_list.add_item(point);
		}
	}
	virtual void get_info(t_uint32 p_subsong, file_info & p_info,abort_callback & p_abort) {
		get_input()->get_info(p_subsong, p_info, p_abort);
		loop_event_point_list points;
		get_point_list(p_subsong,points,p_abort);
		get_info_for_points(p_info, points, get_info_prefix(), get_sample_rate());
	}
};

static loop_type_factory_t<loop_type_thbgm> g_loop_type_thbgm;
