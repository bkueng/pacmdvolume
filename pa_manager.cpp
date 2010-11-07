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
	

	InitDevices();
	
	
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
}


void PAManager::InitDevices() {

	// We'll need these state variables to keep track of our requests
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
					// Update the state so we know what to do next
					state++;
				}
				break;
			case 2:
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
}


const PADeviceInfo* PAManager::Sink(uint32_t idx) {
	pa_dev_list::iterator iter = m_sinks.find(idx);
	if(iter==m_sinks.end()) return(NULL);
	return(iter->second);
}

const PADeviceInfo* PAManager::Source(uint32_t idx) {
	pa_dev_list::iterator iter = m_sources.find(idx);
	if(iter==m_sources.end()) return(NULL);
	return(iter->second);
}


uint32_t PAManager::getSink(const string& name) {
	for(pa_dev_list::iterator iter=m_sinks.begin(); iter!=m_sinks.end(); ++iter) {
		if(toLower(iter->second->name).find(toLower(name))!=string::npos) return(iter->first);
	}
	return((uint32_t)-1);
}

uint32_t PAManager::getSource(const string& name) {
	for(pa_dev_list::iterator iter=m_sources.begin(); iter!=m_sources.end(); ++iter) {
		if(toLower(iter->second->name).find(toLower(name))!=string::npos) return(iter->first);
	}
	return((uint32_t)-1);
}








