local config = {
	manaCost = 300,
	soulCost = 2
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.itemid == 2147 and itemEx.itemid == 2342) then
		doTransformItem(itemEx.uid, 2343)
		doRemoveItem(item.uid, 1)

		doSendMagicEffect(toPosition, CONST_ME_MAGIC_RED)
		return true
	elseif(item.itemid == 7760 and isInArray({9934, 10022}, itemEx.itemid)) then
		doTransformItem(itemEx.uid, 9933)
		doRemoveItem(item.uid, 1)

		doSendMagicEffect(toPosition, CONST_ME_MAGIC_RED)
		return true
	end

	local subtype = item.type
	if(subtype == 0) then
		subtype = 1
	end
 
	local mana = config.manaCost * subtype
	local soul = config.soulCost * subtype
	if(isInArray(enchantableGems, item.itemid)) then
		if(getPlayerMana(cid) < mana) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHMANA)
			return true
		end

		if(getPlayerSoul(cid) < soul) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHSOUL)
			return true
		end

		local a = 1
		for i, id in pairs(enchantableGems) do
			if(item.itemid == id) then
				a = i
			end
		end

		if(itemEx.actionid ~= 100 or not isInArray(enchantingAltars[a], itemEx.itemid)) then
			return false
		end

		doTransformItem(item.uid, enchantedGems[a])
		doPlayerAddMana(cid, -mana)
		doPlayerAddSoul(cid, -soul)
		doSendMagicEffect(fromPosition, CONST_ME_HOLYDAMAGE)
		return true
	end
 
	if(isInArray(enchantedGems, item.itemid)) then
		if(not isInArray(enchantableItems, itemEx.itemid)) then
			return false
		end

		local b = 1
		for i, id in pairs(enchantedGems) do
			if(item.itemid == id) then
				b = i
			end
		end

		if(not isInArray({2544, 8905}, itemEx.itemid)) then
			itemEx.type = 1000
		end

		doTransformItem(itemEx.uid, enchantedItems[itemEx.itemid][b], itemEx.type)
		doSendMagicEffect(getThingPos(itemEx.uid), CONST_ME_HOLYDAMAGE)
		doRemoveItem(item.uid, 1)
		return true
	end

	return false
end
