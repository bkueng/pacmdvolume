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

#include <sstream>

#include "pa_manager.h"

PADeviceInfo::PADeviceInfo(const pa_sink_info& sink_info)
	: name(sink_info.name ? sink_info.name : ""), index(sink_info.index)
	, description(sink_info.description ? sink_info.description : "")
	, sample_spec(sink_info.sample_spec), channel_map(sink_info.channel_map)
	, owner_module(sink_info.owner_module), volume(sink_info.volume), mute(sink_info.mute)
	, monitor_index(sink_info.monitor_source), monitor_name(sink_info.monitor_source_name ? sink_info.monitor_source_name : "")
	, latency(sink_info.latency), driver(sink_info.driver ? sink_info.driver : ""), flags_sink(sink_info.flags)
	, configured_latency(sink_info.configured_latency), base_volume(sink_info.base_volume)
	, state_sink(sink_info.state), n_volume_steps(sink_info.n_volume_steps), card(sink_info.card)
	, dev_type(PADev_sink) {
	
}

PADeviceInfo::PADeviceInfo(const pa_source_info& source_info)
	: name(source_info.name ? source_info.name : ""), index(source_info.index)
	, description(source_info.description ? source_info.description : "")
	, sample_spec(source_info.sample_spec), channel_map(source_info.channel_map)
	, owner_module(source_info.owner_module), volume(source_info.volume), mute(source_info.mute)
	, monitor_index(source_info.monitor_of_sink), monitor_name(source_info.monitor_of_sink_name ? source_info.monitor_of_sink_name : "")
	, latency(source_info.latency), driver(source_info.driver ? source_info.driver : ""), flags_source(source_info.flags)
	, configured_latency(source_info.configured_latency), base_volume(source_info.base_volume)
	, state_source(source_info.state), n_volume_steps(source_info.n_volume_steps), card(source_info.card)
	, dev_type(PADev_source) {
	
}

string PADeviceInfo::Info() const {
	ostringstream ret;
	ret << "card " << card << " (idx = " << index << ") " << State() << "\nname: " << name << "\n" << description
			<< "\nmute: " << mute << "\n";
	for(uint8_t i=0; i<volume.channels; ++i) {
		ret << "channel " << (int)i << ": " << volume.values[i] << " (" 
				<< roundStr((float)volume.values[i]/PA_VOLUME_NORM*100.0, 1) << " %)";
		if(i!=volume.channels-1) ret << "\n";
	}
	
	return(ret.str());
}

string PADeviceInfo::State() const {
	switch(dev_type) {
	case PADev_sink:
		switch(state_sink) {
		case PA_SINK_RUNNING: return("running");
		case PA_SINK_IDLE: return("idle");
		case PA_SINK_SUSPENDED: return("suspended");
		case PA_SINK_INIT: return("init");
		case PA_SINK_UNLINKED: return("unlinked");
		}
		break;
	case PADev_source:
		switch(state_source) {
		case PA_SOURCE_RUNNING: return("running");
		case PA_SOURCE_IDLE: return("idle");
		case PA_SOURCE_SUSPENDED: return("suspended");
		case PA_SOURCE_INIT: return("init");
		case PA_SOURCE_UNLINKED: return("unlinked");
		}
		break;
	}
	return("unknown");
}

PAClientInfo::PAClientInfo(const pa_client_info& info) 
	: index(info.index), name(info.name ? info.name : ""), owner_module(info.owner_module)
	, driver(info.driver ? info.driver : "") {
}

PASinkInputInfo::PASinkInputInfo(const pa_sink_input_info& info)
	: index(info.index), name(info.name ? info.name : ""), owner_module(info.owner_module)
	, client(info.client), sink(info.sink), sample_spec(info.sample_spec), volume(info.volume)
	, buffer_usec(info.buffer_usec), sink_usec(info.sink_usec), driver(info.driver ? info.driver : "")
	, mute(info.mute)
	, sink_obj(NULL), client_obj(NULL) {
}

