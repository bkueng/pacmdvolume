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

#include "main_class.h"
#include "version.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

/*////////////////////////////////////////////////////////////////////////////////////////////////
 ** class CMain
/*////////////////////////////////////////////////////////////////////////////////////////////////


CMain::CMain() : m_parameters(NULL), m_cl_parse_result(Parse_none_found) {
	
}

CMain::~CMain() {
	SAVE_DEL(m_parameters);
}



void CMain::init(int argc, char *argv[]) {
	parseCommandLine(argc, argv);
	
}

void CMain::parseCommandLine(int argc, char *argv[]) {
	
	SAVE_DEL(m_parameters);
	m_parameters=new CCommandLineParser(argc, argv);
	
	//init known arguments
	m_parameters->addSwitch("help", 'h');
	m_parameters->addSwitch("version");
	m_parameters->addSwitch("verbose", 'v');
	
	m_parameters->addParam("card", 'c');
	m_parameters->addParam("card-name", 'C');
	m_parameters->addSwitch("cards-running");
	
	m_parameters->addTask("list", 'l');
	m_parameters->addTask("list-cards", ' ');
	m_parameters->addTask("list-sink", ' ');
	m_parameters->addTask("list-source", ' ');
	m_parameters->addTask("list-playback", ' ');
	
	m_parameters->addParam("set-profile", ' ');
	
	m_parameters->addParam("set-volume", 's');
	m_parameters->addParam("channels", 'n');
	
	m_parameters->addParam("set-source-volume", ' ');
	m_parameters->addParam("set-playback-volume", 'p');
	m_parameters->addParam("index", 'i');
	m_parameters->addParam("client-name", 'I');
	m_parameters->addSwitch("non-corked");
	
	
	m_cl_parse_result=m_parameters->parse();
	
}

void CMain::printHelp() {
	printf("Usage:\n"
		" " APP_NAME " [-v] [<card_selection>]   --list\n"
		" " APP_NAME " [-v] [<card_selection>]   -s <volume> [-n <channels>]\n"
		" " APP_NAME " [-v] [-i <idx> or -I <c>] -p <volume> [-n <channels>]\n"
		" " APP_NAME " [-v]  <card_selection>    --set-profile <profile>\n"
		" " APP_NAME " --version\n"
		"\n"
		"  -l, --list                      list all cards, sinks, sources and playbacks\n"
		"      --list-cards                list cards with profiles\n"
		"      --list-sink                 list sinks\n"
		"      --list-source               list sources\n"
		"      --list-playback             list playback\n"
		"\n"
		"      --set-profile <profile>     set active card profile\n"
		"                                  profile is index or (substring of) the name\n"
		"\n"
		"  -s, --set-volume <volume>       set sink volume\n"
		"                                  <volume> format:\n"
		"                                    absolute or with %%\n"
		"                                    + or - for increase/decrease\n"
		"                                    * or / for logarithmical increase/decrease\n"
		"                                    example: -s '*1.1'\n"
		"                                    mute or unmute\n"
		"      --set-source-volume <volume>\n"
		"                                  set source volume\n"
		"  -p, --set-playback-volume <volume>\n"
		"                                  set playback volume\n"
		"     -i, --index <index>          playback index\n"
		"     -I, --client-name <name>     select playback client name (or substring)\n"
		"     -n, --channels <channels>    specify channels\n"
		"                                  (comma-separated list with channel indexes)\n"
		"     --non-corked                 select non-corked (playing) streams\n"
		
		"\n"
		"  <card_selection>                select a card. one of the following:\n"
		"     -c, --card <idx>             specify card index\n"
		"     -C, --card-name <name>       specify card name\n"
		"                                  (can also be a substring of the name)\n"
		"         --cards-running          select all cards that are running (ie not idle)\n"
		"\n"
		"  -v, --verbose                   print debug messages\n"
		"  -h, --help                      print this message\n"
		"  --version                       print the version\n"
		"\n"
		"Examples:\n"
		" " APP_NAME " --set-volume 20%% -C aureon -n 0,1\n"
		"                                  set the output volume of channels 0 and 1 of\n"
		"                                  card aureon to 20%%\n"
		" " APP_NAME " --set-volume /1.2 -c 0\n"
		"                                  decrease volume of card 0 by 20%%\n"
		" " APP_NAME " --set-playback-volume +10%% -I rhythmbox\n"
		"                                  increase the volume of client rhythmbox by 20%%\n"
		" " APP_NAME " --set-source-volume mute -c 1\n"
		"                                  mute the output with card index 1\n"
		);
}


