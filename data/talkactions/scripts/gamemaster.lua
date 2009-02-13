local ignore = createConditionObject(CONDITION_GAMEMASTER, -1, FALSE, GAMEMASTER_IGNORE)
local teleport = createConditionObject(CONDITION_GAMEMASTER, -1, FALSE, GAMEMASTER_TELEPORT)

function onSay(cid, words, param)
	local condition = ignore
	local subId = GAMEMASTER_IGNORE
	if(words:sub(2, 2) == "c") then
		condition = teleport
		subId = GAMEMASTER_TELEPORT
	end

	if(getCreatureCondition(cid, CONDITION_GAMEMASTER, subId) ~= TRUE) then
		doAddCondition(cid, condition)
	else
		doRemoveCondition(cid, CONDITION_GAMEMASTER, subId)
	end

	return TRUE
end