string PASinkInputInfo::Info() const {
	ostringstream ret;
	ret << "idx " << index << " name: " << name;
	if(client_obj) {
		ret << " (client: " << client_obj->name << ")";
	}
	ret << endl;
	if(sink_obj && sink_obj->name.length()>0) {
		ret << "sink: " << sink_obj->name << " (idx = " << sink_obj->index << ")\n";
	}
	ret << "mute: " << mute << "\n";
	
	for(uint8_t i=0; i<volume.channels; ++i) {
		ret << "channel " << (int)i << ": " << volume.values[i] << " (" 
				<< roundStr((float)volume.values[i]/PA_VOLUME_NORM*100.0, 1) << " %)";
		if(i!=volume.channels-1) ret << "\n";
	}
	
	
	return(ret.str());
}


PACardProfileInfo::PACardProfileInfo(const pa_card_profile_info& profile) 
	: name(profile.name), description(profile.description)
	, n_sinks(profile.n_sinks), n_sources(profile.n_sources)
	, priority(profile.priority) {
	
}


PACardInfo::PACardInfo(const pa_card_info& card) 
	: index(card.index), name(card.name), owner_module(card.owner_module)
	, driver(card.driver) {
	
	active_profile = -1;
	for(uint32_t i=0; i<card.n_profiles; ++i) {
		if(card.active_profile == card.profiles+i) active_profile = (int)i;
		profiles.push_back(new PACardProfileInfo(card.profiles[i]));
	}
}

PACardInfo::~PACardInfo() {
	for(size_t i=0; i<profiles.size(); ++i) delete(profiles[i]);
}

string PACardInfo::Info(bool with_profiles) const {
	ostringstream ret;
	ret << "idx " << index << " name: " << name << endl;
	/*
	ret << "owner module " << owner_module << endl;
	ret << "driver " << driver << endl;
	*/
	
	if(with_profiles) {
		ret << "profiles:" << endl;
		for(size_t i=0; i<profiles.size(); ++i) {
			if((int)i == active_profile) ret << " * ";
			else ret << "   ";
			ret << i << ": " << profiles[i]->name << " (" 
					<< profiles[i]->description << ")" << endl;
		}
	}
	
	return(ret.str());
}


/*////////////////////////////////////////////////////////////////////////////////////////////////
 ** PulseAudio Callback functions
/*////////////////////////////////////////////////////////////////////////////////////////////////

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata) {
	
	pa_context_state_t state;
	int *pa_ready = (int*)userdata;

	state = pa_context_get_state(c);
	switch  (state) {
	// There are just here for reference
	case PA_CONTEXT_UNCONNECTED:
	case PA_CONTEXT_CONNECTING:
	case PA_CONTEXT_AUTHORIZING:
	case PA_CONTEXT_SETTING_NAME:
	default:
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		*pa_ready = 2;
		break;
	case PA_CONTEXT_READY:
		*pa_ready = 1;
		break;
	}
}

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata) {
	
	pa_dev_list *pa_device_list = (pa_dev_list*) userdata;

	// If eol is set to a positive number, you're at the end of the list
	if (eol > 0) {
		return;
	}
	PADeviceInfo* device=new PADeviceInfo(*l);
	(*pa_device_list)[l->index]=device;
	
}

// See above.  This callback is pretty much identical to the previous
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata) {
	
	pa_dev_list *pa_device_list = (pa_dev_list*)userdata;

	if (eol > 0) {
		return;
	}
	
	PADeviceInfo* device=new PADeviceInfo(*l);
	(*pa_device_list)[l->index]=device;
}


void pa_client_cb(pa_context * context, const pa_client_info *i, int eol, void *userdata) {

	pa_client_list* client_list = (pa_client_list*) userdata;
	
	if (eol < 0) {
		if (pa_context_errno(context) == PA_ERR_NOENTITY)
			return;

		LOG(ERROR, "client callback error");
		return;
	}

	if (eol > 0) {
		return;
	}

	PAClientInfo* client=new PAClientInfo(*i);
	(*client_list)[i->index]=client;
}

