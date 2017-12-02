/*
	Score Restorer
	Copyright (C) 2015-2017 Vladimir "allejo" Jimenez

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <map>

#include "bzfsAPI.h"

const char* PLUGIN_NAME = "Score Restorer";

const int MAJOR = 1;
const int MINOR = 0;
const int REV = 0;
const int BUILD = 2;

class ScoreRestorer : public bz_Plugin, public bz_CustomSlashCommandHandler
{
public:
	virtual const char* Name ();
	virtual void Init (const char* config);
	virtual void Event (bz_EventData *eventData);
	virtual void Cleanup (void);

	virtual bool SlashCommand (int playerID, bz_ApiString, bz_ApiString, bz_APIStringList*);

	struct PlayerRecord
	{
		std::string callsign;
		std::string ipAddress;

		int wins;
		int losses;
		int teamKills;

		double expireTime;
	};

	std::map<std::string, PlayerRecord> savedRecords;
	std::string bzdb_saveTimeVar;
};

BZ_PLUGIN(ScoreRestorer)

const char* ScoreRestorer::Name (void)
{
	static const char* pluginBuild;

	if (!pluginBuild)
	{
		pluginBuild = bz_format("%s %d.%d.%d (%d)", PLUGIN_NAME, MAJOR, MINOR, REV, BUILD);
	}

	return pluginBuild;
}

void ScoreRestorer::Init (const char* /*commandLine*/)
{
	Register(bz_ePlayerJoinEvent);
	Register(bz_ePlayerPartEvent);

	bz_registerCustomSlashCommand("score", this);

	bzdb_saveTimeVar = "_scoreSaveTime";

	if (!bz_BZDBItemExists(bzdb_saveTimeVar.c_str()))
	{
		bz_setBZDBDouble(bzdb_saveTimeVar.c_str(), 120);
	}

	bz_setDefaultBZDBDouble(bzdb_saveTimeVar.c_str(), 120);
}

void ScoreRestorer::Cleanup (void)
{
	Flush();

	bz_removeCustomSlashCommand("score");
}

void ScoreRestorer::Event (bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_ePlayerJoinEvent: // This event is called each time a player joins the game
		{
			bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;

			// We'll store callsigns in lower case for sanity's sake
			std::string playerCallsign = bz_tolower(joinData->record->callsign.c_str());

			// Check if we have a record
			if (savedRecords.count(playerCallsign))
			{
				PlayerRecord &record = savedRecords[playerCallsign];

				// Verify their identity by checking the IP address
				if (joinData->record->ipAddress == record.ipAddress)
				{
					// Make sure the record hasn't expired
					if (record.expireTime + bz_getBZDBDouble(bzdb_saveTimeVar.c_str()) > bz_getCurrentTime())
					{
						if (joinData->record->team == eObservers)
						{
							bz_sendTextMessage(BZ_SERVER, joinData->playerID, "Your score record will be saved while you are in observer mode.");
						}
						else
						{
							bz_setPlayerWins(joinData->playerID, record.wins);
							bz_setPlayerLosses(joinData->playerID, record.losses);
							bz_setPlayerTKs(joinData->playerID, record.teamKills);

							bz_sendTextMessage(BZ_SERVER, joinData->playerID, "Your score has been restored.");

							savedRecords.erase(playerCallsign);
						}
					}
					else
					{
						savedRecords.erase(playerCallsign);
					}
				}
			}
		}
		break;

		case bz_ePlayerPartEvent: // This event is called each time a player leaves a game
		{
			bz_PlayerJoinPartEventData_V1* partData = (bz_PlayerJoinPartEventData_V1*)eventData;

			// We'll store callsigns in lower case for sanity's sake
			std::string playerCallsign = bz_tolower(partData->record->callsign.c_str());

			if (!savedRecords.count(playerCallsign))
			{
				PlayerRecord newRecord;

				newRecord.callsign = playerCallsign;
				newRecord.ipAddress = partData->record->ipAddress;
				newRecord.wins = partData->record->wins;
				newRecord.losses = partData->record->losses;
				newRecord.teamKills = partData->record->teamKills;
				newRecord.expireTime = bz_getCurrentTime();

				savedRecords[playerCallsign] = newRecord;
			}
		}
		break;

		default: break;
	}
}


