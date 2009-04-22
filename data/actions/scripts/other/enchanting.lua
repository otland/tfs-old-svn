local config = {
	manaCost = 300,
	soulCost = 2
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.itemid == 2147 and itemEx.itemid == 2342) then
		doTransformItem(itemEx.uid, 2343)
		doRemoveItem(item.uid, 1)
		doSendMagicEffect(toPosition, CONST_ME_MAGIC_RED)
		return TRUE
	end

	local type = item.type
	if(type == 0) then
		type = 1
	end
 
	local mana = config.manaCost * type
	local soul = config.soulCost * type
 
	if(isInArray(enchantableGems, item.itemid) == TRUE) then
		if(getPlayerMana(cid) < mana) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHMANA)
			return TRUE
		end

		if(getPlayerSoul(cid) < soul) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHSOUL)
			return TRUE
		end

		local a = 1
		for i, id in pairs(enchantableGems) do
			if(item.itemid == id) then
				a = i
			end
		end

		if(isInArray(enchantingAltars[a], itemEx.itemid) ~= TRUE or itemEx.actionid ~= 100) then
			return FALSE
		end

		doTransformItem(item.uid, enchantedGems[a])
		doPlayerAddMana(cid, -mana)
		doPlayerAddSoul(cid, -soul)
		doSendMagicEffect(fromPosition, CONST_ME_HOLYDAMAGE)
		return TRUE
	end
 
	if(isInArray(enchantedGems, item.itemid) == TRUE) then
		if(isInArray(enchantableItems, itemEx.itemid) ~= TRUE) then
			return FALSE
		end

		local b = 1
		for i, id in pairs(enchantedGems) do
			if(item.itemid == id) then
				b = i
			end
		end

		if(isInArray({2544, 8905}, itemEx.itemid) ~= TRUE) then
			itemEx.type = 1000
		end

		doTransformItem(itemEx.uid, enchantedItems[itemEx.itemid][b], itemEx.type)
		doSendMagicEffect(getThingPos(itemEx.uid), CONST_ME_HOLYDAMAGE)
		doRemoveItem(item.uid, 1)
		return TRUE
	end

	return FALSE
end