void pa_sink_input_cb(pa_context * context, const pa_sink_input_info *i, int eol, void *userdata) {
	
	pa_sink_input_list* sink_inputs = (pa_sink_input_list*) userdata;
	
	if (eol < 0) {
		if (pa_context_errno(context) == PA_ERR_NOENTITY)
			return;
		
		LOG(ERROR, "sink input callback error");
		return;
	}

	if (eol > 0) { //end of list
		return;
	}

	PASinkInputInfo* input=new PASinkInputInfo(*i);
	(*sink_inputs)[i->index]=input;
}

//volume change callback
void pa_context_success_cb(pa_context* c, int success, void *userdata) {
	
	int* ready=(int*)userdata;
	if(success==1) *ready=1;
	else *ready=-1;
}

//cards
void pa_card_cb(pa_context *, const pa_card_info *i, int eol, void *userdata) {
	
	pa_card_list* cards = static_cast<pa_card_list*>(userdata);

    if (eol < 0) {
        return;
    }

    if (eol > 0) {
    	//all cards done
        return;
    }
    
    PACardInfo* card = new PACardInfo(*i);
    (*cards)[i->index]=card;
}




/*////////////////////////////////////////////////////////////////////////////////////////////////
 ** class PAManager
/*////////////////////////////////////////////////////////////////////////////////////////////////

PAManager::PAManager() : m_pa_context(NULL), m_pa_mainloop(NULL) {
	
}

PAManager::~PAManager() {
	DeInit();
}


void PAManager::Init() {
	ASSERT_THROW(m_pa_context==NULL, EALREADY_INITIALIZED);
	
	/* init pa context */
	pa_mainloop_api *pa_mlapi;
	// Create a mainloop API and connection to the default server
	ASSERT_THROW_e(m_pa_mainloop = pa_mainloop_new(), EASSERT, "Failed to create PulseAudio MainLoop");
	ASSERT_THROW_e(pa_mlapi = pa_mainloop_get_api(m_pa_mainloop), EASSERT, "Failed to create PulseAudio MainLoop");
	ASSERT_THROW_e(m_pa_context = pa_context_new(pa_mlapi, APP_NAME), EDEVICE, "Failed to get a PulseAudio Context Object");
	
	// This function connects to the pulse server
	pa_context_connect(m_pa_context, NULL, (pa_context_flags_t)0, NULL);
	

	InitPAInfo();
	
}

void PAManager::DeInit() {
	
	/* disconnect */
	if(m_pa_context) {
		pa_context_disconnect(m_pa_context);
		pa_context_unref(m_pa_context);
		m_pa_context=NULL;
		
	}
	if(m_pa_mainloop) {
		pa_mainloop_free(m_pa_mainloop);
		m_pa_mainloop=NULL;
	}
	
	/* delete devices */
	
	for(pa_dev_list::iterator iter=m_sinks.begin(); iter!=m_sinks.end(); ++iter) {
		delete(iter->second);
	}
	m_sinks.clear();
	
	for(pa_dev_list::iterator iter=m_sources.begin(); iter!=m_sources.end(); ++iter) {
		delete(iter->second);
	}
	m_sources.clear();
	
	for(pa_client_list::iterator iter=m_clients.begin(); iter!=m_clients.end(); ++iter) {
		delete(iter->second);
	}
	m_clients.clear();
	
	for(pa_sink_input_list::iterator iter=m_sink_inputs.begin(); iter!=m_sink_inputs.end(); ++iter) {
		delete(iter->second);
	}
	m_sink_inputs.clear();
	
	for(pa_card_list::iterator iter = m_cards.begin(); iter!=m_cards.end(); ++iter) {
		delete(iter->second);
	}
	m_cards.clear();
	
}


