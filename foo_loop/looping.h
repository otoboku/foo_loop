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

bool open_path_helper(input_decoder::ptr & p_input, file::ptr p_file, const char * path,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints);
bool parse_entity(const char * & ptr,pfc::string8 & name,pfc::string8 & value);
enum {
	merge_filestats_sum = 1,
	merge_filestats_max = 2,
};
t_filestats merge_filestats(const t_filestats & p_src1, const t_filestats & p_src2, int p_merge_type);

void inline combine_audio_chunks(audio_chunk & p_first,const audio_chunk & p_second) {
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

void inline combine_audio_chunks(audio_chunk & p_first,mem_block_container * p_raw_first,const audio_chunk & p_second,const mem_block_container * p_raw_second) {
	if (!p_raw_first != !p_raw_second)
		throw exception_unexpected_audio_format_change();

	combine_audio_chunks(p_first, p_second);

	if (p_raw_first == NULL) return;

	t_size offset = p_raw_first->get_size();
	t_size size = p_raw_second->get_size();
	// combine mem_block
	p_raw_first->set_size(offset + size);
	memcpy(((char*)p_raw_first->get_ptr())+offset,p_raw_second->get_ptr(),size);
}

void inline truncate_chunk(audio_chunk & p_chunk, mem_block_container * p_raw, t_size p_samples) {
	if (p_raw != NULL) p_raw->set_size(MulDiv_Size(p_raw->get_size(), p_samples, p_chunk.get_sample_count()));
	p_chunk.set_sample_count(p_samples);
	p_chunk.set_data_size(p_samples * p_chunk.get_channel_count());
}


class NOVTABLE input_loop_type_base : public service_base {
protected:
	virtual input_decoder::ptr & get_input() = 0;
	virtual input_decoder_v2::ptr & get_input_v2() = 0;
	virtual void set_succ(bool val) = 0;
	virtual bool get_succ() const = 0;
	virtual void set_cur(t_uint64 val) = 0;
	virtual void add_cur(t_uint64 add) = 0;
public:
	virtual t_uint64 get_cur() const = 0;
	virtual t_uint32 get_sample_rate() const = 0;
	virtual bool get_no_looping() const = 0;

	virtual void raw_seek(t_uint64 samples, abort_callback & p_abort) {
		set_succ(true);
		get_input()->seek(audio_math::samples_to_time(samples, get_sample_rate()), p_abort);
		set_cur(samples);
	}

	virtual void raw_seek(double seconds, abort_callback & p_abort) {
		set_succ(true);
		get_input()->seek(seconds, p_abort);
		set_cur(audio_math::time_to_samples(seconds, get_sample_rate()));
	}

	inline void get_one_chunk(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
		if (!get_succ()) return; // already EOF
		if (p_raw != NULL) {
			set_succ(get_input_v2()->run_raw(p_chunk,*p_raw,p_abort));
		} else {
			set_succ(get_input()->run(p_chunk,p_abort));
		}
		add_cur(p_chunk.get_sample_count());
	}

	virtual t_size get_more_chunk(audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort, t_size candidate) {
		t_size samples = 0;
		mem_block_container_impl * tmp_block = NULL;
		while (get_succ() && candidate > samples) {
			audio_chunk_impl_temporary ptmp_chunk;
			t_size newsamples;
			if (p_raw != NULL) {
				if (tmp_block == NULL)
					tmp_block = new mem_block_container_impl();
				else
					tmp_block->reset();
				set_succ(get_input_v2()->run_raw(ptmp_chunk,*tmp_block,p_abort));
			} else {
				set_succ(get_input()->run(ptmp_chunk,p_abort));
			}
			newsamples = ptmp_chunk.get_sample_count();
			if (newsamples)
				combine_audio_chunks(p_chunk, p_raw, ptmp_chunk, tmp_block);
			samples += newsamples;
			add_cur(newsamples);
		}
		if (tmp_block != NULL) delete tmp_block;
		return samples;
	}

	FB2K_MAKE_SERVICE_INTERFACE(input_loop_type_base, service_base);
};

class input_loop_event_point : public service_base {
public:
	//! Get position of this event occured.
	virtual t_uint64 get_position() const = 0;

	//! Get position of this event occured with audio chunk includes specified point.
	virtual t_uint64 get_prepare_position() const = 0;

	//! Get information of this event.
	virtual void get_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) = 0;

	virtual bool has_dynamic_info() const = 0;
	virtual bool set_dynamic_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) = 0;
	virtual bool reset_dynamic_info(file_info & p_info, const char * p_prefix) = 0;

	virtual bool has_dynamic_track_info() const = 0;
	virtual bool set_dynamic_track_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) = 0;
	virtual bool reset_dynamic_track_info(file_info & p_info, const char * p_prefix) = 0;

	//! process this event with specified chunk. return true after seek or switch input, otherwise false.
	virtual bool process(input_loop_type_base::ptr p_input, t_uint64 p_start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) = 0;
	//! process this event. return true after seek or switch input, otherwise false.
	virtual bool process(input_loop_type_base::ptr p_input, abort_callback & p_abort) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(input_loop_event_point, service_base);
};