void CMain::exec() {
	
	ASSERT_THROW(m_parameters, ENOT_INITIALIZED);
	
	switch(m_cl_parse_result) {
	case Parse_none_found:
		printHelp();
		break;
	case Parse_unknown_command:
		wrongUsage("Unknown command: %s", m_parameters->getUnknownCommand().c_str());
		break;
	case Parse_success:
		if(m_parameters->getSwitch("help")) {
			printHelp();
		} else if(m_parameters->getSwitch("version")) {
			printVersion();
		} else {
			processArgs();
		}
		break;
	}
}

void CMain::printVersion() {
	printf("%s\n", getAppVersion().toStr().c_str());
}

void CMain::wrongUsage(const char* fmt, ...) {
	
	printHelp();
	
	printf("\n ");
	
	va_list args;
	va_start (args, fmt);
	vprintf(fmt, args);
	va_end (args);
	
	printf("\n");
	
}


void CMain::processArgs() {
	
	if(m_parameters->getSwitch("verbose")) CLog::getInstance().setConsoleLevel(DEBUG);
	
	/* connect to pulseaudio */
	m_pa_manager.Init();
	
	/* get card */
	uint32_t sink_card_idx=-1; //-2: card is not found, -1: use all, 0: use vector
	vector<uint32_t> sink_card_indexes;
	uint32_t source_card_idx=-1;
	vector<uint32_t> source_card_indexes;
	uint32_t card_idx=-1;
	vector<uint32_t> card_indexes;
	
	
	string card_val;
	if(m_parameters->getParam("card", card_val)) {
		int t_card_idx;
		if(sscanf(card_val.c_str(), "%i", &t_card_idx)==1) {
			if(m_pa_manager.Sink(t_card_idx)) {
				sink_card_indexes.push_back(t_card_idx);
				sink_card_idx=0;
			} else {
				sink_card_idx=-2;
				LOG(DEBUG, "sink with card idx %i not found", t_card_idx);
			}
			if(m_pa_manager.Source(t_card_idx)) {
				source_card_indexes.push_back(t_card_idx);
				source_card_idx=0;
			} else {
				source_card_idx=-2;
				LOG(DEBUG, "source with card idx %i not found", t_card_idx);
			}
			if(m_pa_manager.Card(t_card_idx)) {
				card_indexes.push_back(t_card_idx);
				card_idx=0;
			} else {
				card_idx=-2;
				LOG(DEBUG, "card idx %i not found", t_card_idx);
			}
		} else {
			THROW_s(EINVALID_PARAMETER, "Failed to parse card idx %s", card_val.c_str());
		}
	} else if(m_parameters->getParam("card-name", card_val)) {
		if(m_pa_manager.getSinks(card_val, sink_card_indexes)) {
			sink_card_idx=0;
		} else {
			sink_card_idx=-2;
			LOG(DEBUG, "sink with name %s not found", card_val.c_str());
		}
		if(m_pa_manager.getSources(card_val, source_card_indexes)) {
			source_card_idx=0;
		} else {
			source_card_idx=-2;
			LOG(DEBUG, "source with name %s not found", card_val.c_str());
		}
		if(m_pa_manager.getCard(card_val, card_indexes)) {
			card_idx=0;
		} else {
			card_idx=-2;
			LOG(DEBUG, "card with name %s not found", card_val.c_str());
		}
	} else if(m_parameters->getSwitch("cards-running")) {
		if(m_pa_manager.getRunningSinks(sink_card_indexes)) {
			sink_card_idx = 0;
		} else {
			sink_card_idx = -2;
			LOG(DEBUG, "no running sink found");
		}
		if(m_pa_manager.getRunningSources(source_card_indexes)) {
			source_card_idx = 0;
		} else {
			source_card_idx = -2;
			LOG(DEBUG, "no running source found");
		}
	}
	
	
	/* list devices */
	bool bPrint_sinks=false;
	bool bPrint_sources=false;
	bool bPrint_playbacks=false;
	bool bPrint_cards=false;
	int print_multiple=0;
	
	if(m_parameters->setTask("list")->bGiven) {
		bPrint_cards=bPrint_sinks=bPrint_sources=bPrint_playbacks=true;
		print_multiple=4;
	} else {
		if(m_parameters->setTask("list-cards")->bGiven) {
			bPrint_cards=true;
			++print_multiple;
		}
		if(m_parameters->setTask("list-sink")->bGiven) {
			bPrint_sinks=true;
			++print_multiple;
		}
		if(m_parameters->setTask("list-source")->bGiven) {
			bPrint_sources=true;
			++print_multiple;
		}
		if(m_parameters->setTask("list-playback")->bGiven) {
			bPrint_playbacks=true;
			++print_multiple;
		}
	}
	
	if(print_multiple>1 && bPrint_cards) cout << "cards:" << endl << endl;
	if(bPrint_cards) {
		if(card_idx==(uint32_t)-2) {
			THROW_s(EINVALID_PARAMETER, "specified card not found");
		} else if(card_idx==(uint32_t)-1) {
			for(pa_card_list::const_iterator iter=m_pa_manager.Cards().begin(); 
					iter!=m_pa_manager.Cards().end(); ++iter) {
				cout << iter->second->Info() << endl << endl;
			}
		} else {
			for(size_t i=0; i<card_indexes.size(); ++i)
				cout << m_pa_manager.Card(card_indexes[i])->Info() << endl << endl;
		}
	}
	
	if(print_multiple>1 && bPrint_sinks) cout << "sinks:" << endl << endl;
	if(bPrint_sinks) {
		if(sink_card_idx==(uint32_t)-2) {
			THROW_s(EINVALID_PARAMETER, "specified sink not found");
		} else if(sink_card_idx==(uint32_t)-1) {
			for(pa_dev_list::const_iterator iter=m_pa_manager.Sinks().begin(); iter!=m_pa_manager.Sinks().end(); ++iter) {
				cout << iter->second->Info() << endl << endl;
			}
		} else {
			for(size_t i=0; i<sink_card_indexes.size(); ++i)
				cout << m_pa_manager.Sink(sink_card_indexes[i])->Info() << endl << endl;
		}
	}
	
	if(print_multiple>1 && bPrint_sources) cout << "sources:" << endl << endl;
	if(bPrint_sources) {
		if(source_card_idx==(uint32_t)-2) {
			THROW_s(EINVALID_PARAMETER, "specified source not found");
		} else if(source_card_idx==(uint32_t)-1) {
			for(pa_dev_list::const_iterator iter=m_pa_manager.Sources().begin(); iter!=m_pa_manager.Sources().end(); ++iter) {
				cout << iter->second->Info() << endl << endl;
			}
		} else {
			for(size_t i=0; i<source_card_indexes.size(); ++i)
				cout << m_pa_manager.Source(source_card_indexes[i])->Info() << endl << endl;
		}
	}
	
	if(print_multiple>1 && bPrint_playbacks) cout << "playback:" << endl << endl;
	if(bPrint_playbacks) {
		if(sink_card_idx==(uint32_t)-2) {
			THROW_s(EINVALID_PARAMETER, "specified source not found");
		} else {
			for(pa_sink_input_list::const_iterator iter=m_pa_manager.SinkInputs().begin(); iter!=m_pa_manager.SinkInputs().end(); ++iter) {
				if(sink_card_idx==(uint32_t)-1) {
					cout << iter->second->Info() << endl << endl;
				} else {
					for(size_t i=0; i<sink_card_indexes.size(); ++i) {
						if(iter->second->sink == sink_card_indexes[i]) {
							cout << iter->second->Info() << endl << endl;
						}
					}
				}
			}
		}
	}
	
	/* set active profile */
	string new_profile;
	if(m_parameters->getParam("set-profile", new_profile)) {
		ASSERT_THROW_e(card_idx!=(uint32_t)-2, EINVALID_PARAMETER, "specified card not found");
		ASSERT_THROW_e(card_idx!=(uint32_t)-1, EINVALID_PARAMETER, "no card specified");
		for(size_t i=0; i<card_indexes.size(); ++i) {
			PACardInfo* card = m_pa_manager.Card(card_indexes[i]);
			if(card) {
				string profile_name = m_pa_manager.cardProfileName(card, new_profile);
				m_pa_manager.setCardProfile(card, profile_name);
			} else {
				THROW_s(EINVALID_PARAMETER, "card with index %i not found", card_indexes[i]);
			}
		}
	}
	
	/* parse channels */
	vector<int> channels;
	string channel_str;
	if(m_parameters->getParam("channels", channel_str)) parseIntList(channel_str, channels);
	
	
	/* change volume */
	
	string vol_change;
	if(m_parameters->getParam("set-volume", vol_change)) {
		
		/* change sink volume */
		if(sink_card_idx==(uint32_t)-2) {
			THROW_s(EINVALID_PARAMETER, "specified sink not found");
		} else if(sink_card_idx==(uint32_t)-1) {
			for(pa_dev_list::const_iterator iter=m_pa_manager.Sinks().begin(); iter!=m_pa_manager.Sinks().end(); ++iter) {
				m_pa_manager.setSinkVolume(iter->first, vol_change, &channels);
			}
		} else {
			for(size_t i=0; i<sink_card_indexes.size(); ++i) {
				m_pa_manager.setSinkVolume(sink_card_indexes[i], vol_change, &channels);
			}
		}
	}
	
	if(m_parameters->getParam("set-source-volume", vol_change)) {
		
		/* change source volume */
		if(source_card_idx==(uint32_t)-2) {
			THROW_s(EINVALID_PARAMETER, "specified source not found");
		} else if(source_card_idx==(uint32_t)-1) {
			for(pa_dev_list::const_iterator iter=m_pa_manager.Sources().begin(); iter!=m_pa_manager.Sources().end(); ++iter) {
				m_pa_manager.setSourceVolume(iter->first, vol_change, &channels);
			}
		} else {
			for(size_t i=0; i<source_card_indexes.size(); ++i) {
				m_pa_manager.setSourceVolume(source_card_indexes[i], vol_change, &channels);
			}
		}
	}
	
	vector<uint32_t> playback_indexes;
	uint32_t playback_idx;
	string s;
	const bool non_corked = m_parameters->getSwitch("non-corked");
	if(m_parameters->getParam("index", s)) {
		if(sscanf(s.c_str(), "%i", &playback_idx)!=1) {
			LOG(WARN, "failed to parse specified index %s", s.c_str());
		} else {
			playback_indexes.push_back(playback_idx);
		}
	} else if(m_parameters->getParam("client-name", s)) {
		m_pa_manager.getSinkInputsFromClient(s, non_corked, playback_indexes);
		ASSERT_THROW_e(playback_indexes.size()!=0, EINVALID_PARAMETER, "client name %s not found", s.c_str());
	} else if (non_corked) {
		m_pa_manager.getNonCorkedSinkInputs(playback_indexes);
		ASSERT_THROW_e(playback_indexes.size()!=0, EINVALID_PARAMETER, "no non-corked clients found");
	}
	
	if(m_parameters->getParam("set-playback-volume", vol_change)) {
		
		/* change playback volume */
		if(playback_indexes.empty()) {
			for(pa_dev_list::const_iterator iter=m_pa_manager.Sinks().begin(); iter!=m_pa_manager.Sinks().end(); ++iter) {
				m_pa_manager.setSinkInputVolume(iter->first, vol_change, &channels);
			}
		} else {
			for(size_t i=0; i<playback_indexes.size(); ++i) {
				m_pa_manager.setSinkInputVolume(playback_indexes[i], vol_change, &channels);
			}
		}
	}
	
	
}

void CMain::parseIntList(const string& str, vector<int>& v) {
	string s=str+",";
	int val;
	while(s.length()>0) {
		if(sscanf(s.c_str(), "%i", &val)==1) v.push_back(val);
		s=s.substr(s.find(',')+1);
	}
}