void PAManager::InitPAInfo() {

	
	int state = 0;
	m_pa_ready=0;
	pa_operation *pa_op=NULL;
	bool bDone=false;
	

	// This function defines a callback so the server will tell us it's state.
	// Our callback will wait for the state to be ready.  The callback will
	// modify the variable to 1 so we know when we have a connection and it's
	// ready.
	// If there's an error, the callback will set pa_ready to 2
	pa_context_set_state_callback(m_pa_context, pa_state_cb, &m_pa_ready);

	
	while(!bDone) {
		// We can't do anything until PA is ready, so just iterate the mainloop
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
		
		if (m_pa_ready == 2) { // We couldn't get a connection to the server, so exit out
			THROW_s(EDEVICE, "Failed to connect to PulseAudio Server");
		} else if(m_pa_ready!=0) {
			// At this point, we're connected to the server and ready to make
			// requests
			switch (state) {
			// State 0: we haven't done anything yet
			case 0:
				// This sends an operation to the server.  pa_sinklist_info is
				// our callback function and a pointer to our devicelist will
				// be passed to the callback The operation ID is stored in the
				// pa_op variable
				pa_op = pa_context_get_sink_info_list(m_pa_context,
						pa_sinklist_cb,
						&m_sinks
				);
				ASSERT_THROW_e(pa_op, EGENERAL, "pa_context_get_sink_info_list failed");
				
				// Update state for next iteration through the loop
				state++;
				break;
			case 1:
				// Now we wait for our operation to complete.  When it's
				// complete our pa_output_devicelist is filled out, and we move
				// along to the next state
				if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
					pa_operation_unref(pa_op);
	
					// Now we perform another operation to get the source
					// (input device) list just like before.  This time we pass
					// a pointer to our input structure
					pa_op = pa_context_get_source_info_list(m_pa_context,
							pa_sourcelist_cb,
							&m_sources
					);
					ASSERT_THROW_e(pa_op, EGENERAL, "pa_context_get_source_info_list failed");
					
					// Update the state so we know what to do next
					state++;
				}
				break;
			case 2:
				if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
					pa_operation_unref(pa_op);
					
					//get client info
		            pa_op=pa_context_get_client_info_list(m_pa_context, pa_client_cb, &m_clients);
		            ASSERT_THROW_e(pa_op, EGENERAL, "pa_context_get_client_info_list failed");
		            
		            ++state;
				}
				break;
			case 3:
				if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
					pa_operation_unref(pa_op);
					
					//get the applications that use a sink
		            pa_op = pa_context_get_sink_input_info_list(m_pa_context, pa_sink_input_cb, &m_sink_inputs);
		            ASSERT_THROW_e(pa_op, EGENERAL, "pa_context_get_sink_input_info_list failed");
		            
		            ++state;
				}
				break;
			case 4:
				if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
					pa_operation_unref(pa_op);
					
					//get the cards
		            pa_op = pa_context_get_card_info_list(m_pa_context, pa_card_cb, &m_cards);
		            ASSERT_THROW_e(pa_op, EGENERAL, "pa_context_get_card_info_list failed");
		            
		            ++state;
				}
				break;
			case 5:
				if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
					// Now we're done, clean up
					pa_operation_unref(pa_op);
					bDone=true;
				}
				break;
			default:
				// We should never see this state
				THROW_s(EGENERAL, "Unexpected state. This MUST NOT happen!");
			}
		}
	}
	
	//fill the objects of sink inputs
	for(pa_sink_input_list::iterator iter=m_sink_inputs.begin(); iter!=m_sink_inputs.end(); ++iter) {
		iter->second->client_obj=Client(iter->second->client);
		iter->second->sink_obj=Sink(iter->second->sink);
	}
}


PADeviceInfo* PAManager::Sink(uint32_t idx) {
	pa_dev_list::iterator iter = m_sinks.find(idx);
	if(iter==m_sinks.end()) return(NULL);
	return(iter->second);
}

PADeviceInfo* PAManager::Source(uint32_t idx) {
	pa_dev_list::iterator iter = m_sources.find(idx);
	if(iter==m_sources.end()) return(NULL);
	return(iter->second);
}

PAClientInfo* PAManager::Client(uint32_t idx) {
	pa_client_list::iterator iter = m_clients.find(idx);
	if(iter==m_clients.end()) return(NULL);
	return(iter->second);
}

PASinkInputInfo* PAManager::SinkInput(uint32_t idx) {
	pa_sink_input_list::iterator iter = m_sink_inputs.find(idx);
	if(iter==m_sink_inputs.end()) return(NULL);
	return(iter->second);
}