template<typename t_item1, typename t_item2>
inline int input_loop_event_compare(const t_item1 & p_item1, const t_item2 & p_item2);

template<>
inline int input_loop_event_compare(const input_loop_event_point & p_item1, const input_loop_event_point & p_item2) {
	return pfc::compare_t(p_item1.get_position(), p_item2.get_position());
}

template<>
inline int input_loop_event_compare(const input_loop_event_point::ptr & p_item1, const input_loop_event_point::ptr & p_item2) {
	return pfc::compare_t(p_item1.is_valid() ? p_item1->get_position() : infinite64, p_item2.is_valid() ? p_item2->get_position() : infinite64);
}

template<>
inline int input_loop_event_compare(const t_uint64 & p_item1, const input_loop_event_point::ptr & p_item2) {
	return pfc::compare_t(p_item1, p_item2.is_valid() ? p_item2->get_position() : infinite64);
}

template<>
inline int input_loop_event_compare(const input_loop_event_point::ptr & p_item1, const t_uint64 & p_item2) {
	return pfc::compare_t(p_item1.is_valid() ? p_item1->get_position() : infinite64, p_item2);
}

template<typename t_item1, typename t_item2>
inline int input_loop_event_prepos_compare(const t_item1 & p_item1, const t_item2 & p_item2);

template<>
inline int input_loop_event_prepos_compare(const input_loop_event_point & p_item1, const input_loop_event_point & p_item2) {
	return pfc::compare_t(p_item1.get_prepare_position(), p_item2.get_prepare_position());
}

template<>
inline int input_loop_event_prepos_compare(const input_loop_event_point::ptr & p_item1, const input_loop_event_point::ptr & p_item2) {
	return pfc::compare_t(p_item1.is_valid() ? p_item1->get_prepare_position() : infinite64, p_item2.is_valid() ? p_item2->get_prepare_position() : infinite64);
}

template<>
inline int input_loop_event_prepos_compare(const t_uint64 & p_item1, const input_loop_event_point::ptr & p_item2) {
	return pfc::compare_t(p_item1, p_item2.is_valid() ? p_item2->get_prepare_position() : infinite64);
}

template<>
inline int input_loop_event_prepos_compare(const input_loop_event_point::ptr & p_item1, const t_uint64 & p_item2) {
	return pfc::compare_t(p_item1.is_valid() ? p_item1->get_prepare_position() : infinite64, p_item2);
}

typedef pfc::list_t<input_loop_event_point::ptr, pfc::alloc_fast> input_loop_event_point_list;

class NOVTABLE input_loop_type : public input_loop_type_base {
protected:
	virtual bool is_raw_supported() const = 0;
	virtual const char * get_info_prefix() const = 0;

public:
	virtual bool parse(const char * ptr) = 0;
	//! process specified event with audio chunk and return true after seek or switch input, otherwise false.
	virtual bool process_event(input_loop_event_point::ptr point, t_uint64 start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) = 0;
	//! process specified event and return true after seek or switch input, otherwise false.
	virtual bool process_event(input_loop_event_point::ptr point, abort_callback & p_abort) = 0;
	virtual bool open_path(file::ptr p_filehint,const char * path,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints) = 0;
	virtual void get_info(t_uint32 subsong, file_info & p_info,abort_callback & p_abort) = 0;
	virtual t_filestats get_file_stats(abort_callback & p_abort) = 0;
	virtual void open_decoding(t_uint32 subsong, t_uint32 flags, abort_callback & p_abort) = 0;
	virtual void seek(t_uint64 samples, abort_callback & p_abort) = 0;
	virtual void seek(double seconds, abort_callback & p_abort) = 0;
	virtual bool run(audio_chunk & p_chunk,abort_callback & p_abort) = 0;
	virtual bool run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) = 0;
	virtual void set_info_prefix(const char * p_prefix) = 0;

	virtual void close() = 0;
	inline bool can_seek() {return get_input()->can_seek();}
	virtual void on_idle(abort_callback & p_abort) = 0;
	virtual bool get_dynamic_info(file_info & p_out,double & p_timestamp_delta) = 0;
	virtual bool get_dynamic_info_track(file_info & p_out,double & p_timestamp_delta) = 0;
	virtual void set_logger(event_logger::ptr ptr) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(input_loop_type, input_loop_type_base);
};

