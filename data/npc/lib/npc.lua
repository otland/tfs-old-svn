function selfSayChannel(cid, message)
	return selfSay(message, cid, false)
end

function selfMoveToThing(id)
	errors(false)
	local thing = getThing(id)

	errors(true)
	if(thing.uid == 0) then
		return
	end

	local t = getThingPosition(id)
	selfMoveTo(t.x, t.y, t.z)
	return
end

function selfMoveTo(x, y, z)
	local position = {x = 0, y = 0, z = 0}
	if(type(x) ~= "table") then
		position = Position(x, y, z)
	else
		position = x
	end

	if(isValidPosition(position)) then
		doSteerCreature(getNpcId(), position)
	end
end

function selfMove(direction, flags)
	local flags = flags or 0
	doMoveCreature(getNpcId(), direction, flags)
end

function selfTurn(direction)
	doCreatureSetLookDirection(getNpcId(), direction)
end

function getNpcDistanceTo(id)
	errors(false)
	local thing = getThing(id)

	errors(true)
	if(thing.uid == 0) then
		return nil
	end

	local c = getCreaturePosition(id)
	if(not isValidPosition(c)) then
		return nil
	end

	local s = getCreaturePosition(getNpcId())
	if(not isValidPosition(s) or s.z ~= c.z) then
		return nil
	end

	return math.max(math.abs(s.x - c.x), math.abs(s.y - c.y))
end

function doMessageCheck(message, keyword)
	if(type(keyword) == "table") then
		return table.isStrIn(keyword, message)
	end

	local a, b = message:lower(), keyword:lower()
	if(keyword == message) then
		return true
	end

	return message:find(keyword) and not message:find('(%w+)' .. keyword)
end

function doNpcSellItem(cid, itemid, amount, subType, ignoreCap, inBackpacks, backpack)
	local amount, subType, ignoreCap, item = amount or 1, subType or 1, ignoreCap and true or false, 0
	if(isItemStackable(itemid)) then
		if(isItemRune(itemid)) then
			amount = amount * subType
		end

		local count = amount
		repeat
			item = doCreateItemEx(itemid, math.min(100, count))
			if(doPlayerAddItemEx(cid, item, ignoreCap) ~= RETURNVALUE_NOERROR) then
				return 0, 0
			end

			count = count - math.min(100, count)
		until count == 0
		return amount, 0
	end

	local a = 0
	if(inBackpacks) then
		local container, b = doCreateItemEx(backpack, 1), 1
		for i = 1, amount do
			item = doAddContainerItem(container, itemid, subType)
			if(itemid == ITEM_PARCEL) then
				doAddContainerItem(item, ITEM_LABEL)
			end

			if(isInArray({(getContainerCapById(backpack) * b), amount}, i)) then
				if(doPlayerAddItemEx(cid, container, ignoreCap) ~= RETURNVALUE_NOERROR) then
					b = b - 1
					break
				end

				a = i
				if(amount > i) then
					container = doCreateItemEx(backpack, 1)
					b = b + 1
				end
			end
		end

		return a, b
	end

	for i = 1, amount do
		item = doCreateItemEx(itemid, subType)
		if(itemid == ITEM_PARCEL) then
			doAddContainerItem(item, ITEM_LABEL)
		end

		if(doPlayerAddItemEx(cid, item, ignoreCap) ~= RETURNVALUE_NOERROR) then
			break
		end

		a = i
	end

	return a, 0
end

function doRemoveItemIdFromPosition(id, n, position)
	local thing = getThingFromPos({x = position.x, y = position.y, z = position.z, stackpos = 1})
	if(thing.itemid ~= id) then
		return false
	end

	doRemoveItem(thing.uid, n)
	return true
end

function getNpcName()
	return getCreatureName(getNpcId())
end

function getNpcPos()
	return getThingPosition(getNpcId())
end

function selfGetPosition()
	local t = getThingPosition(getNpcId())
	return t.x, t.y, t.z
end

msgcontains = doMessageCheck
moveToPosition = selfMoveTo
moveToCreature = selfMoveToThing
selfMoveToCreature = selfMoveToThing
selfMoveToPosition = selfMoveTo
isPlayerPremiumCallback = isPremium
doPosRemoveItem = doRemoveItemIdFromPosition
doRemoveItemIdFromPos = doRemoveItemIdFromPosition
doNpcBuyItem = doPlayerRemoveItem
doNpcSetCreatureFocus = selfFocus
getNpcCid = getNpcId
getDistanceTo = getNpcDistanceTo
getDistanceToCreature = getNpcDistanceTo
getNpcDistanceToCreature = getNpcDistanceTo