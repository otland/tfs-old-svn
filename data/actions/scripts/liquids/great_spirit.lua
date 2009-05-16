local MIN_HEALTH = 200
local MAX_HEALTH = 400
local MIN_MANA = 110
local MAX_MANA = 190
local EMPTY_POTION = 7635

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

	if((not isPaladin(itemEx.uid) or getPlayerLevel(itemEx.uid) < 80) and
		not getPlayerCustomFlagValue(itemEx.uid, PlayerCustomFlag_GamemasterPrivileges))
	then
		doCreatureSay(itemEx.uid, "Only paladins of level 80 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return true
	end

	if(not doCreatureAddHealth(itemEx.uid, math.random(MIN_HEALTH, MAX_HEALTH)) or
		not doPlayerAddMana(itemEx.uid, math.random(MIN_MANA, MAX_MANA))) then
			return false
	end

	doAddCondition(cid, exhaust)
	doSendMagicEffect(getThingPos(itemEx.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(itemEx.uid, "Aaaah...", TALKTYPE_ORANGE_1)
	doTransformItem(item.uid, EMPTY_POTION)
	return true
end
