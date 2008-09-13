-- TODO: Rewrite this script using fluidtypes from LIQUIDS doc-file,
--		and correct itemid's to recieve the liquids.

local drunk = createConditionObject(CONDITION_DRUNK)
setConditionParam(drunk, CONDITION_PARAM_TICKS, 60000)

local poison = createConditionObject(CONDITION_POISON)
addDamageCondition(poison, 2, 6000, -5)
addDamageCondition(poison, 3, 6000, -4)
addDamageCondition(poison, 5, 6000, -3)
addDamageCondition(poison, 10, 6000, -2)
addDamageCondition(poison, 20, 6000, -1)

local fluidType = {3, 4, 5, 7, 10, 11, 13, 15, 19}
local fluidMessage = {"Aah...", "Urgh!", "Mmmh.", "Aaaah...", "Aaaah...", "Urgh!", "Urgh!", "Aah...", "Urgh!"}
function onUse(cid, item, fromPosition, itemEx, toPosition)
	if itemEx.itemid == 1 then
		if item.type == 0 then
			doPlayerSendCancel(cid, "It is empty.")
		elseif itemEx.uid == cid then
			doChangeTypeItem(item.uid, 0)
			if item.type == 3 or item.type == 15 then
				doTargetCombatCondition(0, cid, drunk, CONST_ME_NONE)
			elseif item.type == 4 then
				doTargetCombatCondition(0, cid, poison, CONST_ME_NONE)
			elseif item.type == 7 then
				doPlayerAddMana(cid, math.random(50, 150))
				doSendMagicEffect(fromPosition, CONST_ME_MAGIC_BLUE)
			elseif item.type == 10 then
				doCreatureAddHealth(cid, 60)
				doSendMagicEffect(fromPosition, CONST_ME_MAGIC_BLUE)
			end
			for i = 0, table.maxn(fluidType) do
				if item.type == fluidType[i] then
					doCreatureSay(cid, fluidMessage[i], TALKTYPE_ORANGE_1)
					return TRUE
				end
			end
			doCreatureSay(cid, "Gulp.", TALKTYPE_ORANGE_1)
		else
			local splash = doCreateItem(2025, item.type, toPosition)
			doChangeTypeItem(item.uid, 0)
			doDecayItem(splash)
		end
	elseif (itemEx.itemid >= 490 and itemEx.itemid <= 493) or (itemEx.itemid >= 4608 and itemEx.itemid <= 4625) or (itemEx.itemid >= 618 and itemEx.itemid <= 629) or itemEx.itemid == 1771 then
		doChangeTypeItem(item.uid, 9)
	elseif itemEx.itemid == 103 then
		doChangeTypeItem(item.uid, 19)
	elseif (itemEx.itemid >= 598 and itemEx.itemid < 712) or itemEx.itemid == 1509 then
		doChangeTypeItem(item.uid, 26)
	elseif (itemEx.itemid >= 351 and itemEx.itemid <= 355) then
		doChangeTypeItem(item.uid, 19)
	elseif (itemEx.itemid >= 602 and itemEx.itemid <= 605) then
		doChangeTypeItem(item.uid, 28)
	elseif itemEx.itemid == 1772 then
		doChangeTypeItem(item.uid, 3)
	elseif itemEx.itemid == 1773 then
		doChangeTypeItem(item.uid, 15)
	elseif item.type == 0 then
		doPlayerSendCancel(cid, "It is empty.")
	else
		if toPosition.x == CONTAINER_POSITION then
			toPosition = getCreaturePosition(cid)
		end
		splash = doCreateItem(2025, item.type, toPosition)
		doChangeTypeItem(item.uid, 0)
		doDecayItem(splash)
	end
	return TRUE
end