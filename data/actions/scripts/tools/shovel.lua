function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(HOLES, itemEx.itemid)) then
		if(itemEx.itemid ~= 8579) then
			itemEx.itemid = itemEx.itemid + 1
		else
			itemEx.itemid = 8585
		end

		doTransformItem(itemEx.uid, itemEx.itemid)
		doDecayItem(itemEx.uid)
		return true
	elseif(SAND_HOLES[itemEx.itemid] ~= nil) then
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		doTransformItem(itemEx.uid, SAND_HOLES[itemEx.itemid])

		doDecayItem(itemEx.uid)
		return true
	elseif(itemEx.itemid == SAND and not isRookie(cid)) then
		local rand = math.random(1, 100)
		if(rand >= 1 and rand <= 5) then
			doCreateItem(ITEM_SCARAB_COIN, 1, toPosition)
		elseif(rand > 85) then
			doCreateMonster("Scarab", toPosition, false)
		end

		doSendMagicEffect(toPosition, CONST_ME_POFF)
		return true
	end

	return false
end
