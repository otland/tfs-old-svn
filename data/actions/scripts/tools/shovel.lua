function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(HOLES, itemEx.itemid)) then
		local newId = itemEx.itemid + 1
		if(itemEx.itemid == 8579) then
			newId = 8585
		end

		doTransformItem(itemEx.uid, newId)
		doDecayItem(itemEx.uid)
		return true
	elseif(isInArray(SAND, itemEx.itemid)) then
		local rand = math.random(1, 100)
		if(itemEx.actionid  == 100 and rand <= 20) then
			doTransformItem(itemEx.uid, 489)
			doDecayItem(itemEx.uid)
		elseif(rand >= 1 and rand <= 5) then
			doCreateItem(2159, 1, toPosition)
		elseif(rand > 85) then
			doCreateMonster("Scarab", toPosition, false)
		end

		doSendMagicEffect(toPosition, CONST_ME_POFF)
		return true
	end

	return false
end
