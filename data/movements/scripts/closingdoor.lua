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
	local tmpPos = position
	tmpPos.stackpos = -1

	local i, tmpItem, tileCount = 1, {uid = 1}, getTileThingByPos(tmpPos)
	while(tmpItem.uid ~= 0 and i < tileCount) do
		tmpPos.stackpos = i
		tmpItem = getTileThingByPos(tmpPos)
		if(tmpItem.uid ~= item.uid and tmpItem.uid ~= 0 and isMoveable(tmpItem.uid)) then
			doRemoveItem(tmpItem.uid)
		else
			i = i + 1
		end
	end

	doTransformItem(item.uid, item.itemid - 1)
	return true
end