class input_loop_event_point_baseimpl : public input_loop_event_point {
public:
	virtual t_uint64 get_prepare_position() const {return infinite64;}

	virtual void get_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) {}

	virtual bool has_dynamic_info() const {return false;}
	virtual bool set_dynamic_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) {return false;}
	virtual bool reset_dynamic_info(file_info & p_info, const char * p_prefix) {return false;}

	virtual bool has_dynamic_track_info() const {return false;}
	virtual bool set_dynamic_track_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) {return false;}
	virtual bool reset_dynamic_track_info(file_info & p_info, const char * p_prefix) {return false;}
};

class input_loop_event_point_simple : public input_loop_event_point_baseimpl {
public:
	input_loop_event_point_simple() : from(0), to(0), maxrepeats(0), repeats(0) {}
	t_uint64 from, to;
	t_size maxrepeats, repeats;
	virtual t_uint64 get_position() const {return from;}
	virtual void get_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) {
		pfc::string8 name;
		t_size prefixlen;
		name << p_prefix;
		prefixlen = name.get_length();

		name.truncate(prefixlen);
		name << "from";
		p_info.info_set(name, format_samples_ex(from, sample_rate));

		name.truncate(prefixlen);
		name << "to";
		p_info.info_set(name, format_samples_ex(to, sample_rate));

		if (maxrepeats) {
			name.truncate(prefixlen);
			name << "maxrepeats";
			p_info.info_set_int(name, maxrepeats);
		}
	}
	virtual bool has_dynamic_info() const {return true;}
	virtual bool set_dynamic_info(file_info & p_info, const char * p_prefix, t_uint32 sample_rate) {
		pfc::string8 name;
		name << p_prefix << "repeats";
		p_info.info_set_int(name, repeats);
		return true;
	}
	virtual bool reset_dynamic_info(file_info & p_info, const char * p_prefix) {
		pfc::string8 name;
		name << p_prefix << "repeats";
		return p_info.info_remove(name);
	}

	virtual bool process(input_loop_type_base::ptr p_input, t_uint64 p_start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
		// this event do not process on no_looping
		if (p_input->get_no_looping()) return false;
		++repeats;
		if (maxrepeats && repeats>=maxrepeats) return false;
		t_size newsamples = pfc::downcast_guarded<t_size>(from - p_start);
		truncate_chunk(p_chunk,p_raw,newsamples);
		p_input->raw_seek(to, p_abort);
		return true;
	}
	virtual bool process(input_loop_type_base::ptr p_input, abort_callback & p_abort) {
		// this event do not process on no_looping
		if (p_input->get_no_looping()) return false;
		++repeats;
		if (maxrepeats && repeats>=maxrepeats) return false;
		p_input->raw_seek(to, p_abort);
		return true;
	}
};

class NOVTABLE input_loop_type_impl_base : public input_loop_type {
private:
	t_uint32 m_sample_rate;
	t_uint64 m_cur;
	bool m_no_looping;
	bool m_succ;
	bool m_raw_support;
	pfc::string8 m_info_prefix;
	input_loop_event_point_list m_cur_points;
	input_loop_event_point_list m_old_points, m_old_points_track;
	pfc::list_permutation_t<input_loop_event_point::ptr> * m_cur_points_by_pos, * m_cur_points_by_prepos;
	pfc::array_t<t_size> m_perm_by_pos, m_perm_by_prepos;
	input_decoder::ptr m_current_input;
	input_decoder_v2::ptr m_current_input_v2;
	t_uint64 m_nextpointpos;
	class dynamic_update_tracker {
	public:
		t_uint64 lastupdate;
		t_uint64 updateperiod;
		dynamic_update_tracker() : lastupdate(0), updateperiod(0) {}
		inline bool check(t_uint64 cur) const {
			return lastupdate >= cur || (lastupdate + updateperiod) < cur;
		}
		inline bool check_and_update(t_uint64 cur) {
			if (check(cur)) {
				lastupdate = cur;
				return true;
			}
			return false;
		}
	};
	dynamic_update_tracker m_dynamic, m_dynamic_track;

