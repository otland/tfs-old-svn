local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)

function onCreatureAppear(cid) npcHandler:onCreatureAppear(cid) end
function onCreatureDisappear(cid) npcHandler:onCreatureDisappear(cid) end
function onCreatureSay(cid, type, msg) npcHandler:onCreatureSay(cid, type, msg) end
function onThink() npcHandler:onThink() end

function buyBlessing(cid, message, keywords, parameters, node)
	if(not npcHandler:isFocused(cid)) then
		return false
	end

	if getPlayerBlessing(cid, parameters.blessing) then
		npcHandler:say("A god has already blessed you with this blessing.", cid)
	elseif isPremium(cid) == TRUE then
		if doPlayerRemoveMoney(cid, 10000) == TRUE then
			doPlayerAddBlessing(cid, parameters.blessing)
			npcHandler:say("You have been blessed by one of the five gods!", cid)
		else
			npcHandler:say("You don't have enough money.", cid)
		end
	else
		npcHandler:say("You need a premium account to buy blessings.", cid)
	end

	keywordHandler:moveUp(1)
	return true
end

local node1 = keywordHandler:addKeyword({'first bless'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you want to buy the first blessing for 10000 gold coins?'})
	node1:addChildKeyword({'yes'}, buyBlessing, {blessing = 1})
	node1:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Then not.'})

local node2 = keywordHandler:addKeyword({'second bless'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you want to buy the second blessing for 10000 gold coins?'})
	node2:addChildKeyword({'yes'}, buyBlessing, {blessing = 2})
	node2:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Then not.'})

local node3 = keywordHandler:addKeyword({'third bless'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you want to buy the third blessing for 10000 gold coins?'})
	node3:addChildKeyword({'yes'}, buyBlessing, {blessing = 3})
	node3:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Then not.'})

local node4 = keywordHandler:addKeyword({'fourth bless'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you want to buy the fourth blessing for 10000 gold coins?'})
	node4:addChildKeyword({'yes'}, buyBlessing, {blessing = 4})
	node4:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Then not.'})

local node5 = keywordHandler:addKeyword({'fifth bless'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you want to buy the fifth blessing for 10000 gold coins?'})
	node5:addChildKeyword({'yes'}, buyBlessing, {blessing = 5})
	node5:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Then not.'})

keywordHandler:addKeyword({'blessing'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = "I can provide you with five blessings... the 'first bless', 'second bless', 'third bless', 'fourth bless' and the 'fifth bless', they cost 10000 gold coins each."})
keywordHandler:addKeyword({'blessings'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = "I can provide you with five blessings... the 'first bless', 'second bless', 'third bless', 'fourth bless' and the 'fifth bless', they cost 10000 gold coins each."})
keywordHandler:addKeyword({'offer'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = "I can provide you with five blessings... the 'first bless', 'second bless', 'third bless', 'fourth bless' and the 'fifth bless', they cost 10000 gold coins each."})
keywordHandler:addKeyword({'help'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = "I can provide you with five blessings... the 'first bless', 'second bless', 'third bless', 'fourth bless' and the 'fifth bless', they cost 10000 gold coins each."})

npcHandler:addModule(FocusModule:new())