local MIN = 50
local MAX = 100
local EMPTY_POTION = 7636

local exhaust = createConditionObject(CONDITION_EXHAUST_HEAL)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('timeBetweenExActions'))

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if isPlayer(itemEx.uid) == FALSE then
		return FALSE
	end

	if hasCondition(cid, CONDITION_EXHAUST_HEAL) == TRUE then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return TRUE
	end

	if doCreatureAddHealth(itemEx.uid, math.random(MIN, MAX)) == LUA_ERROR then
		return FALSE
	end

	doAddCondition(cid, exhaust)
	doSendMagicEffect(getThingPos(itemEx.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(itemEx.uid, "Aaaah...", TALKTYPE_ORANGE_1)
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end