	inline t_size get_prepare_length(t_uint64 p_start, t_uint64 p_end, t_size nums) {
		t_size n;
		bsearch_points_by_prepos(p_start, n);
		t_uint64 maxpos = p_end;
		while (n < nums) {
			input_loop_event_point::ptr point = get_points_by_prepos()[n];
			if (point->get_prepare_position() > p_end) break;
			maxpos = pfc::max_t<t_uint64>(maxpos, point->get_position());
			n++;
		}
		return pfc::downcast_guarded<t_size>(maxpos - p_end);
	}

	inline t_uint64 get_prepare_pos(t_uint64 p_pos, t_size nums) {
		t_size n;
		bsearch_points_by_prepos(p_pos, n);
		if (n < nums) {
			return get_points_by_prepos()[n]->get_prepare_position();
		}
		return infinite64;
	}

protected:
	inline bool get_succ() const {return m_succ;}
	inline void set_succ(bool val) {m_succ = val;}
	inline void set_cur(t_uint64 val) {m_cur = val;}
	inline void add_cur(t_uint64 add) {m_cur += add;}
	inline bool is_raw_supported() const {return m_raw_support;}
	virtual double get_dynamic_updateperiod() const {return audio_math::samples_to_time(m_dynamic.updateperiod, m_sample_rate);}
	virtual double get_dynamictrack_updateperiod() const {return audio_math::samples_to_time(m_dynamic_track.updateperiod, m_sample_rate);}
	virtual void set_dynamic_updateperiod(double p_time) {
		m_dynamic.updateperiod = audio_math::time_to_samples(p_time, m_sample_rate);
	}
	virtual void set_dynamictrack_updateperiod(double p_time) {
		m_dynamic_track.updateperiod = audio_math::time_to_samples(p_time, m_sample_rate);
	}
	inline const char * get_info_prefix() const {return m_info_prefix;}

	virtual bool open_path_internal(file::ptr p_filehint,const char * path,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints) = 0;
	virtual void open_decoding_internal(t_uint32 subsong, t_uint32 flags, abort_callback & p_abort) = 0;

	virtual input_decoder::ptr & get_input() {
		if (m_current_input.is_empty()) throw pfc::exception_bug_check_v2();
		return m_current_input;
	}
	virtual input_decoder_v2::ptr & get_input_v2() {
		if (m_current_input_v2.is_empty()) throw pfc::exception_not_implemented();
		return m_current_input_v2;
	}

	virtual void switch_input(input_decoder::ptr p_input) {
		// please specify reopen'd input...
		m_current_input = p_input;
		if (m_current_input_v2.is_valid()) m_current_input_v2.release();
		if (!m_current_input->service_query_t(m_current_input_v2)) m_current_input_v2.release();
		set_succ(true);
	}

	virtual void switch_points(input_loop_event_point_list p_list) {
		m_old_points = m_cur_points;
		m_old_points_track = m_cur_points;
		m_cur_points = p_list;

		m_perm_by_pos.set_size(m_cur_points.get_count());
		order_helper::g_fill(m_perm_by_pos.get_ptr(), m_perm_by_pos.get_size());
		m_cur_points.sort_get_permutation_t(
			input_loop_event_compare<input_loop_event_point::ptr, input_loop_event_point::ptr>,m_perm_by_pos.get_ptr());
		if (m_cur_points_by_pos != NULL) delete m_cur_points_by_pos;
		m_cur_points_by_pos = new pfc::list_permutation_t<input_loop_event_point::ptr>(m_cur_points, m_perm_by_pos.get_ptr(), m_perm_by_pos.get_size());

		m_perm_by_prepos.set_size(m_cur_points.get_count());
		order_helper::g_fill(m_perm_by_prepos.get_ptr(), m_perm_by_prepos.get_size());
		m_cur_points.sort_get_permutation_t(
			input_loop_event_prepos_compare<input_loop_event_point::ptr, input_loop_event_point::ptr>,m_perm_by_prepos.get_ptr());
		if (m_cur_points_by_prepos != NULL) delete m_cur_points_by_prepos;
		m_cur_points_by_prepos = new pfc::list_permutation_t<input_loop_event_point::ptr>(m_cur_points, m_perm_by_prepos.get_ptr(), m_perm_by_prepos.get_size());
	}

	virtual pfc::list_permutation_t<input_loop_event_point::ptr> get_points_by_pos() {
		if (m_cur_points_by_pos == NULL) throw pfc::exception_bug_check_v2();
		return *m_cur_points_by_pos;
	}

	virtual pfc::list_permutation_t<input_loop_event_point::ptr> get_points_by_prepos() {
		if (m_cur_points_by_prepos == NULL) throw pfc::exception_bug_check_v2();
		return *m_cur_points_by_prepos;
	}

