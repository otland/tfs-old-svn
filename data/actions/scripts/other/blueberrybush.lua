function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.itemid == ITEM_BLUEBERRYBUSH) then
		doTransformItem(item.uid, ITEM_BUSH)
		doCreateItem(ITEM_BLUEBERRY, 3, fromPosition)
		addEvent(transformBack, 300000, fromPosition)
	end

	return TRUE
end

function transformBack(position)
	position.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE

	local topThing = getThingFromPos(position)
	if(topThing.itemid == ITEM_BLUEBERRY) then
		addEvent(transformBack, 300000, position)
	else
		topThing = getTileItemById(position, ITEM_BUSH)
		if(topThing.itemid ~= 0) then
			doTransformItem(topThing.uid, ITEM_BLUEBERRYBUSH)
		end
	end
end
