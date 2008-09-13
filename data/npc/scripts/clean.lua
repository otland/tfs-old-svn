local CLEAN_FREQUENCE = 3600 * 2 -- seconds, x2 because onThink is executed every 500ms

local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)
local cleanTimer = CLEAN_FREQUENCE

function onCreatureAppear(cid)			npcHandler:onCreatureAppear(cid)			end
function onCreatureDisappear(cid)		npcHandler:onCreatureDisappear(cid)			end
function onCreatureSay(cid, type, msg)	npcHandler:onCreatureSay(cid, type, msg)	end
function onThink()
	cleanTimer = cleanTimer - 2
	if cleanTimer == 300 then
		doBroadcastMessage("Game map cleaning in 5 minutes.");
	elseif cleanTimer == 180 then
		doBroadcastMessage("Game map cleaning in 3 minutes.");
	elseif cleanTimer == 60 then
		doBroadcastMessage("Game map cleaning in 1 minute, please pick up your items!");
	elseif cleanTimer == 0 then
		local count = cleanMap()
		doBroadcastMessage("Game map cleaned, collected " .. count .. " items. Next clean in 1 hour.");
		cleanTimer = CLEAN_FREQUENCE
	end

	npcHandler:onThink()
end

npcHandler:addModule(FocusModule:new())