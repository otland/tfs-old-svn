local outfit = {lookType = 267, lookHead = 0, lookBody = 0, lookLegs = 0, lookFeet = 0, lookTypeEx = 0, lookAddons = 0}

function onStepIn(cid, item, position, lastPosition, fromPosition, toPosition, actor)
	if(not isPlayer(cid)) then
		return true
	end

	if(fromPosition.x ~= 0 and fromPosition.y ~= 0 and getTileItemById(fromPosition, 4820).itemid == 0) then
		doSendMagicEffect(getThingPos(cid), CONST_ME_WATERSPLASH)
	end

	doRemoveConditions(cid, true)
	doSetCreatureOutfit(cid, outfit, -1)
	return true
end

function onStepOut(cid, item, position, lastPosition, fromPosition, toPosition, actor)
	if(isPlayer(cid) and getTileItemById(toPosition, 4820).itemid == 0) then
		doRemoveCondition(cid, CONDITION_OUTFIT)
	end

	return true
end
