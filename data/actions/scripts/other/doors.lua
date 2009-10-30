local function checkStackpos(item, position)
	position.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE
	local thing = getThingFromPos(position)

	position.stackpos = STACKPOS_TOP_FIELD
	local field = getThingFromPos(position)

	return (item.uid == thing.uid or thing.itemid < 100 or field.itemid == 0)
end

local function doorEnter(cid, item, toPosition)
	doTransformItem(item.uid, item.itemid + 1)
	doTeleportThing(cid, toPosition)
end

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(fromPosition.x ~= CONTAINER_POSITION and isPlayerPzLocked(cid) and getTileInfo(fromPosition).protection) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		return true
	end

	if(getItemLevelDoor(item.itemid) > 0) then
		if(item.actionid == 189) then
			if(not isPremium(cid)) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
				return true
			end

			doorEnter(cid, item, toPosition)
			return true
		end

		local gender = item.actionid - 186
		if(isInArray({PLAYERSEX_FEMALE,  PLAYERSEX_MALE, PLAYERSEX_GAMEMASTER}, gender)) then
			if(gender ~= getPlayerSex(cid)) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
				return true
			end

			doorEnter(cid, item, toPosition)
			return true
		end

		local skull = item.actionid - 180
		if(skull >= SKULL_NONE and skull <= SKULL_BLACK) then
			if(skull ~= getCreatureSkullType(cid)) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
				return true
			end

			doorEnter(cid, item, toPosition)
			return true
		end

		local group = item.actionid - 150
		if(group >= 0 and group < 30) then
			if(group > getPlayerGroupId(cid)) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
				return true
			end

			doorEnter(cid, item, toPosition)
			return true
		end

		local vocation = item.actionid - 100
		if(vocation >= 0 and vocation < 50) then
			local playerVocationInfo = getVocationInfo(getPlayerVocation(cid))
			if(playerVocationInfo.id ~= vocation and playerVocationInfo.fromVocation ~= vocation) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
				return true
			end

			doorEnter(cid, item, toPosition)
			return true
		end

		if(item.actionid == 190 or (item.actionid ~= 0 and getPlayerLevel(cid) >= (item.actionid - getItemLevelDoor(item.itemid)))) then
			doorEnter(cid, item, toPosition)
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
		end

		return true
	end

	if(isInArray(specialDoors, item.itemid)) then
		if(item.actionid == 100 or (item.actionid ~= 0 and getPlayerStorageValue(cid, item.actionid) > 0)) then
			doorEnter(cid, item, toPosition)
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The door seems to be sealed against unwanted intruders.")
		end

		return true
	end

	if(isInArray(keys, item.itemid)) then
		if(itemEx.actionid > 0) then
			if(item.actionid == itemEx.actionid and doors[itemEx.itemid] ~= nil) then
				doTransformItem(itemEx.uid, doors[itemEx.itemid])
				return true
			end

			doPlayerSendCancel(cid, "The key does not match.")
			return true
		end

		return false
	end

	if(isInArray(horizontalOpenDoors, item.itemid) and checkStackpos(item, fromPosition)) then
		local newPosition = toPosition
		newPosition.y = newPosition.y + 1
		local doorPosition = fromPosition
		doorPosition.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE
		local doorCreature = getThingfromPos(doorPosition)
		if(doorCreature.itemid ~= 0) then
			local pzDoorPosition = getTileInfo(doorPosition).protection
			local pzNewPosition = getTileInfo(newPosition).protection
			if((pzDoorPosition and not pzNewPosition and doorCreature.uid ~= cid) or
				(not pzDoorPosition and pzNewPosition and doorCreature.uid == cid and isPlayerPzLocked(cid))) then
				doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
			else
				doTeleportThing(doorCreature.uid, newPosition)
				if(not isInArray(closingDoors, item.itemid)) then
					doTransformItem(item.uid, item.itemid - 1)
				end
			end

			return true
		end

		doTransformItem(item.uid, item.itemid - 1)
		return true
	end

	if(isInArray(verticalOpenDoors, item.itemid) and checkStackpos(item, fromPosition)) then
		local newPosition = toPosition
		newPosition.x = newPosition.x + 1
		local doorPosition = fromPosition
		doorPosition.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE
		local doorCreature = getThingfromPos(doorPosition)
		if(doorCreature.itemid ~= 0) then
			if(getTileInfo(doorPosition).protection and not getTileInfo(newPosition).protection and doorCreature.uid ~= cid) then
				doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
			else
				doTeleportThing(doorCreature.uid, newPosition)
				if(not isInArray(closingDoors, item.itemid)) then
					doTransformItem(item.uid, item.itemid - 1)
				end
			end

			return true
		end

		doTransformItem(item.uid, item.itemid - 1)
		return true
	end

	if(doors[item.itemid] ~= nil and checkStackpos(item, fromPosition)) then
		if(item.actionid == 0) then
			doTransformItem(item.uid, doors[item.itemid])
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "It is locked.")
		end

		return true
	end

	return false
end