bool ScoreRestorer::SlashCommand(int playerID, bz_ApiString command, bz_ApiString /*message*/, bz_APIStringList *params)
{
	if (command == "score")
	{
		if (!bz_hasPerm(playerID, "setall"))
		{
			bz_sendTextMessagef(BZ_SERVER, playerID, "Unknown command [%s]", command.c_str());
			return true;
		}

		std::string action = params->get(0).c_str();

		if (action == "set" || action == "increase")
		{
			std::string change = params->get(1).c_str();
			std::string victim = params->get(2).c_str();
			std::string points = params->get(3).c_str();

			bz_BasePlayerRecord *target = bz_getPlayerBySlotOrCallsign(victim.c_str());

			if (!target)
			{
				bz_sendTextMessagef(BZ_SERVER, playerID, "player %s not found", victim.c_str());
				return true;
			}

			int pointChange = 0;

			std::string targetPhrase = "";
			pointChange = std::stoi(points);

			if (change == "wins" || change == "kills")
			{
				if (action == "set")
				{
					bz_setPlayerWins(target->playerID, pointChange);
				}
				else if (action == "increase")
				{
					bz_incrementPlayerWins(target->playerID, pointChange);
				}

				targetPhrase = "kill";
			}
			else if (change == "losses" || change == "deaths")
			{
				if (action == "set")
				{
					bz_setPlayerLosses(target->playerID, pointChange);
				}
				else if (action == "increase")
				{
					bz_incrementPlayerLosses(target->playerID, pointChange);
				}

				targetPhrase = "death";
			}
			else if (change == "teamkills" || change == "tks")
			{
				if (action == "set")
				{
					bz_setPlayerTKs(target->playerID, pointChange);
				}
				else if (action == "increase")
				{
					bz_incrementPlayerTKs(target->playerID, pointChange);
				}

				targetPhrase = "teamkill";
			}

			bz_sendTextMessagef(BZ_SERVER, target->playerID, "%s has %s your %s count %s %d",
								bz_getPlayerCallsign(playerID),
								(action == "set") ? "set" : "increased",
								targetPhrase.c_str(),
								(action == "set") ? "to" : "by",
								pointChange);

			bz_sendTextMessagef(BZ_SERVER, playerID, "You have %s %s's %s count %s %d",
								(action == "set") ? "set" : "increased",
								bz_getPlayerCallsign(target->playerID),
								targetPhrase.c_str(),
								(action == "set") ? "to" : "by",
								pointChange);

			bz_sendTextMessagef(BZ_SERVER, eAdministrators, "%s has %s %s's %s count %s %d",
								bz_getPlayerCallsign(playerID),
								(action == "set") ? "set" : "increased",
								bz_getPlayerCallsign(target->playerID),
								targetPhrase.c_str(),
								(action == "set") ? "to" : "by",
								pointChange);

			bz_freePlayerRecord(target);
		}
		else if (action == "clear")
		{
			std::string victim = params->get(1).c_str();

			bz_BasePlayerRecord *target = bz_getPlayerBySlotOrCallsign(victim.c_str());

			if (!target)
			{
				bz_sendTextMessagef(BZ_SERVER, playerID, "player %s not found", victim.c_str());
				return true;
			}

			bz_resetPlayerScore(target->playerID);
			bz_sendTextMessagef(BZ_SERVER, target->playerID, "Your score has been cleared by %s", bz_getPlayerCallsign(playerID));
			bz_sendTextMessagef(BZ_SERVER, playerID, "You have cleared %s's score", bz_getPlayerCallsign(target->playerID));
			bz_sendTextMessagef(BZ_SERVER, eAdministrators, "%s has cleared %s's score", bz_getPlayerCallsign(playerID), bz_getPlayerCallsign(target->playerID));

			bz_freePlayerRecord(target);
		}
	}

	return true;
}
