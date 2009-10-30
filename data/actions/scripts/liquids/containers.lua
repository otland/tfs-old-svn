local DISTILLERY = {5513, 5514, 5469, 5470}
local ITEM_RUM_FLASK = 5553
local ITEM_POOL = 2016

local TYPE_EMPTY = 0
local TYPE_WATER = 1
local TYPE_BLOOD = 2
local TYPE_BEER = 3
local TYPE_SLIME = 4
local TYPE_MANA_FLUID = 7
local TYPE_LIFE_FLUID = 10
local TYPE_OIL = 11
local TYPE_WINE = 15
local TYPE_MUD = 19
local TYPE_LAVA = 26
local TYPE_RUM = 27
local TYPE_SWAMP = 28

local oilLamps = {[2046] = 2044}
local casks = {[1771] = TYPE_WATER, [1772] = TYPE_BEER, [1773] = TYPE_WINE}
local alcoholDrinks = {TYPE_BEER, TYPE_WINE, TYPE_RUM}
local poisonDrinks = {TYPE_SLIME, TYPE_SWAMP}

local drunk = createConditionObject(CONDITION_DRUNK)
setConditionParam(drunk, CONDITION_PARAM_TICKS, 60000)

local poison = createConditionObject(CONDITION_POISON)
setConditionParam(poison, CONDITION_PARAM_DELAYED, true) -- Condition will delay the first damage from when it's added
setConditionParam(poison, CONDITION_PARAM_MINVALUE, -50) -- Minimum damage the condition can do at total
setConditionParam(poison, CONDITION_PARAM_MAXVALUE, -120) -- Maximum damage
setConditionParam(poison, CONDITION_PARAM_STARTVALUE, -5) -- The damage the condition will do on the first hit
setConditionParam(poison, CONDITION_PARAM_TICKINTERVAL, 4000) -- Delay between damages
setConditionParam(poison, CONDITION_PARAM_FORCEUPDATE, true) -- Re-update condition when adding it(ie. min/max value)

local exhaust = createConditionObject(CONDITION_EXHAUST)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, (getConfigInfo('timeBetweenExActions') - 100))

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(itemEx.uid == cid) then
		if(item.type == TYPE_EMPTY) then
			doPlayerSendCancel(cid, "It is empty.")
			return true
		end

		if(item.type == TYPE_MANA_FLUID) then
			if(hasCondition(cid, CONDITION_EXHAUST_HEAL)) then
				doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
				return true
			end

			if(not doPlayerAddMana(cid, math.random(80, 160))) then
				return false
			end

			doCreatureSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doSendMagicEffect(toPosition, CONST_ME_MAGIC_BLUE)
			doAddCondition(cid, exhaust)
		elseif(item.type == TYPE_LIFE_FLUID) then
			if(hasCondition(cid, CONDITION_EXHAUST_HEAL)) then
				doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
				return true
			end

			if(not doCreatureAddHealth(cid, math.random(40, 75))) then
				return false
			end

			doCreatureSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doSendMagicEffect(toPosition, CONST_ME_MAGIC_BLUE)
			doAddCondition(cid, exhaust)
		elseif(isInArray(alcoholDrinks, item.type)) then
			if(not doTargetCombatCondition(0, cid, drunk, CONST_ME_NONE)) then
				return false
			end

			doCreatureSay(cid, "Aaah...", TALKTYPE_ORANGE_1)
		elseif(isInArray(poisonDrinks, item.type)) then
			if(not doTargetCombatCondition(0, cid, poison, CONST_ME_NONE)) then
				return false
			end

			doCreatureSay(cid, "Urgh!", TALKTYPE_ORANGE_1)
		else
			doCreatureSay(cid, "Gulp.", TALKTYPE_ORANGE_1)
		end

		doChangeTypeItem(item.uid, TYPE_EMPTY)
		return true
	end

	if(not isCreature(itemEx.uid)) then
		if(item.type == TYPE_EMPTY) then
			if(item.itemid == ITEM_RUM_FLASK and isInArray(DISTILLERY, itemEx.itemid)) then
				if(itemEx.actionid == 100) then
					doItemEraseAttribute(itemEx.uid, "description")
					doItemEraseAttribute(itemEx.uid, "aid")
					doChangeTypeItem(item.uid, TYPE_RUM)
				else
					doPlayerSendCancel(cid, "You have to process the bunch into the distillery to get rum.")
				end
				return true
			end

			if(isItemFluidContainer(itemEx.itemid) and itemEx.type ~= TYPE_EMPTY) then
				doChangeTypeItem(item.uid, itemEx.type)
				doChangeTypeItem(itemEx.uid, TYPE_EMPTY)
				return true
			end

			if(casks[itemEx.itemid] ~= nil) then
				doChangeTypeItem(item.uid, casks[itemEx.itemid])
				return true
			end

			local fluidEx = getFluidSourceType(itemEx.itemid)
			if(fluidEx ~= false) then
				doChangeTypeItem(item.uid, fluidEx)
				return true
			end

			doPlayerSendCancel(cid, "It is empty.")
			return true
		end

		if(item.type == TYPE_OIL and oilLamps[itemEx.itemid] ~= nil) then
			doTransformItem(itemEx.uid, oilLamps[itemEx.itemid])
			doChangeTypeItem(item.uid, TYPE_NONE)
			return true
		end

		if(hasProperty(itemEx.uid, CONST_PROP_BLOCKSOLID)) then
			return false
		end
	end

	doDecayItem(doCreateItem(ITEM_POOL, item.type, toPosition))
	doChangeTypeItem(item.uid, TYPE_EMPTY)
	return true
end