PACardInfo* PAManager::Card(uint32_t card_idx) {
	pa_card_list::iterator iter = m_cards.find(card_idx);
	if(iter==m_cards.end()) return(NULL);
	return(iter->second);
}

string PAManager::cardProfileName(PACardInfo* card, const string& profile) {
	ASSERT_THROW(card, EINVALID_PARAMETER);
	
	int profile_idx;
	if(isInteger(profile, &profile_idx)) {
		ASSERT_THROW_e(profile_idx >= 0 && profile_idx < (int)card->profiles.size()
				, EINVALID_PARAMETER, "profile index out of bound");
		
		return(card->profiles[profile_idx]->name);
	} else {
		for(size_t i=0; i<card->profiles.size(); ++i) {
			if(toLower(card->profiles[i]->name).find(toLower(profile))
					!=string::npos) return(card->profiles[i]->name);
		}
	}
	THROW_s(EINVALID_PARAMETER, "profile %s not found", profile.c_str());
}

void PAManager::setCardProfile(PACardInfo* card, const string& profile_name) {
	ASSERT_THROW(card, EINVALID_PARAMETER);
	m_pa_ready=0;
	pa_operation* op = pa_context_set_card_profile_by_index(m_pa_context
			, card->index, profile_name.c_str(), pa_context_success_cb, &m_pa_ready);
	
	ASSERT_THROW_e(op, EGENERAL, "pa_context_set_card_profile_by_index failed");
	
    pa_operation_unref(op);
    
	while(m_pa_ready==0) {
		//wait for the callback
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
	}
}


uint32_t PAManager::getSink(const string& name) {
	for(pa_dev_list::iterator iter=m_sinks.begin(); iter!=m_sinks.end(); ++iter) {
		if(toLower(iter->second->name).find(toLower(name))!=string::npos) return(iter->first);
	}
	return((uint32_t)-1);
}

bool PAManager::getSinks(const string& name, vector<uint32_t>& sinks) {
	sinks.clear();
	
	for(pa_dev_list::iterator iter=m_sinks.begin(); iter!=m_sinks.end(); ++iter) {
		if(toLower(iter->second->name).find(toLower(name))!=string::npos) sinks.push_back(iter->first);
	}
	
	return(!sinks.empty());
}

uint32_t PAManager::getSource(const string& name) {
	for(pa_dev_list::iterator iter=m_sources.begin(); iter!=m_sources.end(); ++iter) {
		if(toLower(iter->second->name).find(toLower(name))!=string::npos) return(iter->first);
	}
	return((uint32_t)-1);
}

bool PAManager::getSources(const string& name, vector<uint32_t>& sources) {
	sources.clear();
	
	for(pa_dev_list::iterator iter=m_sources.begin(); iter!=m_sources.end(); ++iter) {
		if(toLower(iter->second->name).find(toLower(name))!=string::npos) sources.push_back(iter->first);
	}
	
	return(!sources.empty());
}

uint32_t PAManager::getSinkInputFromClient(const string& client_name) {
	PAClientInfo* client;
	for(pa_sink_input_list::iterator iter=m_sink_inputs.begin(); iter!=m_sink_inputs.end(); ++iter) {
		if((client=iter->second->client_obj)) {
			if(cmpInsensitive(client->name, client_name)) return(iter->first);
		}
	}
	return((uint32_t)-1);
}


bool PAManager::getSinkInputsFromClient(const string& client_name, vector<uint32_t>& inputs) {
	inputs.clear();
	
	PAClientInfo* client;
	for(pa_sink_input_list::iterator iter=m_sink_inputs.begin(); iter!=m_sink_inputs.end(); ++iter) {
		if((client=iter->second->client_obj)) {
			if(cmpInsensitive(client->name, client_name)) inputs.push_back(iter->first);
		}
	}
	
	return(!inputs.empty());
}

bool PAManager::getCard(const string& name, vector<uint32_t>& cards) const {
	
	cards.clear();
	
	for(pa_card_list::const_iterator iter=m_cards.begin(); 
			iter!=m_cards.end(); ++iter) {
		if(toLower(iter->second->name).find(toLower(name))!=string::npos) cards.push_back(iter->first);
	}
	
	return(!cards.empty());
}

