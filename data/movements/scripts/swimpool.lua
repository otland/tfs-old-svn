local outfit = {lookType = 267, lookHead = 0, lookBody = 0, lookLegs = 0, lookFeet = 0, lookTypeEx = 0, lookAddons = 0}

function onStepIn(cid, item, toPosition, fromPosition, actor)
	if(isPlayer(cid)) then
		return true
	end

	if(getTileItemById(fromPosition, 4820).itemid == 0) then
		doSendMagicEffect(getThingPos(cid), CONST_ME_WATERSPLASH)
	end

	doRemoveConditions(cid, true)
	doSetCreatureOutfit(cid, outfit, -1)
	return true
end

function onStepOut(cid, item, toPosition, fromPosition, actor)
	if(isPlayer(cid)) then
		doRemoveCondition(cid, CONDITION_OUTFIT)
	end
end
