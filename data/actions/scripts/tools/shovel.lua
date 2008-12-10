local holes = {468, 481, 483}
local sand = {231, 9059}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(holes, itemEx.itemid) == TRUE) then
		doTransformItem(itemEx.uid, itemEx.itemid + 1)
		doDecayItem(itemEx.uid)
	elseif(isInArray(sand, itemEx.itemid) == TRUE) then
		local rand = math.random(1, 100)
		if(rand >= 1 and rand <= 5) then
			doCreateItem(2159, 1, toPosition)
		elseif(rand > 85) then
			doSummonCreature("Scarab", toPosition)
		end

		doSendMagicEffect(toPosition, CONST_ME_POFF)
	end

	return TRUE
end