	inline input_loop_event_point_list get_points() {
		return m_cur_points;
	}

	virtual t_size bsearch_points_by_pos(t_uint64 pos, t_size & index) {
		return m_cur_points_by_pos->bsearch_t(input_loop_event_compare<input_loop_event_point::ptr, t_uint64>, pos, index);
	}

	virtual t_size bsearch_points_by_prepos(t_uint64 pos, t_size & index) {
		return m_cur_points_by_prepos->bsearch_t(input_loop_event_prepos_compare<input_loop_event_point::ptr, t_uint64>, pos, index);
	}

	inline void set_is_raw_supported(bool val) {m_raw_support = val;}
	inline void set_no_looping(bool val) {m_no_looping = val;}

	virtual t_size get_nearest_point(t_uint64 pos) {
		t_size nums = get_points_by_pos().get_count();
		if (!nums) return infinite_size;
		t_size index;
		bsearch_points_by_pos(pos, index);
		if (index < nums) {
			return index;
		} else {
			return infinite_size;
		}
	}

	inline void do_current_events(abort_callback & p_abort) {
		do_events(get_cur(), p_abort);
	}

	inline void do_events(t_uint64 p_pos, abort_callback & p_abort) {
		do_events(p_pos, p_pos, p_abort);
	}

	virtual void do_events(t_uint64 p_start, t_uint64 p_end, abort_callback & p_abort) {
		t_size nums = get_points_by_pos().get_count();
		t_size n;
		input_loop_event_point::ptr point;
		bsearch_points_by_pos(p_start, n);
		
		while (n < nums) {
			point = get_points_by_pos()[n];
			m_nextpointpos = point->get_position();
			if (m_nextpointpos > p_end) {
				m_nextpointpos = pfc::min_t(m_nextpointpos, get_prepare_pos(p_end, nums));
				return;
			}
			if (process_event(point, p_abort)) {
				// current is updated.
				do_current_events(p_abort);
				return;
			}
			n++;
		}
		m_nextpointpos = infinite64;
	}

	virtual void do_events(t_uint64 p_start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
		t_size nums = get_points_by_pos().get_count();
		t_size n;
		input_loop_event_point::ptr point;
		bsearch_points_by_pos(p_start, n);
		t_uint64 end = p_start + p_chunk.get_sample_count();
		t_size preplen = get_prepare_length(p_start, end, nums);
		if (preplen > 0)
			end += get_more_chunk(p_chunk, p_raw, p_abort, preplen);

		while (n < nums) {
			point = get_points_by_pos()[n];
			m_nextpointpos = point->get_position();
			if (m_nextpointpos > end) {
				m_nextpointpos = pfc::min_t(m_nextpointpos, get_prepare_pos(end, nums));
				return;
			}
			if (process_event(point, p_start, p_chunk, p_raw, p_abort)) {
				// current is updated.
				do_current_events(p_abort);
				return;
			}
			n++;
		}
		m_nextpointpos = infinite64;
	}

	inline void user_seek(double seconds, abort_callback & p_abort) {
		raw_seek(seconds, p_abort);
		do_current_events(p_abort);
	}
	
	inline void user_seek(t_uint64 samples, abort_callback & p_abort) {
		raw_seek(samples, p_abort);
		do_current_events(p_abort);
	}
	
	virtual bool set_dynamic_info(file_info & p_out) {
		t_uint32 sample_rate = get_sample_rate();
		pfc::string8 name;
		name << get_info_prefix() << "current";
		p_out.info_set(name, format_samples_ex(get_cur(), sample_rate));
		name.reset();
		name << get_info_prefix() << "next_event_pos";
		p_out.info_set(name, (m_nextpointpos != infinite64) ? 
			format_samples_ex(m_nextpointpos, sample_rate) : "(nothing or eof)");
		return true;
	}

	virtual bool reset_dynamic_info(file_info & p_out) {
		bool ret = false;
		pfc::string8 name;
		name << get_info_prefix() << "current";
		ret |= p_out.info_remove(name);
		name.reset();
		name << get_info_prefix() << "next_event_pos";
		ret |= p_out.info_remove(name);
		return ret;
	}

	virtual bool set_dynamic_info_track(file_info & p_out) {
		return false;
	}

	virtual bool reset_dynamic_info_track(file_info & p_out) {
		return false;
	}