void PAManager::setSinkVolume(uint32_t idx, const string& volume, const vector<int>* channel_list) {
	PADeviceInfo* sink=Sink(idx);
	ASSERT_THROW_e(sink, EINVALID_PARAMETER, "sink with idx %i not found", idx);
	ASSERT_THROW(volume.length()>0, EINVALID_PARAMETER);
	
	if(volume=="mute") {
		setSinkMute(idx, 1);
	} else if(volume=="unmute") {
		setSinkMute(idx, 0);
	} else {
		applyVolumeChannel(volume, sink->volume, channel_list);
	}
	
	setSinkVolume(idx, sink->volume);
	
}


void PAManager::setSinkVolume(uint32_t idx, const pa_cvolume& volume) {
	
	pa_operation* o;
	m_pa_ready=0;
	
	if(!(o = pa_context_set_sink_volume_by_index(m_pa_context, idx, &volume
			, pa_context_success_cb, &m_pa_ready))) {
		LOG(ERROR, "pa_context_set_sink_volume_by_index() for index %i failed", idx);
	} else {
		pa_operation_unref(o);
	}
	
	while(m_pa_ready==0) {
		//wait for the callback
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
	}
	
}

void PAManager::setSinkMute(uint32_t idx, int mute) {
	
	pa_operation* o;
	m_pa_ready=0;
	
	if(!(o = pa_context_set_sink_mute_by_index(m_pa_context, idx, mute, pa_context_success_cb, &m_pa_ready))) {
		LOG(ERROR, "pa_context_set_sink_mute_by_index() for index %i failed", idx);
	} else {
		pa_operation_unref(o);
	}
	
	while(m_pa_ready==0) {
		//wait for the callback
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
	}
}


void PAManager::setSourceVolume(uint32_t idx, const string& volume, const vector<int>* channel_list) {
	PADeviceInfo* source=Source(idx);
	ASSERT_THROW_e(source, EINVALID_PARAMETER, "source with idx %i not found", idx);
	ASSERT_THROW(volume.length()>0, EINVALID_PARAMETER);
	
	if(volume=="mute") {
		setSourceMute(idx, 1);
	} else if(volume=="unmute") {
		setSourceMute(idx, 0);
	} else {
		applyVolumeChannel(volume, source->volume, channel_list);
	}
	
	setSourceVolume(idx, source->volume);
}

void PAManager::setSourceVolume(uint32_t idx, const pa_cvolume& volume) {
	
	pa_operation* o;
	m_pa_ready=0;
	
	if(!(o = pa_context_set_source_volume_by_index(m_pa_context, idx, &volume, pa_context_success_cb, &m_pa_ready))) {
		LOG(ERROR, "pa_context_set_source_volume_by_index() for index %i failed", idx);
	} else {
		pa_operation_unref(o);
	}
	
	while(m_pa_ready==0) {
		//wait for the callback
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
	}
}

void PAManager::setSourceMute(uint32_t idx, int mute) {
	
	pa_operation* o;
	m_pa_ready=0;
	
	if(!(o = pa_context_set_source_mute_by_index(m_pa_context, idx, mute, pa_context_success_cb, &m_pa_ready))) {
		LOG(ERROR, "pa_context_set_source_mute_by_index() for index %i failed", idx);
	} else {
		pa_operation_unref(o);
	}
	
	while(m_pa_ready==0) {
		//wait for the callback
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
	}
}


void PAManager::setSinkInputVolume(uint32_t idx, const string& volume, const vector<int>* channel_list) {
	PASinkInputInfo* sink_input=SinkInput(idx);
	ASSERT_THROW_e(sink_input, EINVALID_PARAMETER, "playback with idx %i not found", idx);
	ASSERT_THROW(volume.length()>0, EINVALID_PARAMETER);
	
	if(volume=="mute") {
		setSinkInputMute(idx, 1);
	} else if(volume=="unmute") {
		setSinkInputMute(idx, 0);
	} else {
		applyVolumeChannel(volume, sink_input->volume, channel_list);
	}
	
	setSinkInputVolume(idx, sink_input->volume);
}

