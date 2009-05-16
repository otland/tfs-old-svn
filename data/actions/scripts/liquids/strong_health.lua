local MIN = 200
local MAX = 400
local EMPTY_POTION = 7634

local exhaust = createConditionObject(CONDITION_EXHAUST)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, (getConfigInfo('timeBetweenExActions') - 100))

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(not isPlayer(itemEx.uid)) then
		return false
	end

	if(hasCondition(cid, CONDITION_EXHAUST_HEAL)) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return true
	end

	if((not(isKnight(itemEx.uid) or isPaladin(itemEx.uid)) or getPlayerLevel(itemEx.uid) < 50) and
		not getPlayerCustomFlagValue(itemEx.uid, PlayerCustomFlag_GamemasterPrivileges))
	then
		doCreatureSay(itemEx.uid, "Only knights and paladins of level 50 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return true
	end

	if(not doCreatureAddHealth(itemEx.uid, math.random(MIN, MAX))) then
		return false
	end

	doAddCondition(cid, exhaust)
	doSendMagicEffect(getThingPos(itemEx.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(itemEx.uid, "Aaaah...", TALKTYPE_ORANGE_1)
	doTransformItem(item.uid, EMPTY_POTION)
	return true
end