	virtual bool run_common(audio_chunk & p_chunk,mem_block_container * p_raw,abort_callback & p_abort) {
		t_uint64 start = get_cur();
		get_one_chunk(p_chunk,p_raw,p_abort);
		if (m_nextpointpos <= get_cur()) {
			do_events(start,p_chunk,p_raw,p_abort);
		}
		if (!get_succ()) {
			// try dispatching infinite64 event;
			do_events(infinite64, p_abort);
		}
		return get_succ();
	}

	void get_info_for_points(file_info & p_info, input_loop_event_point_list & points, const char * p_prefix, t_uint32 p_sample_rate) {
		for (t_size n = 0, m = points.get_count(); n < m; ++n ) {
			input_loop_event_point::ptr point = points[n];
			pfc::string8 name;
			name << p_prefix << "point_" << pfc::format_int(n, 2) << "_";
			point->get_info(p_info, name, p_sample_rate);
		}
	}

	class dispatch_dynamic_info {
	public:
		inline static bool point_check(input_loop_event_point::ptr point) {
			return point->has_dynamic_info();
		}
		inline static bool point_set(input_loop_event_point::ptr point, file_info & p_info, const char * p_prefix, t_uint32 p_sample_rate) {
			return point->set_dynamic_info(p_info, p_prefix, p_sample_rate);
		}
		inline static bool point_reset(input_loop_event_point::ptr point, file_info & p_info, const char * p_prefix) {
			return point->reset_dynamic_info(p_info, p_prefix);
		}
		inline static bool parent_get(input_decoder::ptr & parent, file_info & p_out, double & p_timestamp_delta) {
			return parent->get_dynamic_info(p_out, p_timestamp_delta);
		}
		inline static bool check_and_update(input_loop_type_impl_base & impl, t_uint64 cur) {
			return impl.m_dynamic.check_and_update(cur);
		}
		inline static bool self_set(input_loop_type_impl_base & impl, file_info & p_out) {
			return impl.set_dynamic_info(p_out);
		}
		inline static bool self_reset(input_loop_type_impl_base & impl, file_info & p_out) {
			return impl.reset_dynamic_info(p_out);
		}
		inline static input_loop_event_point_list oldlist(input_loop_type_impl_base & impl) {
			return impl.m_old_points;
		}
	};

	class dispatch_dynamic_track_info {
	public:
		inline static bool point_check(input_loop_event_point::ptr point) {
			return point->has_dynamic_track_info();
		}
		inline static bool point_set(input_loop_event_point::ptr point, file_info & p_info, const char * p_prefix, t_uint32 p_sample_rate) {
			return point->set_dynamic_track_info(p_info, p_prefix, p_sample_rate);
		}
		inline static bool point_reset(input_loop_event_point::ptr point, file_info & p_info, const char * p_prefix) {
			return point->reset_dynamic_track_info(p_info, p_prefix);
		}
		inline static bool parent_get(input_decoder::ptr & parent, file_info & p_out, double & p_timestamp_delta) {
			return parent->get_dynamic_info_track(p_out, p_timestamp_delta);
		}
		inline static bool check_and_update(input_loop_type_impl_base & impl, t_uint64 cur) {
			return impl.m_dynamic_track.check_and_update(cur);
		}
		inline static bool self_set(input_loop_type_impl_base & impl, file_info & p_out) {
			return impl.set_dynamic_info_track(p_out);
		}
		inline static bool self_reset(input_loop_type_impl_base & impl, file_info & p_out) {
			return impl.reset_dynamic_info_track(p_out);
		}
		inline static input_loop_event_point_list oldlist(input_loop_type_impl_base & impl) {
			return impl.m_old_points_track;
		}
	};

