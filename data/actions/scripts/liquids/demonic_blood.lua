local POTIONS = {7588, 7589}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	doTransformItem(item.uid, POTIONS[math.random(1, #POTIONS)])
	doSendMagicEffect(getThingPos(item.uid), CONST_ME_MAGIC_RED)
	return true
end
