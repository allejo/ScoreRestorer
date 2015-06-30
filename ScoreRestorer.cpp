/*
Score Restorer
    Copyright (C) 2015 Vladimir "allejo" Jimenez

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

// Define plugin name
const std::string PLUGIN_NAME = "Score Restorer";

// Define plugin version numbering
const int MAJOR = 1;
const int MINOR = 0;
const int REV = 0;
const int BUILD = 1;


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

    double bzdb_saveTime;
};

BZ_PLUGIN(ScoreRestorer)

const char* UselessMine::Name (void)
{
    static std::string pluginBuild = "";

    if (!pluginBuild.size())
    {
        std::ostringstream pluginBuildStream;

        pluginBuildStream << PLUGIN_NAME << " " << MAJOR << "." << MINOR << "." << REV << " (" << BUILD << ")";
        pluginBuild = pluginBuildStream.str();
    }

    return pluginBuild.c_str();
}

void ScoreRestorer::Init (const char* commandLine)
{
    // Register our events with Register()
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPartEvent);

    // Register our custom slash commands
    bz_registerCustomSlashCommand("score", this);

    // Registr our custom BZDB variables
    bzdb_saveTime = bztk_registerCustomIntBZDB("_scoreSaveTime", 15);
}

void ScoreRestorer::Cleanup (void)
{
    Flush(); // Clean up all the events

    // Clean up our custom slash commands
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
            std::string playerCallsign = bz_tolower(joinData->record->callsign);

            // Check if we have a record
            if (savedRecords.count(playerCallsign))
            {
                PlayerRecord &record = savedRecords[playerCallsign];

                // Verify their identity by checking the IP address
                if (record->ipAddress == joinData->record->ipAddress)
                {
                    // Make sure the record hasn't expired
                    if (playerRecord->expireTime + bzdb_saveTime > bz_getCurrentTime())
                    {
                        if (joinData->record->team == eObservers)
                        {
                            bz_sendTextMessage(BZ_SERVER, joinData->playerID, "Your score record will be saved while you are in observer mode.");
                        }
                        else
                        {
                            bz_setPlayerWins(joinData->playerID, playerRecord->wins);
                            bz_setPlayerLosses(joinData->playerID, playerRecord->losses);
                            bz_setPlayerTKs(joinData->playerID, playerRecord->teamKills);

                            bz_sendTextMessage(BZ_SERVER, joinData->playerID, "Your score has been restored.");

                            savedRecords.erase(playerCallsign);
                        }
                    }
                    else // The record has expired, so let's erase it
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
            std::string playerCallsign = bz_tolower(joinData->record->callsign);

            // Check if we have a record and they just left the observer team in order to extend
            // the expiration time
            if (savedRecords.count(playerCallsign) && partData->record->team == eObservers)
            {
                PlayerRecord &record = savedRecords[playerCallsign];

                record->expireTime = bz_getCurrentTime();
            }
            else // They don't have a record, so create one
            {
                PlayerRecord newRecord;

                newRecord->callsign = playerCallsign;
                newRecord->ipAddress = partData->record->ipAddress;
                newRecord->wins = partData->record->wins;
                newRecord->losses = partData->record->losses;
                newRecord->teamKills = partData->record->teamKills;

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

            bz_BasePlayerRecord* pr = bz_getPlayerBySlotOrCallsign(victim);

            if (pr)
            {
                int pointChange = 0;

                try
                {
                    std::string targetPhrase = "";
                    pointChange = std::stoi(points);

                    if (change == "wins" || change == "kills")
                    {
                        if (action == "set")
                        {
                            bz_setPlayerWins(pr->playerID, pointChange);
                        }
                        else if (action == "increase")
                        {
                            bz_incrementPlayerWins(pr->playerID, pointChange);
                        }

                        targetPhrase = "kill";
                    }
                    else if (change == "losses" || change == "deaths")
                    {
                        if (action == "set")
                        {
                            bz_setPlayerLosses(pr->playerID, pointChange);
                        }
                        else if (action == "increase")
                        {
                            bz_incrementPlayerLosses(pr->playerID, pointChange);
                        }

                        targetPhrase = "death";
                    }
                    else if (change == "teamkills" || change == "tks")
                    {
                        if (action == "set")
                        {
                            bz_setPlayerTKs(pr->playerID, pointChange);
                        }
                        else if (action == "increase")
                        {
                            bz_incrementPlayerTKs(pr->playerID, pointChange);
                        }

                        targetPhrase = "teamkill";
                    }

                    bz_sendTextMessagef(BZ_SERVER, pr->playerID, "%s has %s your %s count %s %d",
                        bz_getPlayerCallsign(playerID).c_str(),
                        (action == "set") ? "set" : "increased",
                        targetPhrase.c_str()
                        (action == "set") ? "to" : "by",
                        pointChange);

                    bz_sendTextMessagef(BZ_SERVER, playerID, "You have %s %s's %s count %s %d",
                        (action == "set") ? "set" : "increased",
                        bz_getPlayerCallsign(pr->playerID).c_str(),
                        targetPhrase.c_str()
                        (action == "set") ? "to" : "by",
                        pointChange);

                    bz_sendTextMessagef(BZ_SERVER, eAdministrators, "%s has %s %s's %s count %s %d",
                        bz_getPlayerCallsign(playerID).c_str(),
                        (action == "set") ? "set" : "increased",
                        bz_getPlayerCallsign(pr->playerID).c_str(),
                        targetPhrase.c_str()
                        (action == "set") ? "to" : "by",
                        pointChange);
                }
                catch(std::exception const & e)
                {
                    bz_sendTextMessagef(BZ_SERVER, playerID, "Invalid score change [%s]", points.c_str());
                }
            }
            else
            {
                bz_sendTextMessagef(BZ_SERVER, playerID, "player %s not found", victim.c_str());
            }

            bz_freePlayerRecord(pr);
        }
        else if (action == "clear")
        {
            bz_BasePlayerRecord* pr = bz_getPlayerBySlotOrCallsign(victim);

            if (pr)
            {
                bz_resetPlayerScore(pr->playerID);
                bz_sendTextMessagef(BZ_SERVER, pr->playerID, "Your score has been cleared by %s", bz_getPlayerCallsign(playerID).c_str());
                bz_sendTextMessagef(BZ_SERVER, playerID, "You have cleared %s's score", bz_getPlayerCallsign(pr->playerID).c_str());
                bz_sendTextMessagef(BZ_SERVER, eAdministrators, "%s has cleared %s's score", bz_getPlayerCallsign(playerID).c_str(), bz_getPlayerCallsign(pr->playerID).c_str());
            }

            bz_freePlayerRecord(pr);
        }

        return true;
    }
}