	template <typename t_dispatcher>
	inline bool get_dynamic_info_t(file_info & p_out,double & p_timestamp_delta) {
		bool ret = t_dispatcher::parent_get(get_input(),p_out,p_timestamp_delta);
		input_loop_event_point_list oldlist = t_dispatcher::oldlist(*this);
		if (oldlist.get_count() != 0) {
			ret |= t_dispatcher::self_reset(*this, p_out);
			for (t_size n = 0, m = get_points().get_count(); n < m; ++n ) {
				input_loop_event_point::ptr point = get_points()[n];
				if (t_dispatcher::point_check(point)) {
					pfc::string8 name;
					name << get_info_prefix() << "point_" << pfc::format_int(n, 2) << "_";
					ret |= t_dispatcher::point_reset(point, p_out, name);
				}
			}
			oldlist.remove_all();
		}
		if (!get_no_looping()) {
			if (t_dispatcher::check_and_update(*this, get_cur())) {
				p_timestamp_delta = !ret ? 0.5 : pfc::min_t<double>(0.5, p_timestamp_delta);
				ret |= t_dispatcher::self_set(*this, p_out);
				t_uint32 sample_rate = get_sample_rate();
				for (t_size n = 0, m = get_points().get_count(); n < m; ++n ) {
					input_loop_event_point::ptr point = get_points()[n];
					if (t_dispatcher::point_check(point)) {
						pfc::string8 name;
						name << get_info_prefix() << "point_" << pfc::format_int(n, 2) << "_";
						ret |= t_dispatcher::point_set(point, p_out, name, sample_rate);
					}
				}
			}
		}
		return ret;
	}

public:
	input_loop_type_impl_base() :
	  m_sample_rate(0), m_cur(0), m_succ(false), m_raw_support(true), 
	  m_cur_points_by_pos(NULL), m_cur_points_by_prepos(NULL), m_info_prefix("loop_") {
	}
	~input_loop_type_impl_base() {
		if (m_cur_points_by_pos != NULL) delete m_cur_points_by_pos;
		if (m_cur_points_by_prepos != NULL) delete m_cur_points_by_prepos;
	}

	t_uint64 virtual get_cur() const {return m_cur;}
	t_uint32 virtual get_sample_rate() const {return m_sample_rate;}
	virtual bool get_no_looping() const {return m_no_looping;}
	virtual void set_info_prefix(const char *p_prefix) {m_info_prefix = p_prefix;}

	virtual bool open_path(file::ptr p_filehint,const char * path,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints) {
		bool ret = open_path_internal(p_filehint,path,p_abort,p_from_redirect,p_skip_hints);
		if (ret) {
			file_info_impl p_info;
			get_input()->get_info(0, p_info, p_abort);
			m_sample_rate = pfc::downcast_guarded<t_uint32>(p_info.info_get_int("samplerate"));
		}
		return ret;
	}

	void virtual open_decoding(t_uint32 subsong, t_uint32 flags, abort_callback & p_abort) {
		m_no_looping = (flags & input_flag_simpledecode) != 0;
		set_dynamic_updateperiod(0.5);
		set_dynamictrack_updateperiod(1.0);
		open_decoding_internal(subsong, flags, p_abort);
		set_cur(0);
		set_succ(true);
		do_current_events(p_abort);
	}

	virtual bool process_event(input_loop_event_point::ptr point, t_uint64 p_start, audio_chunk & p_chunk, mem_block_container * p_raw, abort_callback & p_abort) {
		return point->process(this, p_start, p_chunk, p_raw, p_abort);
	}

	virtual bool process_event(input_loop_event_point::ptr point, abort_callback & p_abort) {
		return point->process(this, p_abort);
	}


	virtual bool run(audio_chunk & p_chunk,abort_callback & p_abort) {
		return run_common(p_chunk,NULL,p_abort);
	}

	virtual bool run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
		if (!get_no_looping() && !is_raw_supported()) throw pfc::exception_not_implemented();
		return run_common(p_chunk,&p_raw,p_abort);
	}

	virtual void seek(double p_seconds,abort_callback & p_abort) {
		user_seek(p_seconds,p_abort);
	}

	virtual void seek(t_uint64 p_samples,abort_callback & p_abort) {
		user_seek(p_samples,p_abort);
	}

	// other input_decoder methods
	bool virtual get_dynamic_info(file_info & p_out,double & p_timestamp_delta) {
		return get_dynamic_info_t<dispatch_dynamic_info>(p_out,p_timestamp_delta);
	}

	bool virtual get_dynamic_info_track(file_info & p_out,double & p_timestamp_delta) {
		return get_dynamic_info_t<dispatch_dynamic_track_info>(p_out,p_timestamp_delta);
	}
	void virtual set_logger(event_logger::ptr ptr) {get_input_v2()->set_logger(ptr);}
};

class input_loop_type_impl_singlefile_base : public input_loop_type_impl_base {
protected:
	void virtual open_decoding_internal(t_uint32 subsong, t_uint32 flags, abort_callback & p_abort) {
		get_input()->initialize(subsong, flags, p_abort);
	}

public:
	void virtual get_info(t_uint32 subsong, file_info & p_info,abort_callback & p_abort) {
		get_input()->get_info(subsong, p_info, p_abort);
	}

	virtual t_filestats get_file_stats(abort_callback & p_abort) {
		return get_input()->get_file_stats(p_abort);
	}
	void virtual close() {get_input().release();}
	void virtual on_idle(abort_callback & p_abort) {get_input()->on_idle(p_abort);}
};

