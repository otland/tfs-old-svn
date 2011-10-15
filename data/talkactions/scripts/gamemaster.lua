local ignore = createConditionObject(CONDITION_GAMEMASTER, -1, false, GAMEMASTER_IGNORE, CONDITIONID_DEFAULT)
local teleport = createConditionObject(CONDITION_GAMEMASTER, -1, false, GAMEMASTER_TELEPORT, CONDITIONID_DEFAULT)

function onSay(cid, words, param, channel)
	local condition, subId, name = ignore, GAMEMASTER_IGNORE, "private messages ignoring"
	if(words:sub(2, 2) == "c") then
		condition, subId, name = teleport, GAMEMASTER_TELEPORT, "map click teleport"
	end

	local action = "off"
	if(not getCreatureCondition(cid, CONDITION_GAMEMASTER, subId, CONDITIONID_DEFAULT)) then
		doAddCondition(cid, condition)
		action = "on"
	else
		doRemoveCondition(cid, CONDITION_GAMEMASTER, subId)
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You have turned " .. action .. " " .. name .. ".")
	return true
end
