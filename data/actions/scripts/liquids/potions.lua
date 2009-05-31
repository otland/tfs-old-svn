local config = {
	removeOnUse = "no",
	healthMultiplier = 1.0,
	manaMultiplier = 1.0
}
config.removeOnUse = getBooleanFromString(config.removeOnUse)

local HEALTH_SMALL, HEALTH_NORMAL, HEALTH_STRONG, HEALTH_GREAT, HEALTH_ULTIMATE = 8704, 7618, 7588, 7591, 8473
local MANA_NORMAL, MANA_STRONG, MANA_GREAT = 7620, 7589, 7590
local SPIRIT_GREAT = 8472
local POTIONS = {
	[HEALTH_SMALL] = {empty = 7636, health = {50, 100}},
	[HEALTH_NORMAL] = {empty = 7636, health = {100, 200}},
	[HEALTH_STRONG] = {empty = 7634, health = {200, 400}, level = 50, vocations = {3, 4, 7, 8}, vocStr = "knights and paladins"},
	[HEALTH_GREAT] = {empty = 7635, health = {500, 700},	level = 80, vocations = {4, 8}, vocStr = "knights"},
	[HEALTH_ULTIMATE] = {empty = 7635, health = {800, 1000}, level = 130, vocations = {4, 8}, vocStr = "knights"},

	[MANA_NORMAL] =	{empty = 7636, mana = {70, 130}},
	[MANA_STRONG] =	{empty = 7634, mana = {110, 190}, level = 50, vocations = {1, 2, 3, 5, 6, 7}, vocStr = "sorcerers, druids and paladins"},
	[MANA_GREAT] = {empty = 7635, mana = {200, 300}, level = 80, vocations = {1, 2, 5, 6}, vocStr = "sorcerers and druids"},

	[SPIRIT_GREAT] = {empty = 7635, health = {200, 400}, mana = {110, 190}, level = 80, vocations = {3, 7}, vocStr = "paladins"}
}

local exhaust = createConditionObject(CONDITION_EXHAUST)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, (getConfigInfo('timeBetweenExActions') - 100))

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local potion = POTIONS[item.itemid]
	if(not potion or not isPlayer(itemEx.uid)) then
		return false
	end

	if(hasCondition(cid, CONDITION_EXHAUST_HEAL)) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return true
	end

	if(((potion.vocations and not isInArray(potion.vocations, getPlayerVocation(cid))) or (potion.level and getPlayerLevel(cid) < potion.level)) and
		not getPlayerCustomFlagValue(cid, PlayerCustomFlag_GamemasterPrivileges))
	then
		doCreatureSay(itemEx.uid, "Only " .. potion.vocStr .. (potion.level and (" of level " .. potion.level) or "") .. " or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return true
	end

	local health = potion.health
	if(health and not doCreatureAddHealth(itemEx.uid, math.ceil(math.random(health[1], health[2]) * config.healthMultiplier))) then
		return false
	end

	local mana = potion.mana
	if(mana and not doPlayerAddMana(itemEx.uid, math.ceil(math.random(mana[1], mana[2]) * config.manaMultiplier))) then
		return false
	end

	doSendMagicEffect(getThingPos(itemEx.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(itemEx.uid, "Aaaah...", TALKTYPE_ORANGE_1)

	doAddCondition(cid, exhaust)
	if(not potion.empty or config.removeOnUse) then
		doRemoveItem(item.uid)
		return true
	end

	doTransformItem(item.uid, potion.empty)
	return true
end
