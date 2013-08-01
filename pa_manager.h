/*
 * Copyright (C) 2010-2011 Beat Küng <beat-kueng@gmx.net>
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
	
	bool isRunning() { return dev_type==PADev_sink ? state_sink==PA_SINK_RUNNING
	    : /* PADev_source */ state_source==PA_SOURCE_RUNNING; }
	
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


struct PACardProfileInfo {
	PACardProfileInfo(const pa_card_profile_info& profile);
	
    string name;                        /**< Name of this profile */
    string description;                 /**< Description of this profile */
    uint32_t n_sinks;                   /**< Number of sinks this profile would create */
    uint32_t n_sources;                 /**< Number of sources this profile would create */
    uint32_t priority;                  /**< The higher this value is the more useful this profile is as a default */
};

struct PACardInfo {
	PACardInfo(const pa_card_info& card);
	~PACardInfo();
	
	string Info(bool with_profiles=true) const;
	
    uint32_t index;                      /**< Index of this card */
    string name;                         /**< Name of this card */
    uint32_t owner_module;               /**< Index of the owning module, or PA_INVALID_INDEX */
    string driver;                       /**< Driver name */
    vector<PACardProfileInfo*> profiles;         
    int active_profile;                  /**< Pointer to active profile in the array, or -1 */
};

/*////////////////////////////////////////////////////////////////////////////////////////////////
 ** class PAManager
 * connects to pulseaudio, retrieves info and changes values
/*////////////////////////////////////////////////////////////////////////////////////////////////

typedef map<uint32_t, PADeviceInfo*> pa_dev_list;
typedef map<uint32_t, PACardInfo*> pa_card_list; //map key is card index
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
	
	const pa_card_list& Cards() const { return(m_cards); }
	PACardInfo* Card(uint32_t card_idx); /* returns NULL if not found */
	
		/* profile can either be the profile index or (substring of) 
		 * profile name (first matching is returned) */
	string cardProfileName(PACardInfo* card, const string& profile);
	void setCardProfile(PACardInfo* card, const string& profile_name);
	
	
	/* find a sink/source where name is a substring of the card name. the first found will be returned
	 * , returns -1 if not found 
	 * for the ones with bool return: true on success (vector non empty), false if non found (vector empty)*/
	uint32_t getSink(const string& name);
	bool getSinks(const string& name, vector<uint32_t>& sinks);
	bool getRunningSinks(vector<uint32_t>& sinks);
	uint32_t getSource(const string& name);
	bool getSources(const string& name, vector<uint32_t>& sources);
	bool getRunningSources(vector<uint32_t>& sources);
	uint32_t getSinkInputFromClient(const string& client_name);
	bool getSinkInputsFromClient(const string& client_name, vector<uint32_t>& inputs);
	bool getCard(const string& name, vector<uint32_t>& cards) const;
	
	/* volume */
	
	/* volume has the format: 
	 * absolute value or with %
	 * + or - for in/decrease
	 * * or / for logarithmical in/decrease
	 * 
	 * volume can also be mute or unmute
	 */
	void setSinkVolume(uint32_t idx, const string& volume, const vector<int>* channel_list=NULL);
	void setSinkVolume(uint32_t idx, const pa_cvolume& volume);
	void setSinkMute(uint32_t idx, int mute);
	
	void setSourceVolume(uint32_t idx, const string& volume, const vector<int>* channel_list=NULL);
	void setSourceVolume(uint32_t idx, const pa_cvolume& volume);
	void setSourceMute(uint32_t idx, int mute);
	
	/* playback volume */
	void setSinkInputVolume(uint32_t idx, const string& volume, const vector<int>* channel_list=NULL);
	void setSinkInputVolume(uint32_t idx, const pa_cvolume& volume);
	void setSinkInputMute(uint32_t idx, int mute);
	
private:
	
	void InitPAInfo();
	
	void applyVolume(const string& volume, pa_volume_t& value);
	// this will call applyVolume for all chosen channels:
	void applyVolumeChannel(const string& volume, pa_cvolume& vol, const vector<int>* channel_list);
	
	pa_dev_list m_sinks;
	pa_dev_list m_sources;
	
	pa_card_list m_cards;
	
	pa_client_list m_clients;
	pa_sink_input_list m_sink_inputs; /* these are the connected playback streams */
	
	/* pulseaudio stuff */
	pa_context* m_pa_context;
	pa_mainloop* m_pa_mainloop;
	int m_pa_ready; //used for callback
};







#endif /* PA_MANAGER_H_ */