class input_loop_type_entry : public service_base {
public:
	virtual const char * get_name() const = 0;
	virtual const char * get_short_name() const = 0;
	virtual bool is_our_type(const char * type) const = 0;
	virtual bool is_explicit() const = 0;
	virtual input_loop_type::ptr instantiate() const = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(input_loop_type_entry);
};

template<typename t_instance_impl>
class input_loop_type_impl_t : public input_loop_type_entry {
public:
	const char * get_name() const {return t_instance_impl::g_get_name();}
	const char * get_short_name() const {return t_instance_impl::g_get_short_name();}
	bool is_our_type(const char * type) const {return t_instance_impl::g_is_our_type(type);}
	bool is_explicit() const {return t_instance_impl::g_is_explicit();}
	input_loop_type::ptr instantiate() const {return new service_impl_t<t_instance_impl>();}
};

template<typename t_instance_impl> class input_loop_type_factory_t :
	public service_factory_single_t<input_loop_type_impl_t<t_instance_impl> > {};


class input_loop_type_none : public input_loop_type_impl_singlefile_base
{
private:
	input_decoder::ptr m_input;
	input_loop_event_point_list m_points;
public:
	static const char * g_get_name() {return "nothing";}
	static const char * g_get_short_name() {return "none";}
	static bool g_is_our_type(const char * type) {return !pfc::stringCompareCaseInsensitive(type, "none");}
	static bool g_is_explicit() {return true;}
	virtual bool parse(const char * ptr) {return true;}
	virtual bool open_path_internal(file::ptr p_filehint,const char * path,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints) {
		open_path_helper(m_input, p_filehint, path, p_abort, p_from_redirect, p_skip_hints);
		//m_points.remove_all();
		switch_input(m_input);
		switch_points(m_points);
		return true;
	}
};

static input_loop_type_factory_t<input_loop_type_none> g_input_loop_type_none;

class NOVTABLE input_loop_base
{
public:
	void open(file::ptr p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		open_internal(p_filehint, p_path, p_reason, p_abort);
		PFC_ASSERT(m_looptype.is_valid());
		PFC_ASSERT(m_loopentry.is_valid());
		m_looptype->set_info_prefix(m_info_prefix);
	}

	void get_info(file_info & p_info,abort_callback & p_abort) {
		m_looptype->get_info(0,p_info,p_abort);
		pfc::string8 name;
		pfc::string8 buf;
		name << m_info_prefix << "type";
		buf << m_loopentry->get_short_name() << " [" << m_loopentry->get_name() << "]";
		p_info.info_set(name, buf);
		name.reset();
		if (!m_loopcontent.is_empty()) {
			name << m_info_prefix << "content";
			p_info.info_set(name, m_loopcontent);
		}
	}

	t_filestats get_file_stats(abort_callback & p_abort) {
		if (m_loopfile.is_valid())
			return merge_filestats(
				m_loopfile->get_stats(p_abort),
				m_looptype->get_file_stats(p_abort),
				merge_filestats_sum);
		else
			return m_looptype->get_file_stats(p_abort);
	}

	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		m_looptype->open_decoding(0,p_flags,p_abort);
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		return m_looptype->run(p_chunk,p_abort);
	}

	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
		return m_looptype->run_raw(p_chunk,p_raw,p_abort);
	}

	void decode_seek(double p_seconds,abort_callback & p_abort) {
		m_looptype->seek(p_seconds,p_abort);
	}

	bool decode_can_seek() {return m_looptype->can_seek();}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
		return m_looptype->get_dynamic_info(p_out,p_timestamp_delta);
	}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
		return m_looptype->get_dynamic_info_track(p_out, p_timestamp_delta);
	}

	void decode_on_idle(abort_callback & p_abort) {m_looptype->on_idle(p_abort);}

	void retag(const file_info & p_info,abort_callback & p_abort) {throw exception_io_unsupported_format();}

	void set_logger(event_logger::ptr ptr) {m_looptype->set_logger(ptr);}

protected:
	input_loop_base(const char * p_info_prefix) : m_info_prefix(p_info_prefix) {}
	virtual void open_internal(file::ptr p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) = 0;
	//static bool g_is_our_content_type(const char * p_content_type);
	//static bool g_is_our_path(const char * p_path,const char * p_extension);
	file::ptr m_loopfile;
	pfc::string8 m_path;
	input_loop_type_entry::ptr m_loopentry;
	input_loop_type::ptr m_looptype;
	pfc::string8 m_loopcontent;
	pfc::string8 m_info_prefix;
};
