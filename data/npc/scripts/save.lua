local SAVE_FREQUENCE = 600 * 2 -- seconds, x2 because onThink is executed every 500ms

local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)
local saveTimer = SAVE_FREQUENCE

function onCreatureAppear(cid)			npcHandler:onCreatureAppear(cid)			end
function onCreatureDisappear(cid)		npcHandler:onCreatureDisappear(cid)			end
function onCreatureSay(cid, type, msg)	npcHandler:onCreatureSay(cid, type, msg)	end
function onThink()
	saveTimer = saveTimer - 2
	if saveTimer == 0 then
		saveServer()
		saveTimer = SAVE_FREQUENCE
	end

	npcHandler:onThink()
end

npcHandler:addModule(FocusModule:new())