void PAManager::setSinkInputVolume(uint32_t idx, const pa_cvolume& volume) {
	pa_operation* o;
	m_pa_ready=0;
	
	if(!(o = pa_context_set_sink_input_volume(m_pa_context, idx, &volume, pa_context_success_cb, &m_pa_ready))) {
		LOG(ERROR, "pa_context_set_sink_input_volume() for index %i failed", idx);
	} else {
		pa_operation_unref(o);
	}
	
	while(m_pa_ready==0) {
		//wait for the callback
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
	}
}

void PAManager::setSinkInputMute(uint32_t idx, int mute) {
	
	pa_operation* o;
	m_pa_ready=0;
	
	if(!(o = pa_context_set_sink_input_mute(m_pa_context, idx, mute, pa_context_success_cb, &m_pa_ready))) {
		LOG(ERROR, "pa_context_set_sink_input_mute() for index %i failed", idx);
	} else {
		pa_operation_unref(o);
	}
	
	while(m_pa_ready==0) {
		//wait for the callback
		pa_mainloop_iterate(m_pa_mainloop, 1, NULL);
	}
}

void PAManager::applyVolume(const string& volume, pa_volume_t& value) {
	string vol=trim(volume);
	ASSERT_THROW_e(vol.length()>0, EINVALID_PARAMETER, "invalid volume format");
	bool bPercentage=false;
	if(vol[vol.length()-1]=='%') {
		bPercentage=true;
		vol=vol.substr(0, vol.length()-1);
		ASSERT_THROW_e(vol.length()>0, EINVALID_PARAMETER, "invalid volume format");
	}
	float fchange;
	
	if(sscanf(vol.c_str(), "+%f", &fchange)==1) {
		if(bPercentage) {
			value=min(MAX_VOLUME, (pa_volume_t)((float)value+(float)MAX_VOLUME*fchange/100.0));
		} else {
			value=min(MAX_VOLUME, value+(pa_volume_t)fchange);
		}
	} else if(sscanf(vol.c_str(), "-%f", &fchange)==1) {
		if(bPercentage) {
			value=(pa_volume_t)max(0.0, ((float)value-(float)MAX_VOLUME*fchange/100.0));
		} else {
			if((pa_volume_t)fchange >= value) value=0;
			else value=value-(pa_volume_t)fchange;
		}
	} else if(sscanf(vol.c_str(), "*%f", &fchange)==1) {
		if(value==0) value=10;
		if(bPercentage) {
			value=min(MAX_VOLUME, (pa_volume_t)((float)value*fchange/100.0));
		} else {
			value=min(MAX_VOLUME, (pa_volume_t)((float)value*fchange));
		}
	} else if(sscanf(vol.c_str(), "/%f", &fchange)==1) {
		if(value==0) value=10;
		if(fchange==0.0) fchange=1.0;
		if(bPercentage) {
			value=min(MAX_VOLUME, (pa_volume_t)((float)value/fchange*100.0));
		} else {
			value=min(MAX_VOLUME, (pa_volume_t)((float)value/fchange));
		}
	} else if(sscanf(vol.c_str(), "%f", &fchange)==1) {
		if(bPercentage) {
			value=min(MAX_VOLUME, ((pa_volume_t)fchange*MAX_VOLUME)/100);
		} else {
			value=min(MAX_VOLUME, (pa_volume_t)fchange);
		}
	} else {
		THROW_s(EINVALID_PARAMETER, "volume format %s not recogniced", volume.c_str());
	}
	
}

void PAManager::applyVolumeChannel(const string& volume, pa_cvolume& vol, const vector<int>* channel_list) {
	if(channel_list && channel_list->size()>0) {
		for(size_t i=0; i<channel_list->size(); ++i) {
			if((*channel_list)[i]>=0 && (*channel_list)[i]<vol.channels) {
				applyVolume(volume, vol.values[(*channel_list)[i]]);
			} else {
				LOG(ERROR, "device does not have a channel with index %i", (*channel_list)[i]);
			}
		}
	} else { //all channels
		for(uint8_t i=0; i<vol.channels; ++i) {
			applyVolume(volume, vol.values[i]);
		}
	}
}







