CONSTRUCTIONS = {[3901] = 1650, [3902] = 1658, [3903] = 1666, [3904] = 1670, [3905] = 3813, [3906] = 3817, [3909] = 1614, [3910] = 1615, [3911] = 1616, [3912] = 1619, [3913] = 3805, [3914] = 3807, [3919] = 3809, [3931] = 2105, [3938] = 1750, [3921] = 1714, [3932] = 1724, [3934] = 1732, [3935] = 1775, [3915] = 1740, [3908] = 2603, [3918] = 2095, [3917] = 2084, [3926] = 2080, [3927] = 2098, [3933] = 1728, [3937] = 2064}
function onUse(cid, item, fromPosition, itemEx, toPosition)
	if getTileHouseInfo(fromPosition) == FALSE then
		doPlayerSendCancel(cid,"You may only construct this inside a house.")
	elseif fromPosition.x == CONTAINER_POSITION then
		doPlayerSendCancel(cid, "Put the construction kit on the floor first.")
	elseif CONSTRUCTIONS[item.itemid] ~= nil then
		doTransformItem(item.uid, CONSTRUCTIONS[item.itemid])
		doSendMagicEffect(fromPosition, CONST_ME_POFF)
	else
		return FALSE
	end
	return TRUE
end