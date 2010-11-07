/*
 * Copyright (C) 2010 Beat Küng <beat-kueng@gmx.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef PA_MANAGER_H_
#define PA_MANAGER_H_

#include "global.h"
#include <map>
#include <pulse/pulseaudio.h>


#define MAX_VOLUME PA_VOLUME_NORM //which is 0dB


enum EPADeviceType {
	PADev_sink,
	PADev_source
};

struct PADeviceInfo {
	PADeviceInfo(const pa_sink_info& sink_info);
	PADeviceInfo(const pa_source_info& source_info);
	
	string Info() const;
	string State() const;
	
    const string name;
    uint32_t index;
    const string description;
    pa_sample_spec sample_spec;
    pa_channel_map channel_map;
    uint32_t owner_module;
    pa_cvolume volume;
    int mute;
    uint32_t monitor_index; /* if device is sink this is the connected source, if it's source it's the connected sink */
    const string monitor_name;
    pa_usec_t latency;
    const string driver;
    pa_sink_flags_t flags_sink;
    pa_source_flags_t flags_source;
    pa_usec_t configured_latency;
    pa_volume_t base_volume;
    pa_sink_state_t state_sink;
    pa_source_state_t state_source;
    uint32_t n_volume_steps;
    uint32_t card;
    
    EPADeviceType dev_type;
};

struct PAClientInfo {
	PAClientInfo(const pa_client_info& info);

	uint32_t index;
	const string name;
	uint32_t owner_module;
	const string driver;
};

struct PASinkInputInfo {
	PASinkInputInfo(const pa_sink_input_info& info);
	
	string Info() const;
	
	uint32_t index;
	const string name;
	uint32_t owner_module;
	uint32_t client;
	uint32_t sink;
	pa_sample_spec sample_spec;
	pa_channel_map channel_map;
	pa_cvolume volume;
	pa_usec_t buffer_usec;
	pa_usec_t sink_usec;
	const string driver;
	int mute;
	
	PADeviceInfo* sink_obj;
	PAClientInfo* client_obj;
};

/*////////////////////////////////////////////////////////////////////////////////////////////////
 ** class PAManager
 * connects to pulseaudio, retrieves info and changes values
/*////////////////////////////////////////////////////////////////////////////////////////////////

typedef map<uint32_t, PADeviceInfo*> pa_dev_list;
typedef map<uint32_t, PAClientInfo*> pa_client_list;
typedef map<uint32_t, PASinkInputInfo*> pa_sink_input_list;

class PAManager {
public:
	PAManager();
	~PAManager();
	
	void Init();
	void DeInit();
	
	const pa_dev_list& Sinks() const { return(m_sinks); }
	PADeviceInfo* Sink(uint32_t idx); /* returns NULL if not found */
	const pa_dev_list& Sources() const { return(m_sources); }
	PADeviceInfo* Source(uint32_t idx); /* returns NULL if not found */
	
	const pa_client_list& Clients() const { return(m_clients); }
	PAClientInfo* Client(uint32_t idx); /* returns NULL if not found */
	
	const pa_sink_input_list& SinkInputs() const { return(m_sink_inputs); }
	PASinkInputInfo* SinkInput(uint32_t idx); /* returns NULL if not found */
	
	
	
	/* find a sink/source where name is a substring of the card name. the first found will be returned
	 * , returns -1 if not found */
	uint32_t getSink(const string& name); 
	uint32_t getSource(const string& name);
	uint32_t getSinkInputFromClient(const string& client_name);
	
	/* volume */
	
	/* volume has the format: 
	 * absolute value or with %
	 * + or - for in/decrease
	 * * or / for logarithmical in/decrease
	 */
	void setSinkVolume(uint32_t idx, const string& volume, const vector<int>* channel_list=NULL);
	void setSinkVolume(uint32_t idx, const pa_cvolume& volume);
	
	/* playback volume */
	void setSinkInputVolume(uint32_t idx, const string& volume, const vector<int>* channel_list=NULL);
	void setSinkInputVolume(uint32_t idx, const pa_cvolume& volume);
	
private:
	
	void InitPAInfo();
	
	void applyVolume(const string& volume, pa_volume_t& value);
	
	pa_dev_list m_sinks;
	pa_dev_list m_sources;
	
	pa_client_list m_clients;
	pa_sink_input_list m_sink_inputs; /* these are the connected playback streams */
	
	/* pulseaudio stuff */
	pa_context* m_pa_context;
	pa_mainloop* m_pa_mainloop;
	int m_pa_ready; //used for callback
};







#endif /* PA_MANAGER_H_ */



