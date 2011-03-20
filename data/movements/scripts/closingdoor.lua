function onStepOut(cid, item, position, fromPosition)
	if(getTileInfo(position).creatures > 0) then
		return true
	end

	local newPosition = {x = position.x, y = position.y, z = position.z}
	if(isInArray(verticalOpenDoors, item.itemid)) then
		newPosition.x = newPosition.x + 1
	else
		newPosition.y = newPosition.y + 1
	end

	doRelocate(position, newPosition)
	position.stackpos = -1

	local i, tileItem, tileCount = 1, {uid = 1}, getTileThingByPos(position)
	while(tileItem.uid ~= 0 and i < tileCount) do
		position.stackpos = i
		tileItem = getTileThingByPos(position)
		if(tileItem.uid ~= 0 and tileItem.uid ~= item.uid and isMovable(tileItem.uid)) then
			doRemoveItem(tileItem.uid)
		else
			i = i + 1
		end
	end

	doTransformItem(item.uid, item.itemid - 1)
	return true
end
